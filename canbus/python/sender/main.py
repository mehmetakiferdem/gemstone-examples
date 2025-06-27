#!.venv/bin/python3

import signal
import sys
from typing import Optional

from can_sender import CanSender

# Global variable for signal handling
g_sender: Optional[CanSender] = None


def signal_handler(sig, frame):
    print("\nShutting down...")
    if g_sender:
        g_sender.stop()


def print_usage(program_name: str):
    print(f"Usage: {program_name} [OPTIONS]")
    print("Options:")
    print("  DEVICE    CAN bus interface name")
    print(f"\nExample: {program_name} vcan0")


def main():
    global g_sender

    if len(sys.argv) < 2:
        print_usage(sys.argv[0])
        return 1

    interface_name = sys.argv[1]
    g_sender = CanSender(interface_name)

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    if g_sender.initialize():
        print("Failed to initialize CAN sender", file=sys.stderr)
        return 1

    g_sender.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
