// Copyright (c) 2025 by T3 Foundation. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//     https://www.t3gemstone.org/license
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "MMC5603.h"
#include <cerrno>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <threads.h>
#include <unistd.h>

MMC5603::MMC5603() {}

MMC5603::~MMC5603()
{
    close();
}

uint64_t MMC5603::get_timestamp_ms()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * 1000 + static_cast<uint64_t>(tv.tv_usec) / 1000;
}

void MMC5603::delay_ms(int ms)
{
    struct timespec request = {0, ms * 1'000'000};
    struct timespec remaining;
    thrd_sleep(&request, &remaining);
}

bool MMC5603::read_register(uint8_t reg, uint8_t& value)
{
    if (write(m_fd, &reg, 1) != 1)
    {
        return false;
    }

    if (read(m_fd, &value, 1) != 1)
    {
        return false;
    }

    return true;
}

bool MMC5603::write_register(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};

    if (write(m_fd, buffer, 2) != 2)
    {
        return false;
    }

    return true;
}

bool MMC5603::read_registers(uint8_t reg, uint8_t* buffer, int length)
{
    if (write(m_fd, &reg, 1) != 1)
    {
        return false;
    }

    if (read(m_fd, buffer, length) != length)
    {
        return false;
    }

    return true;
}

bool MMC5603::init(const std::string& i2c_device, uint8_t address, int32_t sensor_id)
{
    uint8_t chip_id;

    m_address = address;
    m_sensor_id = sensor_id;

    m_fd = open(i2c_device.c_str(), O_RDWR);
    if (m_fd < 0)
    {
        std::cerr << "Failed to open I2C device " << i2c_device << ": " << strerror(errno) << std::endl;
        return false;
    }

    if (ioctl(m_fd, I2C_SLAVE, address) < 0)
    {
        std::cerr << "Failed to set I2C slave address 0x" << std::hex << static_cast<int>(address) << ": "
                  << strerror(errno) << std::endl;
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (!read_register(static_cast<uint8_t>(MMC56X3Register::PRODUCT_ID), chip_id))
    {
        std::cerr << "Failed to read chip ID" << std::endl;
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (chip_id != MMC56X3_CHIP_ID && chip_id != 0x00)
    {
        std::cerr << "Invalid chip ID: 0x" << std::hex << static_cast<int>(chip_id) << " (expected 0x"
                  << static_cast<int>(MMC56X3_CHIP_ID) << ")" << std::endl;
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (!reset())
    {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    return true;
}

void MMC5603::close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool MMC5603::reset()
{
    if (m_fd < 0)
    {
        return false;
    }

    if (!write_register(static_cast<uint8_t>(MMC56X3Register::CTRL1_REG), 0x80))
    {
        return false;
    }

    delay_ms(20);

    m_odr_cache = 0;
    m_ctrl2_cache = 0;

    if (!magnet_set_reset())
    {
        return false;
    }

    if (!set_continuous_mode(false))
    {
        return false;
    }

    return true;
}

bool MMC5603::magnet_set_reset()
{
    if (m_fd < 0)
    {
        return false;
    }

    // Turn on SET bit
    if (!write_register(static_cast<uint8_t>(MMC56X3Register::CTRL0_REG), 0x08))
    {
        return false;
    }
    delay_ms(1);

    // Turn on RESET bit
    if (!write_register(static_cast<uint8_t>(MMC56X3Register::CTRL0_REG), 0x10))
    {
        return false;
    }
    delay_ms(1);

    return true;
}

bool MMC5603::set_continuous_mode(bool continuous)
{
    if (m_fd < 0)
    {
        return false;
    }

    if (continuous)
    {
        // Turn on cmm_freq_en bit
        if (!write_register(static_cast<uint8_t>(MMC56X3Register::CTRL0_REG), 0x80))
        {
            return false;
        }
        m_ctrl2_cache |= 0x10; // Turn on cmm_en bit
    }
    else
    {
        m_ctrl2_cache &= ~0x10; // Turn off cmm_en bit
    }

    return write_register(static_cast<uint8_t>(MMC56X3Register::CTRL2_REG), m_ctrl2_cache);
}

bool MMC5603::is_continuous_mode() const
{
    return (m_ctrl2_cache & 0x10) != 0;
}

bool MMC5603::read_temperature(float& temp)
{
    uint8_t status;
    uint8_t temp_data;
    int timeout;

    if (m_fd < 0)
    {
        return false;
    }

    if (is_continuous_mode())
    {
        temp = NAN;
        return false;
    }

    // Trigger temperature measurement
    if (!write_register(static_cast<uint8_t>(MMC56X3Register::CTRL0_REG), 0x02))
    {
        return false;
    }

    // Wait for measurement to complete
    timeout = 1000;
    do
    {
        if (!read_register(static_cast<uint8_t>(MMC56X3Register::STATUS_REG), status))
        {
            return false;
        }
        if (status & 0x80)
        {
            break;
        }
        delay_ms(5);
        timeout -= 5;
    } while (timeout > 0);

    if (timeout <= 0)
    {
        return false;
    }

    if (!read_register(static_cast<uint8_t>(MMC56X3Register::OUT_TEMP), temp_data))
    {
        return false;
    }

    // Convert to Celsius
    temp = static_cast<float>(temp_data) * 0.8f - 75.0f;

    return true;
}

bool MMC5603::read_mag(MagData& data)
{
    uint8_t buffer[9];
    uint8_t status;
    int timeout;

    if (m_fd < 0)
    {
        return false;
    }

    data = {}; // Initialize to zero

    if (!is_continuous_mode())
    {
        if (!write_register(static_cast<uint8_t>(MMC56X3Register::CTRL0_REG), 0x01))
        {
            return false;
        }

        // Wait for measurement to complete
        timeout = 1000;
        do
        {
            if (!read_register(static_cast<uint8_t>(MMC56X3Register::STATUS_REG), status))
            {
                return false;
            }
            if (status & 0x40)
            {
                break;
            }
            delay_ms(5);
            timeout -= 5;
        } while (timeout > 0);

        if (timeout <= 0)
        {
            return false;
        }
    }

    if (!read_registers(static_cast<uint8_t>(MMC56X3Register::OUT_X_L), buffer, 9))
    {
        return false;
    }

    m_raw_x = (static_cast<uint32_t>(buffer[0]) << 12) | (static_cast<uint32_t>(buffer[1]) << 4) |
              (static_cast<uint32_t>(buffer[6]) >> 4);
    m_raw_y = (static_cast<uint32_t>(buffer[2]) << 12) | (static_cast<uint32_t>(buffer[3]) << 4) |
              (static_cast<uint32_t>(buffer[7]) >> 4);
    m_raw_z = (static_cast<uint32_t>(buffer[4]) << 12) | (static_cast<uint32_t>(buffer[5]) << 4) |
              (static_cast<uint32_t>(buffer[8]) >> 4);

    // Apply center offset correction
    m_raw_x -= (1UL << 19);
    m_raw_y -= (1UL << 19);
    m_raw_z -= (1UL << 19);

    // Convert to uTesla (scale factor from datasheet: 0.00625 uT/LSB)
    data.x = static_cast<float>(m_raw_x) * 0.00625f;
    data.y = static_cast<float>(m_raw_y) * 0.00625f;
    data.z = static_cast<float>(m_raw_z) * 0.00625f;
    data.timestamp = get_timestamp_ms();

    return true;
}

bool MMC5603::set_data_rate(uint16_t rate)
{
    if (m_fd < 0)
    {
        return false;
    }

    // Limit rate to valid range
    if (rate > 255)
    {
        rate = 1000;
    }

    m_odr_cache = rate;

    if (rate == 1000)
    {
        // High power mode for 1000 Hz
        if (!write_register(static_cast<uint8_t>(MMC56X3Register::ODR_REG), 255))
        {
            return false;
        }
        m_ctrl2_cache |= 0x80; // Turn on hpower bit
    }
    else
    {
        // Normal mode
        if (!write_register(static_cast<uint8_t>(MMC56X3Register::ODR_REG), static_cast<uint8_t>(rate)))
        {
            return false;
        }
        m_ctrl2_cache &= ~0x80; // Turn off hpower bit
    }

    return write_register(static_cast<uint8_t>(MMC56X3Register::CTRL2_REG), m_ctrl2_cache);
}

uint16_t MMC5603::get_data_rate() const
{
    return m_odr_cache;
}

void MMC5603::get_sensor_info(SensorInfo& info) const
{
    info.name = "MMC5603";
    info.sensor_id = m_sensor_id;
    info.max_value = 3000.0f;   // 30 gauss = 3000 uTesla
    info.min_value = -3000.0f;  // -30 gauss = -3000 uTesla
    info.resolution = 0.00625f; // 20-bit resolution, 0.00625 uT/LSB
}
