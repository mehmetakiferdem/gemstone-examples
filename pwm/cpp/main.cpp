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

#include "gpio_controller.h"
#include "pwm_sysfs.h"
#include <csignal>
#include <cstdlib>
#include <iostream>

// Global variables
GpioController g_gpio_controller {};

void signal_handler([[maybe_unused]] int sig)
{
    std::cout << "\nShutting down..." << std::endl;
    g_gpio_controller.stop();
}

int main()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    PwmSysfs pwm_gpio18 {2, 0, 1'000'000'000, 500'000'000}; // GPIO18 set to PWM with 1s period, 0.5s duty-cycle

    if (pwm_gpio18.initialize())
    {
        std::cerr << "Failed to initialize pwmchip2/pwm0 as 1s period, 0.5s duty-cycle" << std::endl;
        return EXIT_FAILURE;
    }
    if (pwm_gpio18.set_enable("1"))
    {
        std::cerr << "Failed to enable PWM" << std::endl;
        return EXIT_FAILURE;
    }

    printf("PWM configuration complete:\n");
    printf("- pwmchip2/pwm0 (GPIO18)  : period 1s, duty-cycle 0.5s\n");

    if (g_gpio_controller.initialize())
    {
        std::cerr << "Failed to initialize GPIO controller" << std::endl;
        return EXIT_FAILURE;
    }

    g_gpio_controller.run();

    return EXIT_SUCCESS;
}
