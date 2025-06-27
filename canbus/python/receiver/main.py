#!.venv/bin/python3

import signal
import sys

from can_receiver import CanReceiver


def signal_handler(sig, frame):
    print("\nShutting down...")
    sys.exit(0)


def print_usage(program_name: str):
    print(f"Usage: {program_name} [OPTIONS]")
    print("Options:")
    print("  DEVICE    CAN bus interface name")
    print(f"\nExample: {program_name} vcan0")


def main():
    if len(sys.argv) < 2:
        print_usage(sys.argv[0])
        return 1

    interface_name = sys.argv[1]

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    receiver = CanReceiver(interface_name)

    if receiver.initialize():
        print("Failed to initialize CAN receiver", file=sys.stderr)
        return 1

    receiver.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
