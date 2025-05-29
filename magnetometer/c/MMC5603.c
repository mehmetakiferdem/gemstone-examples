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
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

static uint64_t get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

static void delay_ms(int ms)
{
    struct timespec request = {.tv_sec = 0, .tv_nsec = ms * 1.0E6};
    struct timespec remaining;
    thrd_sleep(&request, &remaining);
}

static bool read_register(mmc5603_t* dev, uint8_t reg, uint8_t* value)
{
    if (write(dev->fd, &reg, 1) != 1)
    {
        return false;
    }

    if (read(dev->fd, value, 1) != 1)
    {
        return false;
    }

    return true;
}

static bool write_register(mmc5603_t* dev, uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};

    if (write(dev->fd, buffer, 2) != 2)
    {
        return false;
    }

    return true;
}

static bool read_registers(mmc5603_t* dev, uint8_t reg, uint8_t* buffer, int length)
{
    if (write(dev->fd, &reg, 1) != 1)
    {
        return false;
    }

    if (read(dev->fd, buffer, length) != length)
    {
        return false;
    }

    return true;
}

bool mmc5603_init(mmc5603_t* dev, const char* i2c_device, uint8_t address, int32_t sensor_id)
{
    uint8_t chip_id;

    if (!dev)
    {
        return false;
    }

    memset(dev, 0, sizeof(mmc5603_t));
    dev->address = address;
    dev->sensor_id = sensor_id;

    dev->fd = open(i2c_device, O_RDWR);
    if (dev->fd < 0)
    {
        fprintf(stderr, "Failed to open I2C device %s: %s\n", i2c_device, strerror(errno));
        return false;
    }

    if (ioctl(dev->fd, I2C_SLAVE, address) < 0)
    {
        fprintf(stderr, "Failed to set I2C slave address 0x%02X: %s\n", address, strerror(errno));
        close(dev->fd);
        return false;
    }

    if (!read_register(dev, MMC56X3_PRODUCT_ID, &chip_id))
    {
        fprintf(stderr, "Failed to read chip ID\n");
        close(dev->fd);
        return false;
    }

    if (chip_id != MMC56X3_CHIP_ID && chip_id != 0x00)
    {
        fprintf(stderr, "Invalid chip ID: 0x%02X (expected 0x%02X)\n", chip_id, MMC56X3_CHIP_ID);
        close(dev->fd);
        return false;
    }

    if (!mmc5603_reset(dev))
    {
        close(dev->fd);
        return false;
    }

    return true;
}

void mmc5603_close(mmc5603_t* dev)
{
    if (dev && dev->fd >= 0)
    {
        close(dev->fd);
        dev->fd = -1;
    }
}

bool mmc5603_reset(mmc5603_t* dev)
{
    if (!dev || dev->fd < 0)
    {
        return false;
    }

    if (!write_register(dev, MMC56X3_CTRL1_REG, 0x80))
    {
        return false;
    }

    delay_ms(20);

    dev->odr_cache = 0;
    dev->ctrl2_cache = 0;

    if (!mmc5603_magnet_set_reset(dev))
    {
        return false;
    }

    if (!mmc5603_set_continuous_mode(dev, false))
    {
        return false;
    }

    return true;
}

bool mmc5603_magnet_set_reset(mmc5603_t* dev)
{
    if (!dev || dev->fd < 0)
    {
        return false;
    }

    // Turn on SET bit
    if (!write_register(dev, MMC56X3_CTRL0_REG, 0x08))
    {
        return false;
    }
    delay_ms(1);

    // Turn on RESET bit
    if (!write_register(dev, MMC56X3_CTRL0_REG, 0x10))
    {
        return false;
    }
    delay_ms(1);

    return true;
}

bool mmc5603_set_continuous_mode(mmc5603_t* dev, bool continuous)
{
    if (!dev || dev->fd < 0)
    {
        return false;
    }

    if (continuous)
    {
        // Turn on cmm_freq_en bit
        if (!write_register(dev, MMC56X3_CTRL0_REG, 0x80))
        {
            return false;
        }
        dev->ctrl2_cache |= 0x10; // Turn on cmm_en bit
    }
    else
    {
        dev->ctrl2_cache &= ~0x10; // Turn off cmm_en bit
    }

    return write_register(dev, MMC56X3_CTRL2_REG, dev->ctrl2_cache);
}

