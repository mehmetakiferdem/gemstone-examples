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

#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include <gpiod.h>

class GpioController
{
  public:
    GpioController();
    ~GpioController();

    GpioController(const GpioController&) = delete;
    GpioController& operator=(const GpioController&) = delete;

    bool initialize();
    void run();
    void cleanup();

  private:
    struct gpiod_chip* m_chip1 {};
    struct gpiod_chip* m_chip2 {};

    struct gpiod_line* m_line_gpio4 {};     // GPIO4 set to active-high output with low value
    struct gpiod_line* m_line_led_red {};   // LED_RED output GPIO
    struct gpiod_line* m_line_led_green {}; // LED_GREEN output GPIO
    struct gpiod_line* m_line_gpio17 {};    // GPIO17 set to input with pull-up resistor enabled (normally high)

    int m_prev_input_state {};
    int m_current_input_state {};

    bool configure_outputs();
    bool configure_inputs();
    void print_configuration();
    static void delay_ms(int ms);
};

#endif // GPIO_CONTROLLER_H
