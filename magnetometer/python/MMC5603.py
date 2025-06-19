# Copyright (c) 2025 by T3 Foundation. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#     https://docs.t3gemstone.org/en/license
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

import time
from dataclasses import dataclass
from enum import IntEnum
from typing import Optional, Tuple

try:
    import smbus2
except ImportError:
    print("Error: smbus2 library not found. Install with: pip install smbus2")
    raise

# Constants
MMC56X3_DEFAULT_ADDRESS = 0x30
MMC56X3_CHIP_ID = 0x10


class MMC56X3Register(IntEnum):
    PRODUCT_ID = 0x39
    CTRL0_REG = 0x1B
    CTRL1_REG = 0x1C
    CTRL2_REG = 0x1D
    STATUS_REG = 0x18
    OUT_TEMP = 0x09
    OUT_X_L = 0x00
    ODR_REG = 0x1A


@dataclass
class MagData:
    x: float = 0.0  # X-axis magnetic field in uTesla
    y: float = 0.0  # Y-axis magnetic field in uTesla
    z: float = 0.0  # Z-axis magnetic field in uTesla
    timestamp: int = 0  # Timestamp in milliseconds


@dataclass
class SensorInfo:
    name: str = ""  # Sensor name
    sensor_id: int = 0  # Sensor ID
    max_value: float = 0.0  # Maximum value in uTesla
    min_value: float = 0.0  # Minimum value in uTesla
    resolution: float = 0.0  # Resolution in uTesla/LSB


