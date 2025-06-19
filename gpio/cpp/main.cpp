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

#include "gpio_controller.h"
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

    if (!g_gpio_controller.initialize())
    {
        std::cerr << "Failed to initialize GPIO controller" << std::endl;
        return EXIT_FAILURE;
    }

    g_gpio_controller.run();

    return EXIT_SUCCESS;
}
