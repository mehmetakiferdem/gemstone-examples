#include "can_sender.h"
#include <iostream>
#include <memory>
#include <signal.h>

// Global variables
static std::unique_ptr<CanSender> g_sender;

void signal_handler([[maybe_unused]] int sig)
{
    std::cout << "\nShutting down..." << std::endl;
    if (g_sender)
    {
        g_sender->stop();
    }
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
    g_sender = std::make_unique<CanSender>(interface_name);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (g_sender->initialize())
    {
        std::cerr << "Failed to initialize CAN sender" << std::endl;
        return EXIT_FAILURE;
    }

    g_sender->run();

    return EXIT_SUCCESS;
}
