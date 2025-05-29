#!.venv/bin/python3
"""
Copyright (c) 2025 by T3 Foundation. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0
    https://www.t3gemstone.org/license

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0
"""

import math
import signal
import sys
import time

from MMC5603 import MMC56X3_DEFAULT_ADDRESS, MMC5603

# Global flag for graceful shutdown
running = True


def signal_handler(signum, frame):
    global running
    running = False
    print("\nShutting down...")


def main():
    global running

    # Set up signal handlers for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print("MMC5603 Magnetometer Test")
    print("=========================\n")

    # Initialize the magnetometer
    with MMC5603() as mag:
        # Change this to match your I2C bus number (usually 1 for Raspberry Pi)
        i2c_bus = 3  # Equivalent to /dev/i2c-3 in the C++ version

        if not mag.init(i2c_bus, MMC56X3_DEFAULT_ADDRESS, 12345):
            print("Failed to initialize MMC5603 magnetometer")
            return 1

        print("MMC5603 magnetometer initialized successfully!")

        # Get sensor information
        sensor_info = mag.get_sensor_info()
        print(f"Sensor: {sensor_info.name}")
        print(f"Range: {sensor_info.min_value} to {sensor_info.max_value} uTesla")
        print(f"Resolution: {sensor_info.resolution} uTesla/LSB\n")

        # Set data rate to 100 Hz
        if not mag.set_data_rate(100):
            print("Failed to set data rate")
            return 1
        print(f"Data rate set to {mag.get_data_rate()} Hz")

        # Read temperature (only works in one-shot mode)
        success, temperature = mag.read_temperature()
        if success:
            print(f"Temperature: {temperature:.1f}°C\n")
        else:
            print("Temperature reading failed\n")

        # Example 1: One-shot mode readings
        print("=== ONE-SHOT MODE READINGS ===")
        for i in range(5):
            if not running:
                break

            success, mag_data = mag.read_mag()
            if success:
                magnitude = math.sqrt(mag_data.x**2 + mag_data.y**2 + mag_data.z**2)
                print(
                    f"Reading {i+1}: X={mag_data.x:.3f}, Y={mag_data.y:.3f}, "
                    f"Z={mag_data.z:.3f} uT (magnitude={magnitude:.3f} uT)"
                )
            else:
                print("Failed to read magnetometer data")

            time.sleep(1.0)

        if not running:
            return 0

        # Example 2: Continuous mode readings
        print("\n=== CONTINUOUS MODE READINGS ===")
        if not mag.set_continuous_mode(True):
            print("Failed to set continuous mode")
            return 1

        print("Continuous mode enabled. Reading for 10 seconds...")

        reading_count = 0
        start_time = None

        while running:
            success, mag_data = mag.read_mag()
            if success:
                if start_time is None:
                    start_time = mag_data.timestamp

                reading_count += 1

                # Print every 10th reading to avoid flooding the terminal
                if reading_count % 10 == 0:
                    magnitude = math.sqrt(mag_data.x**2 + mag_data.y**2 + mag_data.z**2)
                    elapsed_time = mag_data.timestamp - start_time
                    print(
                        f"Time: {elapsed_time} ms, X={mag_data.x:.3f}, "
                        f"Y={mag_data.y:.3f}, Z={mag_data.z:.3f} uT "
                        f"(magnitude={magnitude:.3f} uT)"
                    )

                # Stop after 10 seconds
                if mag_data.timestamp - start_time > 10000:
                    break
            else:
                print("Failed to read magnetometer data in continuous mode")
                break

            time.sleep(0.01)  # 10ms delay for ~100Hz reading rate

        print(f"\nTotal readings: {reading_count}")
        if start_time is not None:
            print(f"Average rate: {reading_count / 10.0:.1f} Hz")

        # Perform magnetic calibration sequence
        print("\n=== MAGNETIC CALIBRATION ===")
        print("Performing magnetic set/reset sequence...")

        # Switch back to one-shot mode for calibration
        if not mag.set_continuous_mode(False):
            print("Failed to set one-shot mode")
        else:
            # Perform magnetic set/reset
            if mag.magnet_set_reset():
                print("Magnetic set/reset completed successfully")

                # Take a reading after calibration
                success, mag_data = mag.read_mag()
                if success:
                    print(
                        f"Post-calibration reading: X={mag_data.x:.3f}, " f"Y={mag_data.y:.3f}, Z={mag_data.z:.3f} uT"
                    )
            else:
                print("Failed to perform magnetic set/reset")

        print("\nExample completed successfully!")

    return 0


if __name__ == "__main__":
    sys.exit(main())
