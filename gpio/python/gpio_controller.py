# Copyright (c) 2025 by T3 Foundation. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#     https://www.t3gemstone.org/license
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

import sys
import time
from typing import Optional

import gpiod


class GpioController:
    """GPIO Controller class for managing GPIO pins using libgpiod."""

    def __init__(self):
        self.m_chip1: Optional[gpiod.Chip] = None
        self.m_chip2: Optional[gpiod.Chip] = None

        self.m_line1_38: Optional[gpiod.Line] = None  # GPIO4 set to active-high output with low value
        self.m_line1_11: Optional[gpiod.Line] = None  # RED LED output GPIO
        self.m_line1_12: Optional[gpiod.Line] = None  # GREEN LED output GPIO
        self.m_line2_8: Optional[gpiod.Line] = None  # GPIO17 set to input with pull-up resistor enabled

        self.m_prev_input_state: int = 0
        self.m_current_input_state: int = 0
        self.m_toggle_state: bool = False
        self._cleaned_up: bool = False

    def __del__(self):
        self.cleanup()

    def initialize(self) -> bool:
        try:
            self.m_chip1 = gpiod.Chip("gpiochip1")
            self.m_chip2 = gpiod.Chip("gpiochip2")

            self.m_line1_38 = self.m_chip1.get_line(38)
            self.m_line1_11 = self.m_chip1.get_line(11)
            self.m_line1_12 = self.m_chip1.get_line(12)
            self.m_line2_8 = self.m_chip2.get_line(8)

        except Exception as e:
            print(f"Failed to open GPIO chips or get lines: {e}", file=sys.stderr)
            return False

        # Configure outputs and inputs
        if not self._configure_outputs() or not self._configure_inputs():
            return False

        # Read initial input state
        try:
            self.m_prev_input_state = self.m_line2_8.get_value()
        except Exception as e:
            print(f"Failed to read initial input state: {e}", file=sys.stderr)
            return False

        self._print_configuration()
        return True

    def _configure_outputs(self) -> bool:
        try:
            # Configure gpiochip1-38 as active-high output with value 0
            self.m_line1_38.request(consumer="gpio_example", type=gpiod.LINE_REQ_DIR_OUT, default_val=0)

            # Configure gpiochip1-11 as active-low output with value 0
            self.m_line1_11.request(
                consumer="gpio_example",
                type=gpiod.LINE_REQ_DIR_OUT,
                flags=gpiod.LINE_REQ_FLAG_ACTIVE_LOW,
                default_val=0,
            )

            # Configure gpiochip1-12 as active-high output with value 0
            self.m_line1_12.request(consumer="gpio_example", type=gpiod.LINE_REQ_DIR_OUT, default_val=0)

        except Exception as e:
            print(f"Failed to configure output lines: {e}", file=sys.stderr)
            return False

        return True

    def _configure_inputs(self) -> bool:
        try:
            # Configure gpiochip2-8 as pull-up input
            self.m_line2_8.request(
                consumer="gpio_example", type=gpiod.LINE_REQ_DIR_IN, flags=gpiod.LINE_REQ_FLAG_BIAS_PULL_UP
            )
        except Exception as e:
            print(f"Failed to configure input line: {e}", file=sys.stderr)
            return False

        return True

    def _print_configuration(self) -> None:
        print("GPIO configuration complete:")
        print("- gpiochip1-38 (GPIO4)    : active-high output, value=0")
        print("- gpiochip1-11 (RED LED)  : active-low output , value=0")
        print("- gpiochip1-12 (GREEN LED): active-high output, value=0")
        print("- gpiochip2-8  (GPIO17)   : pull-up input")
        print()
        print("Waiting for input transitions on GPIO17...")
        print("Press Ctrl+C to exit")
        print()

    def _handle_input_transition(self) -> bool:
        try:
            if not self.m_toggle_state:
                self.m_line1_11.set_value(1)
                self.m_line1_12.set_value(0)
                print("-> Set gpiochip1-11=HIGH, gpiochip1-12=LOW")
            else:
                self.m_line1_11.set_value(0)
                self.m_line1_12.set_value(1)
                print("-> Set gpiochip1-11=LOW, gpiochip1-12=HIGH")

            self.m_toggle_state = not self.m_toggle_state

        except Exception as e:
            print(f"Failed to set output values: {e}", file=sys.stderr)
            return False

        return True

    def run(self) -> None:
        try:
            while True:
                try:
                    self.m_current_input_state = self.m_line2_8.get_value()
                except Exception as e:
                    print(f"Failed to read input state: {e}", file=sys.stderr)
                    break

                # Detect falling edge (1->0 transition)
                if self.m_prev_input_state == 1 and self.m_current_input_state == 0:
                    print("Input transition detected (1->0) - Toggling outputs")
                    if not self._handle_input_transition():
                        break

                self.m_prev_input_state = self.m_current_input_state

                # Small delay to avoid excessive CPU usage
                time.sleep(0.01)

        except KeyboardInterrupt:
            print("\nReceived interrupt signal")
        except Exception as e:
            print(f"Error in main loop: {e}", file=sys.stderr)

    def cleanup(self) -> None:
        if self._cleaned_up:
            return

        print("\nCleaning up...")

        self._cleaned_up = True

        if self.m_line1_38 and self.m_line1_38.is_requested():
            self.m_line1_38.release()
            self.m_line1_38 = None

        if self.m_line1_11 and self.m_line1_11.is_requested():
            self.m_line1_11.release()
            self.m_line1_11 = None

        if self.m_line1_12 and self.m_line1_12.is_requested():
            self.m_line1_12.release()
            self.m_line1_12 = None

        if self.m_line2_8 and self.m_line2_8.is_requested():
            self.m_line2_8.release()
            self.m_line2_8 = None

        if self.m_chip1:
            self.m_chip1.close()
            self.m_chip1 = None

        if self.m_chip2:
            self.m_chip2.close()
            self.m_chip2 = None
