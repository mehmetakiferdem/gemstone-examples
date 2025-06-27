#include "can_receiver.h"
#include <iostream>
#include <signal.h>

void signal_handler([[maybe_unused]] int sig)
{
    std::cout << "\nShutting down..." << std::endl;
    exit(0);
}

void print_usage(std::string_view program_name)
{
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  DEVICE    CAN bus interface name" << std::endl;
    std::cout << std::endl << "Example: " << program_name << " vcan0" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string_view interface_name = argv[1];

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    CanReceiver receiver {interface_name};

    if (receiver.initialize())
    {
        std::cerr << "Failed to initialize CAN receiver" << std::endl;
        return EXIT_FAILURE;
    }

    receiver.run();

    return EXIT_SUCCESS;
}
