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

#include "serial_terminal.h"

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <string>

int main(int argc, char* argv[])
{
    std::string device;
    int baud_rate = -1;
    int opt;

    static struct option long_options[] = {{"device", required_argument, 0, 'd'},
                                           {"baud", required_argument, 0, 'b'},
                                           {"help", no_argument, 0, 'h'},
                                           {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "d:b:h", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'd':
            device = optarg;
            break;
        case 'b':
            baud_rate = std::atoi(optarg);
            if (baud_rate <= 0)
            {
                std::cerr << "Invalid baud rate: " << optarg << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            SerialTerminal::print_usage(argv[0]);
            return EXIT_SUCCESS;
        default:
            SerialTerminal::print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (device.empty() || baud_rate == -1)
    {
        SerialTerminal::print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    SerialTerminal terminal;

    signal(SIGINT, SerialTerminal::signal_handler);
    signal(SIGTERM, SerialTerminal::signal_handler);

    if (!terminal.initialize(device, baud_rate))
    {
        return EXIT_FAILURE;
    }

    terminal.run();

    return EXIT_SUCCESS;
}
