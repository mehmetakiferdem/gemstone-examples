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

#include "icm20948.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

// Global variables
static volatile bool g_is_running = true;

void signal_handler(__attribute__((unused)) int sig)
{
    printf("\nShutting down...\n");
    g_is_running = false;
}

void delay_ms(int ms)
{
    struct timespec request = {ms / 1000, (ms % 1000) * 1.0E6};
    struct timespec remaining;

    while (thrd_sleep(&request, &remaining) == -1 && errno == EINTR)
    {
        request = remaining; // Sleep again with remaining time if interrupted
    }
}

const char* acce_fs_t_to_str(icm20948_acce_fs_t acce_fs)
{
    switch (acce_fs)
    {
    case ACCE_FS_2G:
        return "2g";
    case ACCE_FS_4G:
        return "4g";
    case ACCE_FS_8G:
        return "8g";
    case ACCE_FS_16G:
        return "16g";
    default:
        return "";
    }
}

const char* gyro_fs_t_to_str(icm20948_gyro_fs_t gyro_fs)
{
    switch (gyro_fs)
    {
    case GYRO_FS_250DPS:
        return "250DPS";
    case GYRO_FS_500DPS:
        return "500DPS";
    case GYRO_FS_1000DPS:
        return "1000DPS";
    case GYRO_FS_2000DPS:
        return "2000DPS";
    default:
        return "";
    }
}

int main()
{
    icm20948_data_t icm20948_data;
    char dev_name[] = "icm20948";
    const char* spi_dev_path = "/dev/spidev0.3";
    icm20948_handle_t icm20948 = icm20948_create(&icm20948_data, dev_name);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("ICM-20948 IMU Accel/Gyro/Temp Test\n");
    printf("==================================\n\n");

    if (icm20948_spi_bus_init(icm20948, spi_dev_path) != 0)
    {
        fprintf(stderr, "Failed to initialize ICM-20948 IMU with SPI bus, %s\n", spi_dev_path);
        icm20948_delete(icm20948);
        return EXIT_FAILURE;
    }

    if (icm20948_configure(icm20948, ACCE_FS_8G, GYRO_FS_2000DPS) != 0)
    {
        fprintf(stderr, "Failed to configure ICM-20948 IMU\n");
        icm20948_delete(icm20948);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 100; i++)
    {
        delay_ms(1);
        icm20948_get_temp(icm20948);
    }

    printf("\n");
    printf("Temperature:            %-8.2f\n", icm20948_data.temp);
    printf("Accel Sensitivity:      %-8.2f\n", icm20948_get_acce_sensitivity(icm20948));
    printf("Accel Full Scale Range: %s (+/-)\n", acce_fs_t_to_str(icm20948_get_acce_fs(icm20948)));
    printf("Gyro Sensitivity:       %-8.2f\n", icm20948_get_gyro_sensitivity(icm20948));
    printf("Gyro Full Scale Range:  %s (+/-)\n", gyro_fs_t_to_str(icm20948_get_gyro_fs(icm20948)));
    printf("\nContinuous test will begin shortly. Press Ctrl+C to exit.\n");
    sleep(3);

    while (g_is_running)
    {
        icm20948_get_acce(icm20948);
        icm20948_get_gyro(icm20948);
        icm20948_get_angle(icm20948, 0.01f); // dt 10ms

        printf("Angle: x=%7.2f, y=%7.2f, z=%7.2f\n", icm20948_data.anglex, icm20948_data.angley, icm20948_data.anglez);

        if (icm20948_check_online(icm20948) != 0)
        {
            icm20948_configure(icm20948, ACCE_FS_8G, GYRO_FS_2000DPS);
        }

        delay_ms(10); // 10ms
    }

    // Clean up
    icm20948_delete(icm20948);

    return EXIT_SUCCESS;
}
