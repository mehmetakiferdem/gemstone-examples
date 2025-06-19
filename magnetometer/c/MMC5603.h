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

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define MMC56X3_DEFAULT_ADDRESS 0x30
#define MMC56X3_CHIP_ID 0x10

typedef enum
{
    MMC56X3_PRODUCT_ID = 0x39,
    MMC56X3_CTRL0_REG = 0x1B,
    MMC56X3_CTRL1_REG = 0x1C,
    MMC56X3_CTRL2_REG = 0x1D,
    MMC56X3_STATUS_REG = 0x18,
    MMC56X3_OUT_TEMP = 0x09,
    MMC56X3_OUT_X_L = 0x00,
    MMC5603_ODR_REG = 0x1A,
} mmc56x3_register_t;

typedef struct
{
    float x;            //!< X-axis magnetic field in uTesla
    float y;            //!< Y-axis magnetic field in uTesla
    float z;            //!< Z-axis magnetic field in uTesla
    uint64_t timestamp; //!< Timestamp in milliseconds
} mmc5603_mag_data_t;

typedef struct
{
    char name[32];     //!< Sensor name
    int32_t sensor_id; //!< Sensor ID
    float max_value;   //!< Maximum value in uTesla
    float min_value;   //!< Minimum value in uTesla
    float resolution;  //!< Resolution in uTesla/LSB
} mmc5603_sensor_info_t;

typedef struct
{
    int fd;              //!< I2C file descriptor
    uint8_t address;     //!< I2C device address
    int32_t sensor_id;   //!< Sensor ID
    uint16_t odr_cache;  //!< Cached output data rate
    uint8_t ctrl2_cache; //!< Cached control register 2 value
    int32_t raw_x;       //!< Raw X-axis data
    int32_t raw_y;       //!< Raw Y-axis data
    int32_t raw_z;       //!< Raw Z-axis data
} mmc5603_t;

bool mmc5603_init(mmc5603_t* dev, const char* i2c_device, uint8_t address, int32_t sensor_id);
void mmc5603_close(mmc5603_t* dev);
bool mmc5603_reset(mmc5603_t* dev);
bool mmc5603_magnet_set_reset(mmc5603_t* dev);
bool mmc5603_set_continuous_mode(mmc5603_t* dev, bool continuous);
bool mmc5603_is_continuous_mode(mmc5603_t* dev);
bool mmc5603_read_mag(mmc5603_t* dev, mmc5603_mag_data_t* data);
bool mmc5603_read_temperature(mmc5603_t* dev, float* temp);
bool mmc5603_set_data_rate(mmc5603_t* dev, uint16_t rate);
uint16_t mmc5603_get_data_rate(mmc5603_t* dev);
void mmc5603_get_sensor_info(mmc5603_t* dev, mmc5603_sensor_info_t* info);

#endif /* MMC5603_H */
