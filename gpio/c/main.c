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

#include <gpiod.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Global variables for cleanup
struct gpiod_chip* chip1 = NULL;
struct gpiod_chip* chip2 = NULL;
struct gpiod_line* line1_38 = NULL; // GPIO4 set to active-high output with low value
struct gpiod_line* line1_11 = NULL; // RED LED output GPIO
struct gpiod_line* line1_12 = NULL; // GREEN LED output GPIO
struct gpiod_line* line2_8 = NULL;  // GPIO17 set to input with pull-up resistor enabled (normally high)

void cleanup(void)
{
    printf("\nCleaning up...\n");

    if (line1_38)
        gpiod_line_release(line1_38);
    if (line1_11)
        gpiod_line_release(line1_11);
    if (line1_12)
        gpiod_line_release(line1_12);
    if (line2_8)
        gpiod_line_release(line2_8);

    if (chip1)
        gpiod_chip_close(chip1);
    if (chip2)
        gpiod_chip_close(chip2);
}

void signal_handler(__attribute__((unused)) int sig)
{
    cleanup();
    _exit(128 + sig);
}

int main()
{
    int ret;
    int prev_input_state = 0;
    int current_input_state = 0;
    bool toggle_state = false;

    // Set up cleanup
    atexit(cleanup);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    chip1 = gpiod_chip_open_by_name("gpiochip1");
    if (!chip1)
    {
        fprintf(stderr, "Failed to open gpiochip1\n");
        return EXIT_FAILURE;
    }

    chip2 = gpiod_chip_open_by_name("gpiochip2");
    if (!chip2)
    {
        fprintf(stderr, "Failed to open gpiochip2\n");
        return EXIT_FAILURE;
    }

    line1_38 = gpiod_chip_get_line(chip1, 38);
    line1_11 = gpiod_chip_get_line(chip1, 11);
    line1_12 = gpiod_chip_get_line(chip1, 12);
    line2_8 = gpiod_chip_get_line(chip2, 8);

    if (!line1_38 || !line1_11 || !line1_12 || !line2_8)
    {
        fprintf(stderr, "Failed to get GPIO lines\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-38 as active-high output with value 0
    ret = gpiod_line_request_output(line1_38, "gpio_example", 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-38 as output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-11 as active-low output with value 0
    ret = gpiod_line_request_output_flags(line1_11, "gpio_example", GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-11 as active-low output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-12 as active-high output with value 0
    ret = gpiod_line_request_output(line1_12, "gpio_example", 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-12 as output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip2-8 as pull-up input
    ret = gpiod_line_request_input_flags(line2_8, "gpio_example", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line2-8 as input\n");
        return EXIT_FAILURE;
    }

    printf("GPIO configuration complete:\n");
    printf("- gpiochip1-38 (GPIO4)    : active-high output, value=0\n");
    printf("- gpiochip1-11 (RED LED)  : active-low output , value=0\n");
    printf("- gpiochip1-12 (GREEN LED): active-high output, value=0\n");
    printf("- gpiochip2-8  (GPIO17)   : pull-up input\n");
    printf("\nWaiting for input transitions on GPIO17...\n");
    printf("Press Ctrl+C to exit\n\n");

    // Read initial state of input
    prev_input_state = gpiod_line_get_value(line2_8);
    if (prev_input_state < 0)
    {
        fprintf(stderr, "Failed to read initial input state\n");
        return 1;
    }

    while (1)
    {
        current_input_state = gpiod_line_get_value(line2_8);
        if (current_input_state < 0)
        {
            fprintf(stderr, "Failed to read input state\n");
            break;
        }

        if (prev_input_state == 1 && current_input_state == 0)
        {
            printf("Input transition detected (1->0) - Toggling outputs\n");

            if (!toggle_state)
            {
                ret = gpiod_line_set_value(line1_11, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "Failed to set line1-11\n");
                    break;
                }
                ret = gpiod_line_set_value(line1_12, 0);
                if (ret < 0)
                {
                    fprintf(stderr, "Failed to set line1-12\n");
                    break;
                }
                printf("-> Set gpiochip1-11=HIGH, gpiochip1-12=LOW\n");
            }
            else
            {
                ret = gpiod_line_set_value(line1_11, 0);
                if (ret < 0)
                {
                    fprintf(stderr, "Failed to set line1-11\n");
                    break;
                }
                ret = gpiod_line_set_value(line1_12, 1);
                if (ret < 0)
                {
                    fprintf(stderr, "Failed to set line1-12\n");
                    break;
                }
                printf("-> Set gpiochip1-11=LOW, gpiochip1-12=HIGH\n");
            }

            toggle_state = !toggle_state;
        }

        prev_input_state = current_input_state;

        // Small delay to avoid excessive CPU usage
        usleep(10000);
    }

    return EXIT_SUCCESS;
}
