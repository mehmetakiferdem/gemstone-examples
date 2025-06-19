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

#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#define PWM_SYSFS_BASE "/sys/class/pwm"

typedef struct
{
    uint8_t chip_no;
    uint8_t channel_no;
    uint32_t period_ns;
    uint32_t duty_cycle_ns;
} pwm_sysfs_t;

// Global variables
pwm_sysfs_t g_pwm_gpio18 = {2, 0, 1E9, 5E8}; // GPIO18 set to PWM with 1s period, 0.5s duty-cycle
struct gpiod_chip* g_chip1 = NULL;
struct gpiod_chip* g_chip2 = NULL;
struct gpiod_line* g_line_led_red = NULL;   // LED_RED output GPIO
struct gpiod_line* g_line_led_green = NULL; // LED_GREEN output GPIO
struct gpiod_line* g_line_gpio17 = NULL;    // GPIO17 set to input with pull-up resistor enabled (normally high)

int write_to_file(const char* path, const char* value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    if (write(fd, value, strlen(value)) < 0)
    {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int pwm_sysfs_initialize(const pwm_sysfs_t* pwm)
{
    char path[256];
    char value[256];

    snprintf(path, sizeof(path), "%s/pwmchip%d/export", PWM_SYSFS_BASE, pwm->chip_no);
    snprintf(value, sizeof(value), "%d", pwm->channel_no);
    if (write_to_file(path, value) < 0)
    {
        printf("Note: PWM channel might already be exported\n");
    }

    snprintf(path, sizeof(path), "%s/pwmchip%d/pwm%d/period", PWM_SYSFS_BASE, pwm->chip_no, pwm->channel_no);
    snprintf(value, sizeof(value), "%d", pwm->period_ns);
    if (write_to_file(path, value) < 0)
    {
        fprintf(stderr, "Failed to set PWM period\n");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/pwmchip%d/pwm%d/duty_cycle", PWM_SYSFS_BASE, pwm->chip_no, pwm->channel_no);
    snprintf(value, sizeof(value), "%d", pwm->duty_cycle_ns);
    if (write_to_file(path, value) < 0)
    {
        fprintf(stderr, "Failed to set PWM duty cycle\n");
        return 1;
    }

    return 0;
}

int pwm_sysfs_set_enable(const pwm_sysfs_t* pwm, const char* value)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/pwmchip%d/pwm%d/enable", PWM_SYSFS_BASE, pwm->chip_no, pwm->channel_no);
    if (write_to_file(path, value) < 0)
    {
        return 1;
    }
    return 0;
}

void cleanup(void)
{
    pwm_sysfs_set_enable(&g_pwm_gpio18, "0");

    if (g_line_led_red)
        gpiod_line_release(g_line_led_red);
    if (g_line_led_green)
        gpiod_line_release(g_line_led_green);
    if (g_line_gpio17)
        gpiod_line_release(g_line_gpio17);

    if (g_chip1)
        gpiod_chip_close(g_chip1);
    if (g_chip2)
        gpiod_chip_close(g_chip2);
}

void signal_handler(__attribute__((unused)) int sig)
{
    printf("\nShutting down...\n");
    exit(0);
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

    g_chip1 = gpiod_chip_open_by_name("gpiochip1");
    if (!g_chip1)
    {
        fprintf(stderr, "Failed to open gpiochip1\n");
        return EXIT_FAILURE;
    }

    g_chip2 = gpiod_chip_open_by_name("gpiochip2");
    if (!g_chip2)
    {
        fprintf(stderr, "Failed to open gpiochip2\n");
        return EXIT_FAILURE;
    }

    g_line_led_red = gpiod_chip_get_line(g_chip1, 11);
    g_line_led_green = gpiod_chip_get_line(g_chip1, 12);
    g_line_gpio17 = gpiod_chip_get_line(g_chip2, 8);

    if (!g_line_led_red || !g_line_led_green || !g_line_gpio17)
    {
        fprintf(stderr, "Failed to get GPIO lines\n");
        return EXIT_FAILURE;
    }

    ret = pwm_sysfs_initialize(&g_pwm_gpio18);
    if (ret)
    {
        fprintf(stderr, "Failed to initialize pwmchip2/pwm0 as 1s period, 0.5s duty-cycle\n");
        return EXIT_FAILURE;
    }

    ret = pwm_sysfs_set_enable(&g_pwm_gpio18, "1");
    if (ret)
    {
        fprintf(stderr, "Failed to enable PWM\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-11 as active-low output with value 0
    ret = gpiod_line_request_output_flags(g_line_led_red, "gpio_example", GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-11 as active-low output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip1-12 as active-high output with value 0
    ret = gpiod_line_request_output(g_line_led_green, "gpio_example", 0);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line1-12 as output\n");
        return EXIT_FAILURE;
    }

    // Configure gpiochip2-8 as pull-up input
    ret = gpiod_line_request_input_flags(g_line_gpio17, "gpio_example", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to configure line2-8 as input\n");
        return EXIT_FAILURE;
    }

    printf("PWM configuration complete:\n");
    printf("- pwmchip2/pwm0 (GPIO18)  : period 1s, duty-cycle 0.5s\n");
    printf("GPIO configuration complete:\n");
    printf("- gpiochip1-11 (LED_RED)  : active-low output , value=0\n");
    printf("- gpiochip1-12 (LED_GREEN): active-high output, value=0\n");
    printf("- gpiochip2-8  (GPIO17)   : pull-up input\n");
    printf("\nWaiting for input transitions on GPIO17...\n");
    printf("Press Ctrl+C to exit\n\n");

    // Read initial state of input
    prev_input_state = gpiod_line_get_value(g_line_gpio17);
    if (prev_input_state < 0)
    {
        fprintf(stderr, "Failed to read initial input state\n");
        return EXIT_FAILURE;
    }

    while (1)
    {
        current_input_state = gpiod_line_get_value(g_line_gpio17);
        if (current_input_state < 0)
        {
            fprintf(stderr, "Failed to read input state\n");
            break;
        }

        if (prev_input_state == 1 && current_input_state == 0)
        {
            ret = gpiod_line_set_value(g_line_led_red, 1);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_RED\n");
                break;
            }
            ret = gpiod_line_set_value(g_line_led_green, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_GREEN\n");
                break;
            }
            printf("-> Set LED_RED=HIGH, LED_GREEN=LOW\n");
        }
        else if (prev_input_state == 0 && current_input_state == 1)
        {
            ret = gpiod_line_set_value(g_line_led_red, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set LED_RED\n");
                break;
            }
            ret = gpiod_line_set_value(g_line_led_green, 1);
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