class MMC5603:
    def __init__(self):
        self._bus: Optional[smbus2.SMBus] = None
        self.m_address = 0
        self.m_sensor_id = 0
        self.m_odr_cache = 0
        self.m_ctrl2_cache = 0
        self.m_raw_x = 0
        self.m_raw_y = 0
        self.m_raw_z = 0

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    @staticmethod
    def _get_timestamp_ms() -> int:
        return int(time.time() * 1000)

    @staticmethod
    def _delay_ms(ms: int) -> None:
        time.sleep(ms / 1000.0)

    def _read_register(self, reg: int) -> Optional[int]:
        try:
            if self._bus is None:
                return None
            return self._bus.read_byte_data(self.m_address, reg)
        except Exception as e:
            print(f"Error reading register 0x{reg:02X}: {e}")
            return None

    def _write_register(self, reg: int, value: int) -> bool:
        try:
            if self._bus is None:
                return False
            self._bus.write_byte_data(self.m_address, reg, value)
            return True
        except Exception as e:
            print(f"Error writing register 0x{reg:02X}: {e}")
            return False

    def _read_registers(self, reg: int, length: int) -> Optional[list]:
        try:
            if self._bus is None:
                return None
            return self._bus.read_i2c_block_data(self.m_address, reg, length)
        except Exception as e:
            print(f"Error reading registers from 0x{reg:02X}: {e}")
            return None

    def init(self, i2c_bus: int, address: int = MMC56X3_DEFAULT_ADDRESS, sensor_id: int = 12345) -> bool:
        self.m_address = address
        self.m_sensor_id = sensor_id

        try:
            self._bus = smbus2.SMBus(i2c_bus)
        except Exception as e:
            print(f"Failed to open I2C bus {i2c_bus}: {e}")
            return False

        # Read chip ID
        chip_id = self._read_register(MMC56X3Register.PRODUCT_ID)
        if chip_id is None:
            print("Failed to read chip ID")
            self.close()
            return False

        if chip_id != MMC56X3_CHIP_ID and chip_id != 0x00:
            print(f"Invalid chip ID: 0x{chip_id:02X} (expected 0x{MMC56X3_CHIP_ID:02X})")
            self.close()
            return False

        if not self.reset():
            self.close()
            return False

        return True

    def close(self) -> None:
        if self._bus is not None:
            self._bus.close()
            self._bus = None

    def reset(self) -> bool:
        if self._bus is None:
            return False

        # Software reset
        if not self._write_register(MMC56X3Register.CTRL1_REG, 0x80):
            return False

        self._delay_ms(20)

        # Reset cache variables
        self.m_odr_cache = 0
        self.m_ctrl2_cache = 0

        # Perform magnetic set/reset
        if not self.magnet_set_reset():
            return False

        # Set to one-shot mode
        if not self.set_continuous_mode(False):
            return False

        return True

    def magnet_set_reset(self) -> bool:
        if self._bus is None:
            return False

        # Turn on SET bit
        if not self._write_register(MMC56X3Register.CTRL0_REG, 0x08):
            return False
        self._delay_ms(1)

        # Turn on RESET bit
        if not self._write_register(MMC56X3Register.CTRL0_REG, 0x10):
            return False
        self._delay_ms(1)

        return True

    def set_continuous_mode(self, continuous: bool) -> bool:
        if self._bus is None:
            return False

        if continuous:
            # Turn on cmm_freq_en bit
            if not self._write_register(MMC56X3Register.CTRL0_REG, 0x80):
                return False
            self.m_ctrl2_cache |= 0x10  # Turn on cmm_en bit
        else:
            self.m_ctrl2_cache &= ~0x10  # Turn off cmm_en bit

        return self._write_register(MMC56X3Register.CTRL2_REG, self.m_ctrl2_cache)

    def is_continuous_mode(self) -> bool:
        return (self.m_ctrl2_cache & 0x10) != 0

    def read_temperature(self) -> Tuple[bool, float]:
        if self._bus is None:
            return False, float("nan")

        if self.is_continuous_mode():
            return False, float("nan")

        # Trigger temperature measurement
        if not self._write_register(MMC56X3Register.CTRL0_REG, 0x02):
            return False, float("nan")

        # Wait for measurement to complete
        timeout = 1000
        while timeout > 0:
            status = self._read_register(MMC56X3Register.STATUS_REG)
            if status is None:
                return False, float("nan")
            if status & 0x80:
                break
            self._delay_ms(5)
            timeout -= 5

        if timeout <= 0:
            return False, float("nan")

        temp_data = self._read_register(MMC56X3Register.OUT_TEMP)
        if temp_data is None:
            return False, float("nan")

        # Convert to Celsius
        temperature = float(temp_data) * 0.8 - 75.0

        return True, temperature

    def read_mag(self) -> Tuple[bool, MagData]:
        data = MagData()

        if self._bus is None:
            return False, data

        if not self.is_continuous_mode():
            # Trigger one-shot measurement
            if not self._write_register(MMC56X3Register.CTRL0_REG, 0x01):
                return False, data

            # Wait for measurement to complete
            timeout = 1000
            while timeout > 0:
                status = self._read_register(MMC56X3Register.STATUS_REG)
                if status is None:
                    return False, data
                if status & 0x40:
                    break
                self._delay_ms(5)
                timeout -= 5

            if timeout <= 0:
                return False, data

        # Read 9 bytes of data
        buffer = self._read_registers(MMC56X3Register.OUT_X_L, 9)
        if buffer is None or len(buffer) != 9:
            return False, data

        # Parse 20-bit values
        self.m_raw_x = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[6] >> 4)
        self.m_raw_y = (buffer[2] << 12) | (buffer[3] << 4) | (buffer[7] >> 4)
        self.m_raw_z = (buffer[4] << 12) | (buffer[5] << 4) | (buffer[8] >> 4)

        # Apply center offset correction
        self.m_raw_x -= 1 << 19
        self.m_raw_y -= 1 << 19
        self.m_raw_z -= 1 << 19

        # Convert to signed 32-bit integers
        if self.m_raw_x >= (1 << 19):
            self.m_raw_x -= 1 << 20
        if self.m_raw_y >= (1 << 19):
            self.m_raw_y -= 1 << 20
        if self.m_raw_z >= (1 << 19):
            self.m_raw_z -= 1 << 20

        # Convert to uTesla (scale factor from datasheet: 0.00625 uT/LSB)
        data.x = float(self.m_raw_x) * 0.00625
        data.y = float(self.m_raw_y) * 0.00625
        data.z = float(self.m_raw_z) * 0.00625
        data.timestamp = self._get_timestamp_ms()

        return True, data

    def set_data_rate(self, rate: int) -> bool:
        if self._bus is None:
            return False

        # Limit rate to valid range
        if rate > 255:
            rate = 1000

        self.m_odr_cache = rate

        if rate == 1000:
            # High power mode for 1000 Hz
            if not self._write_register(MMC56X3Register.ODR_REG, 255):
                return False
            self.m_ctrl2_cache |= 0x80  # Turn on hpower bit
        else:
            # Normal mode
            if not self._write_register(MMC56X3Register.ODR_REG, rate):
                return False
            self.m_ctrl2_cache &= ~0x80  # Turn off hpower bit

        return self._write_register(MMC56X3Register.CTRL2_REG, self.m_ctrl2_cache)

    def get_data_rate(self) -> int:
        return self.m_odr_cache

    def get_sensor_info(self) -> SensorInfo:
        info = SensorInfo()
        info.name = "MMC5603"
        info.sensor_id = self.m_sensor_id
        info.max_value = 3000.0  # 30 gauss = 3000 uTesla
        info.min_value = -3000.0  # -30 gauss = -3000 uTesla
        info.resolution = 0.00625  # 20-bit resolution, 0.00625 uT/LSB
        return info
