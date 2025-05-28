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
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static volatile bool running = true;

void signal_handler(__attribute__((unused)) int sig)
{
    running = false;
    printf("\nShutting down...\n");
}

void delay_ms(int ms)
{
    struct timespec request = {0, ms * 1.0E6};
    struct timespec remaining;
    nanosleep(&request, &remaining);
}

int main(void)
{
    mmc5603_t mag;
    mmc5603_mag_data_t mag_data;
    mmc5603_sensor_info_t sensor_info;
    float temperature;

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("MMC5603 Magnetometer Test\n");
    printf("=========================\n\n");

    // Initialize the magnetometer
    if (!mmc5603_init(&mag, "/dev/i2c-3", MMC56X3_DEFAULT_ADDRESS, 12345))
    {
        fprintf(stderr, "Failed to initialize MMC5603 magnetometer\n");
        return EXIT_FAILURE;
    }

    printf("MMC5603 magnetometer initialized successfully!\n");

    // Get sensor information
    mmc5603_get_sensor_info(&mag, &sensor_info);
    printf("Sensor: %s\n", sensor_info.name);
    printf("Range: %.1f to %.1f uTesla\n", sensor_info.min_value, sensor_info.max_value);
    printf("Resolution: %.5f uTesla/LSB\n\n", sensor_info.resolution);

    // Set data rate to 100 Hz
    if (!mmc5603_set_data_rate(&mag, 100))
    {
        fprintf(stderr, "Failed to set data rate\n");
        mmc5603_close(&mag);
        return EXIT_FAILURE;
    }
    printf("Data rate set to %d Hz\n", mmc5603_get_data_rate(&mag));

    // Read temperature (only works in one-shot mode)
    if (mmc5603_read_temperature(&mag, &temperature))
    {
        printf("Temperature: %.1f°C\n\n", temperature);
    }
    else
    {
        printf("Temperature reading failed\n\n");
    }

    // Example 1: One-shot mode readings
    printf("=== ONE-SHOT MODE READINGS ===\n");
    for (int i = 0; i < 5 && running; i++)
    {
        if (mmc5603_read_mag(&mag, &mag_data))
        {
            printf("Reading %d: X=%.3f, Y=%.3f, Z=%.3f uT (magnitude=%.3f uT)\n", i + 1, mag_data.x, mag_data.y,
                   mag_data.z, sqrtf(mag_data.x * mag_data.x + mag_data.y * mag_data.y + mag_data.z * mag_data.z));
        }
        else
        {
            printf("Failed to read magnetometer data\n");
        }
        delay_ms(1000);
    }

    if (!running)
    {
        mmc5603_close(&mag);
        return EXIT_SUCCESS;
    }

    // Example 2: Continuous mode readings
    printf("\n=== CONTINUOUS MODE READINGS ===\n");
    if (!mmc5603_set_continuous_mode(&mag, true))
    {
        fprintf(stderr, "Failed to set continuous mode\n");
        mmc5603_close(&mag);
        return EXIT_FAILURE;
    }

    printf("Continuous mode enabled. Reading for 10 seconds...\n");

    int reading_count = 0;
    uint64_t start_time = 0;

    while (running)
    {
        if (mmc5603_read_mag(&mag, &mag_data))
        {
            if (start_time == 0)
            {
                start_time = mag_data.timestamp;
            }

            reading_count++;

            // Print every 10th reading to avoid flooding the terminal
            if (reading_count % 10 == 0)
            {
                printf("Time: %llu ms, X=%.3f, Y=%.3f, Z=%.3f uT (magnitude=%.3f uT)\n",
                       (unsigned long long)(mag_data.timestamp - start_time), mag_data.x, mag_data.y, mag_data.z,
                       sqrtf(mag_data.x * mag_data.x + mag_data.y * mag_data.y + mag_data.z * mag_data.z));
            }

            // Stop after 10 seconds
            if (mag_data.timestamp - start_time > 10000)
            {
                break;
            }
        }
        else
        {
            printf("Failed to read magnetometer data in continuous mode\n");
            break;
        }

        delay_ms(10); // 10ms delay for ~100Hz reading rate
    }

    printf("\nTotal readings: %d\n", reading_count);
    printf("Average rate: %.1f Hz\n", (float)reading_count / 10.0f);

    // Perform magnetic calibration sequence
    printf("\n=== MAGNETIC CALIBRATION ===\n");
    printf("Performing magnetic set/reset sequence...\n");

    // Switch back to one-shot mode for calibration
    if (!mmc5603_set_continuous_mode(&mag, false))
    {
        fprintf(stderr, "Failed to set one-shot mode\n");
    }
    else
    {
        // Perform magnetic set/reset
        if (mmc5603_magnet_set_reset(&mag))
        {
            printf("Magnetic set/reset completed successfully\n");

            // Take a reading after calibration
            if (mmc5603_read_mag(&mag, &mag_data))
            {
                printf("Post-calibration reading: X=%.3f, Y=%.3f, Z=%.3f uT\n", mag_data.x, mag_data.y, mag_data.z);
            }
        }
        else
        {
            printf("Failed to perform magnetic set/reset\n");
        }
    }

    // Clean up
    mmc5603_close(&mag);
    printf("\nExample completed successfully!\n");

    return EXIT_SUCCESS;
}
