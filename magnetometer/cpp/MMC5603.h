// Copyright (c) 2025 by T3 Foundation. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//     https://docs.t3gemstone.org/en/license
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MMC5603_H
#define MMC5603_H

#include <cstdint>
#include <string>

constexpr uint8_t MMC56X3_DEFAULT_ADDRESS = 0x30;
constexpr uint8_t MMC56X3_CHIP_ID = 0x10;

enum class MMC56X3Register : uint8_t
{
    PRODUCT_ID = 0x39,
    CTRL0_REG = 0x1B,
    CTRL1_REG = 0x1C,
    CTRL2_REG = 0x1D,
    STATUS_REG = 0x18,
    OUT_TEMP = 0x09,
    OUT_X_L = 0x00,
    ODR_REG = 0x1A,
};

struct MagData
{
    float x {};            //!< X-axis magnetic field in uTesla
    float y {};            //!< Y-axis magnetic field in uTesla
    float z {};            //!< Z-axis magnetic field in uTesla
    uint64_t timestamp {}; //!< Timestamp in milliseconds
};

struct SensorInfo
{
    std::string name {};  //!< Sensor name
    int32_t sensor_id {}; //!< Sensor ID
    float max_value {};   //!< Maximum value in uTesla
    float min_value {};   //!< Minimum value in uTesla
    float resolution {};  //!< Resolution in uTesla/LSB
};

class MMC5603
{
  public:
    MMC5603();
    ~MMC5603();

    bool init(const std::string& i2c_device, uint8_t address = MMC56X3_DEFAULT_ADDRESS, int32_t sensor_id = 12345);
    void close();
    bool reset();
    bool magnet_set_reset();
    bool set_continuous_mode(bool continuous);
    bool is_continuous_mode() const;
    bool read_mag(MagData& data);
    bool read_temperature(float& temp);
    bool set_data_rate(uint16_t rate);
    uint16_t get_data_rate() const;
    void get_sensor_info(SensorInfo& info) const;

  private:
    int m_fd {-1};            //!< I2C file descriptor
    uint8_t m_address {};     //!< I2C device address
    int32_t m_sensor_id {};   //!< Sensor ID
    uint16_t m_odr_cache {};  //!< Cached output data rate
    uint8_t m_ctrl2_cache {}; //!< Cached control register 2 value
    int32_t m_raw_x {};       //!< Raw X-axis data
    int32_t m_raw_y {};       //!< Raw Y-axis data
    int32_t m_raw_z {};       //!< Raw Z-axis data

    // Helper methods
    static uint64_t get_timestamp_ms();
    static void delay_ms(int ms);
    bool read_register(uint8_t reg, uint8_t& value);
    bool write_register(uint8_t reg, uint8_t value);
    bool read_registers(uint8_t reg, uint8_t* buffer, int length);
};

#endif /* MMC5603_H */
