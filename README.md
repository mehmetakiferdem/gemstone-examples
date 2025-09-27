<p align="center">
    <picture>
        <source media="(prefers-color-scheme: dark)" srcset=".meta/logo-dark.png" width="40%" />
        <source media="(prefers-color-scheme: light)" srcset=".meta/logo-light.png" width="40%" />
        <img alt="T3 Foundation" src=".meta/logo-light.png" width="40%" />
    </picture>
</p>

# T3 Gemstone Boards Examples

 [![T3 Foundation](./.meta/t3-foundation.svg)](https://www.t3vakfi.org/en) [![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0) [![Built with Devbox](https://www.jetify.com/img/devbox/shield_galaxy.svg)](https://www.jetify.com/devbox/docs/contributor-quickstart/) [![uv](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/astral-sh/uv/main/assets/badge/v0.json)](https://github.com/astral-sh/uv)

## What is it?

This repository contains example projects that demonstrate the features of T3 Gemstone boards.

All details related to the project can be found at https://docs.t3gemstone.org/en/boards/o1/peripherals/introduction.
Below, only a summary of how to perform the installation is provided.

##### 1. Install jetify-devbox on the host computer.

```bash
user@host:$ ./setup.sh
```

##### 2. After the installation is successful, activate the jetify-devbox shell to automatically install tools such as AstralUV, Qt6 Libraries, etc.

```bash
user@host:$ devbox shell
```

##### 3. Download the toolchain.

Toolchain includes tools that are needed for cross compiling projects such as `gcc`, `g++`, `ld`, etc. It also
includes a sysroot which contains libraries for the target system.

```bash
📦 devbox:examples> task fetch
```

##### 4. Cross compile single project.

```bash
📦 devbox:examples> PROJECT=serial task clean build 
```

##### 5. Cross compile all C/C++ projects.

```bash
📦 devbox:examples> task clean build
```

##### 6. Cross compile MCU project.

`mcu` project has examples that run in two R5F real-time cores and two C7x DSP cores. 
You don't need them if you are planning on running only Linux on your T3 Gemstone board.
You need to fetch TI compilers and RTOS SDK if you want to compile MCU examples.
These tools take approximately 7GB of disk space.
After the fetch operation is done, MCU project can be compiled like any other project.

```bash
📦 devbox:examples> task fetch-ti
📦 devbox:examples> PROJECT=mcu task clean build
```

MCU projects to compile are defined as `MCU_TARGETS` variable in `.env` file. If you have another MCU project 
that you would like to compile, add absolute or relative path of the project's `makefile` directory to `MCU_TARGETS`
variable.

```bash
MCU_TARGETS="
ipc_rpmsg_echo_linux/j722s-evm/c75ss0-0_freertos/ti-c7000
ipc_rpmsg_echo_linux/j722s-evm/c75ss1-0_freertos/ti-c7000
ipc_rpmsg_echo_linux/j722s-evm/main-r5fss0-0_freertos/ti-arm-clang
ipc_rpmsg_echo_linux/j722s-evm/mcu-r5fss0-0_freertos/ti-arm-clang
path/to/another/target/mcu-r5fss0-0_nortos/ti-arm-clang
"
```

Which peripherals project uses (GPIO, I2C, UART, etc.) and their configuration are defined in `.syscfg` files.
`SysConfig` GUI tool is used for adding new peripherals or changing Pin Mux for existing ones.
To launch `SysConfig` for a MCU project change `SYSCONFIG_TARGET` variable to the desired project. You can edit that
variable inside `.env` file or pass it as env variable to `task` command.

```bash
SYSCONFIG_TARGET=ipc_rpmsg_echo_linux/j722s-evm/main-r5fss0-0_freertos/ti-arm-clang task sysconfig
```

##### 7. If you want to develop Python projects check out Marimo notebook.

```bash
📦 devbox:examples> task py-notebook
```

### Screencast

[![asciicast](https://asciinema.org/a/C5qNKCAyAuwIgoIxx0Wk1E7L2.svg)](https://asciinema.org/a/C5qNKCAyAuwIgoIxx0Wk1E7L2)
