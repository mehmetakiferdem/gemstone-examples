#!.venv/bin/python3

# Copyright (c) 2025 by T3 Foundation. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#     https://docs.t3gemstone.org/en/license
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

import signal
import sys

from gpio_controller import GpioController
from pwm_sysfs import PwmSysfs

# Global variables
g_gpio_controller = GpioController()


def signal_handler(sig, frame):
    global g_gpio_controller

    print("\nShutting down...")
    g_gpio_controller.stop()


def main():
    global g_gpio_controller

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    pwm_sysfs = PwmSysfs(2, 0, 1_000_000_000, 500_000_000)

    if pwm_sysfs.initialize():
        print("Failed to initialize pwmchip2/pwm0 as 1s period, 0.5s duty-cycle", file=sys.stderr)
        return 1

    if pwm_sysfs.set_enable("1"):
        print("Failed to enable PWM", file=sys.stderr)
        return 1

    print("PWM configuration complete:")
    print("- pwmchip2/pwm0 (GPIO18)  : period 1s, duty-cycle 0.5s")

    if g_gpio_controller.initialize():
        print("Failed to initialize GPIO controller", file=sys.stderr)
        return 1

    g_gpio_controller.run()

    return 0


if __name__ == "__main__":
    sys.exit(main())
