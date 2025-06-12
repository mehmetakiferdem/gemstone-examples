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
#include <cmath>
#include <csignal>
#include <ctime>
#include <iostream>
#include <threads.h>
#include <unistd.h>

static volatile bool running = true;

void signal_handler([[maybe_unused]] int sig)
{
    running = false;
    std::cout << "\nShutting down..." << std::endl;
}

void delay_ms(int ms)
{
    struct timespec request = {ms / 1000, (ms % 1000) * 1'000'000};
    struct timespec remaining;

    while (thrd_sleep(&request, &remaining) == -1 && errno == EINTR)
    {
        request = remaining; // Sleep again with remaining time if interrupted
    }
}

int main(void)
{
    MMC5603 mag;
    MagData mag_data;
    SensorInfo sensor_info;
    float temperature;

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "MMC5603 Magnetometer Test" << std::endl;
    std::cout << "=========================" << std::endl << std::endl;

    // Initialize the magnetometer
    if (!mag.init("/dev/i2c-3", MMC56X3_DEFAULT_ADDRESS, 12345))
    {
        std::cerr << "Failed to initialize MMC5603 magnetometer" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "MMC5603 magnetometer initialized successfully!" << std::endl;

    // Get sensor information
    mag.get_sensor_info(sensor_info);
    std::cout << "Sensor: " << sensor_info.name << std::endl;
    std::cout << "Range: " << sensor_info.min_value << " to " << sensor_info.max_value << " uTesla" << std::endl;
    std::cout << "Resolution: " << sensor_info.resolution << " uTesla/LSB" << std::endl << std::endl;

    // Set data rate to 100 Hz
    if (!mag.set_data_rate(100))
    {
        std::cerr << "Failed to set data rate" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Data rate set to " << mag.get_data_rate() << " Hz" << std::endl;

    // Read temperature (only works in one-shot mode)
    if (mag.read_temperature(temperature))
    {
        std::cout << "Temperature: " << temperature << "°C" << std::endl << std::endl;
    }
    else
    {
        std::cout << "Temperature reading failed" << std::endl << std::endl;
    }

    // Example 1: One-shot mode readings
    std::cout << "=== ONE-SHOT MODE READINGS ===" << std::endl;
    for (int i = 0; i < 5 && running; i++)
    {
        if (mag.read_mag(mag_data))
        {
            float magnitude = std::sqrt(mag_data.x * mag_data.x + mag_data.y * mag_data.y + mag_data.z * mag_data.z);
            std::cout << "Reading " << (i + 1) << ": X=" << mag_data.x << ", Y=" << mag_data.y << ", Z=" << mag_data.z
                      << " uT (magnitude=" << magnitude << " uT)" << std::endl;
        }
        else
        {
            std::cout << "Failed to read magnetometer data" << std::endl;
        }
        delay_ms(1000);
    }

    if (!running)
    {
        return EXIT_SUCCESS;
    }

    // Example 2: Continuous mode readings
    std::cout << std::endl << "=== CONTINUOUS MODE READINGS ===" << std::endl;
    if (!mag.set_continuous_mode(true))
    {
        std::cerr << "Failed to set continuous mode" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Continuous mode enabled. Reading for 10 seconds..." << std::endl;

    int reading_count = 0;
    uint64_t start_time = 0;

    while (running)
    {
        if (mag.read_mag(mag_data))
        {
            if (start_time == 0)
            {
                start_time = mag_data.timestamp;
            }

            reading_count++;

            // Print every 10th reading to avoid flooding the terminal
            if (reading_count % 10 == 0)
            {
                float magnitude =
                    std::sqrt(mag_data.x * mag_data.x + mag_data.y * mag_data.y + mag_data.z * mag_data.z);
                std::cout << "Time: " << (mag_data.timestamp - start_time) << " ms, X=" << mag_data.x
                          << ", Y=" << mag_data.y << ", Z=" << mag_data.z << " uT (magnitude=" << magnitude << " uT)"
                          << std::endl;
            }

            // Stop after 10 seconds
            if (mag_data.timestamp - start_time > 10000)
            {
                break;
            }
        }
        else
        {
            std::cout << "Failed to read magnetometer data in continuous mode" << std::endl;
            break;
        }

        delay_ms(10); // 10ms delay for ~100Hz reading rate
    }

    std::cout << std::endl << "Total readings: " << reading_count << std::endl;
    std::cout << "Average rate: " << (static_cast<float>(reading_count) / 10.0f) << " Hz" << std::endl;

    // Perform magnetic calibration sequence
    std::cout << std::endl << "=== MAGNETIC CALIBRATION ===" << std::endl;
    std::cout << "Performing magnetic set/reset sequence..." << std::endl;

    // Switch back to one-shot mode for calibration
    if (!mag.set_continuous_mode(false))
    {
        std::cerr << "Failed to set one-shot mode" << std::endl;
    }
    else
    {
        // Perform magnetic set/reset
        if (mag.magnet_set_reset())
        {
            std::cout << "Magnetic set/reset completed successfully" << std::endl;

            // Take a reading after calibration
            if (mag.read_mag(mag_data))
            {
                std::cout << "Post-calibration reading: X=" << mag_data.x << ", Y=" << mag_data.y
                          << ", Z=" << mag_data.z << " uT" << std::endl;
            }
        }
        else
        {
            std::cout << "Failed to perform magnetic set/reset" << std::endl;
        }
    }

    std::cout << std::endl << "Example completed successfully!" << std::endl;

    return EXIT_SUCCESS;
}