bool mmc5603_is_continuous_mode(mmc5603_t* dev)
{
    if (!dev)
    {
        return false;
    }
    return (dev->ctrl2_cache & 0x10) != 0;
}

bool mmc5603_read_temperature(mmc5603_t* dev, float* temp)
{
    uint8_t status;
    uint8_t temp_data;
    int timeout;

    if (!dev || dev->fd < 0 || !temp)
    {
        return false;
    }

    if (mmc5603_is_continuous_mode(dev))
    {
        *temp = NAN;
        return false;
    }

    // Trigger temperature measurement
    if (!write_register(dev, MMC56X3_CTRL0_REG, 0x02))
    {
        return false;
    }

    // Wait for measurement to complete
    timeout = 1000;
    do
    {
        if (!read_register(dev, MMC56X3_STATUS_REG, &status))
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

    if (!read_register(dev, MMC56X3_OUT_TEMP, &temp_data))
    {
        return false;
    }

    // Convert to Celsius
    *temp = (float)temp_data * 0.8f - 75.0f;

    return true;
}

bool mmc5603_read_mag(mmc5603_t* dev, mmc5603_mag_data_t* data)
{
    uint8_t buffer[9];
    uint8_t status;
    int timeout;

    if (!dev || dev->fd < 0 || !data)
    {
        return false;
    }

    memset(data, 0, sizeof(mmc5603_mag_data_t));

    if (!mmc5603_is_continuous_mode(dev))
    {
        if (!write_register(dev, MMC56X3_CTRL0_REG, 0x01))
        {
            return false;
        }

        // Wait for measurement to complete
        timeout = 1000;
        do
        {
            if (!read_register(dev, MMC56X3_STATUS_REG, &status))
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

    if (!read_registers(dev, MMC56X3_OUT_X_L, buffer, 9))
    {
        return false;
    }

    dev->raw_x = ((uint32_t)buffer[0] << 12) | ((uint32_t)buffer[1] << 4) | ((uint32_t)buffer[6] >> 4);
    dev->raw_y = ((uint32_t)buffer[2] << 12) | ((uint32_t)buffer[3] << 4) | ((uint32_t)buffer[7] >> 4);
    dev->raw_z = ((uint32_t)buffer[4] << 12) | ((uint32_t)buffer[5] << 4) | ((uint32_t)buffer[8] >> 4);

    // Apply center offset correction
    dev->raw_x -= (1UL << 19);
    dev->raw_y -= (1UL << 19);
    dev->raw_z -= (1UL << 19);

    // Convert to uTesla (scale factor from datasheet: 0.00625 uT/LSB)
    data->x = (float)dev->raw_x * 0.00625f;
    data->y = (float)dev->raw_y * 0.00625f;
    data->z = (float)dev->raw_z * 0.00625f;
    data->timestamp = get_timestamp_ms();

    return true;
}

bool mmc5603_set_data_rate(mmc5603_t* dev, uint16_t rate)
{
    if (!dev || dev->fd < 0)
    {
        return false;
    }

    // Limit rate to valid range
    if (rate > 255)
    {
        rate = 1000;
    }

    dev->odr_cache = rate;

    if (rate == 1000)
    {
        // High power mode for 1000 Hz
        if (!write_register(dev, MMC5603_ODR_REG, 255))
        {
            return false;
        }
        dev->ctrl2_cache |= 0x80; // Turn on hpower bit
    }
    else
    {
        // Normal mode
        if (!write_register(dev, MMC5603_ODR_REG, (uint8_t)rate))
        {
            return false;
        }
        dev->ctrl2_cache &= ~0x80; // Turn off hpower bit
    }

    return write_register(dev, MMC56X3_CTRL2_REG, dev->ctrl2_cache);
}

uint16_t mmc5603_get_data_rate(mmc5603_t* dev)
{
    if (!dev)
    {
        return 0;
    }
    return dev->odr_cache;
}

void mmc5603_get_sensor_info(mmc5603_t* dev, mmc5603_sensor_info_t* info)
{
    if (!dev || !info)
    {
        return;
    }

    memset(info, 0, sizeof(mmc5603_sensor_info_t));

    strncpy(info->name, "MMC5603", sizeof(info->name) - 1);
    info->sensor_id = dev->sensor_id;
    info->max_value = 3000.0f;   // 30 gauss = 3000 uTesla
    info->min_value = -3000.0f;  // -30 gauss = -3000 uTesla
    info->resolution = 0.00625f; // 20-bit resolution, 0.00625 uT/LSB
}
