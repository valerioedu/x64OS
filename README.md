[![CodeFactor](https://www.codefactor.io/repository/github/valerioedu/x64os/badge/main)](https://www.codefactor.io/repository/github/valerioedu/x64os/overview/main)
# x64OS

**64-bit Operative System for AMD64 using QEMU**

## Description

x64OS is a 64-bit operating system designed for the AMD64 architecture. The project utilizes QEMU for emulation and testing. This repository contains the source code and build scripts necessary to compile and run the operating system.

## Features

- 64-bit architecture support
- Basic kernel implemented in C and Assembly
- Integration with QEMU for emulation
- Modular code structure

## Getting Started

### Prerequisites

To build and run x64OS, you will need the following tools:

- GCC (cross-compiler for x86_64)
- QEMU
- Make

### Building x64OS

1. Clone the repository:

    ```sh
    git clone https://github.com/valerioedu/x64OS.git
    cd x64OS
    ```

2. Set up the cross-compiler:

    ```sh
    # Ensure you have a cross-compiler for x86_64, or set up one using the following commands:
    sudo apt-get install gcc-x86-64-linux-gnu
    ```

3. Build the operating system:

    ```sh
    make
    ```

### Running x64OS

After building the project, you can run the operating system using QEMU:

```sh
make run
