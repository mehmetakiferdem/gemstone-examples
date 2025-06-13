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

#include <errno.h>
#include <gpiod.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

// Global variables for cleanup
struct gpiod_chip* chip1 = NULL;
struct gpiod_chip* chip2 = NULL;
struct gpiod_line* line_gpio4 = NULL;     // GPIO4 set to active-high output with low value
struct gpiod_line* line_led_red = NULL;   // LED_RED output GPIO
struct gpiod_line* line_led_green = NULL; // LED_GREEN output GPIO
struct gpiod_line* line_gpio17 = NULL;    // GPIO17 set to input with pull-up resistor enabled (normally high)

void cleanup(void)
{
    printf("\nCleaning up...\n");

    if (line_gpio4)
        gpiod_line_release(line_gpio4);
    if (line_led_red)
        gpiod_line_release(line_led_red);
    if (line_led_green)
        gpiod_line_release(line_led_green);
    if (line_gpio17)
        gpiod_line_release(line_gpio17);

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

void delay_ms(int ms)
{
    struct timespec request = {ms / 1000, (ms % 1000) * 1.0E6};
    struct timespec remaining;

    while (thrd_sleep(&request, &remaining) == -1 && errno == EINTR)
    {
        request = remaining; // Sleep again with remaining time if interrupted
    }
}

int main()
{
    int ret;
    int prev_input_state = 0;
    int current_input_state = 0;

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

    line_gpio4 = gpiod_chip_get_line(chip1, 38);
    line_led_red = gpiod_chip_get_line(chip1, 11);
    line_led_green = gpiod_chip_get_line(chip1, 12);
    line_gpio17 = gpiod_chip_get_line(chip2, 8);

    if (!line_gpio4 || !line_led_red || !line_led_green || !line_gpio17)
    {
        fprintf(stderr, "Failed to get GPIO lines\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-38 as active-high output with value 0
    ret = gpiod_line_request_output(line_gpio4, "gpio_example", 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-38 as output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-11 as active-low output with value 0
    ret = gpiod_line_request_output_flags(line_led_red, "gpio_example", GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-11 as active-low output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-12 as active-high output with value 0
    ret = gpiod_line_request_output(line_led_green, "gpio_example", 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-12 as output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip2-8 as pull-up input
    ret = gpiod_line_request_input_flags(line_gpio17, "gpio_example", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line2-8 as input\n");
        return EXIT_FAILURE;
    }

    printf("GPIO configuration complete:\n");
    printf("- gpiochip1-38 (GPIO4)    : active-high output, value=0\n");
    printf("- gpiochip1-11 (LED_RED)  : active-low output , value=0\n");
    printf("- gpiochip1-12 (LED_GREEN): active-high output, value=0\n");
    printf("- gpiochip2-8  (GPIO17)   : pull-up input\n");
    printf("\nWaiting for input transitions on GPIO17...\n");
    printf("Press Ctrl+C to exit\n\n");

    // Read initial state of input
    prev_input_state = gpiod_line_get_value(line_gpio17);
    if (prev_input_state < 0)
    {
        fprintf(stderr, "Failed to read initial input state\n");
        return 1;
    }

    while (1)
    {
        current_input_state = gpiod_line_get_value(line_gpio17);
        if (current_input_state < 0)
        {
            fprintf(stderr, "Failed to read input state\n");
            break;
        }

        if (prev_input_state == 1 && current_input_state == 0)
        {
            ret = gpiod_line_set_value(line_led_red, 1);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_RED\n");
                break;
            }
            ret = gpiod_line_set_value(line_led_green, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_GREEN\n");
                break;
            }
            printf("-> Set LED_RED=HIGH, LED_GREEN=LOW\n");
        }
        else if (prev_input_state == 0 && current_input_state == 1)
        {
            ret = gpiod_line_set_value(line_led_red, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_RED\n");
                break;
            }
            ret = gpiod_line_set_value(line_led_green, 1);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_GREEN\n");
                break;
            }
            printf("-> Set LED_RED=LOW, LED_GREEN=HIGH\n");
        }

        prev_input_state = current_input_state;

        // Small delay to avoid excessive CPU usage
        delay_ms(10);
    }

    return EXIT_SUCCESS;
}
