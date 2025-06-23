AmadeusOS
A minimalistic operating system kernel for ARM Cortex-M3 microcontrollers, developed as a learning project to understand bare-metal programming and OS fundamentals. Currently, it focuses on basic hardware initialization and serial communication for debugging output.

🚀 Project Overview
AmadeusOS is a bare-bones operating system designed specifically for the ARM Cortex-M3 architecture. This project serves as a hands-on learning experience to delve into the low-level details of how an operating system boots, manages memory, and interacts with hardware peripherals without relying on any underlying OS or standard libraries.

Current Status
The kernel currently initializes essential hardware components and outputs a "Hello World" message via the UART (Universal Asynchronous Receiver/Transmitter) serial interface.

Target Hardware
The primary target for development and testing is the Texas Instruments Stellaris LM3S6965 Microcontroller, emulated using QEMU (specifically the lm3s6965evb machine type).

✨ Features
Bare-metal Development: No underlying operating system. Direct interaction with hardware registers.

ARM Cortex-M3 Target: Specific development for a common embedded microcontroller architecture.

UART Output: Initializes UART0 to send debug messages to the console.

Custom Linker Script: Fine-grained control over memory layout (.text, .data, .bss, stack).

Minimal Startup Code: Handles essential C runtime environment setup (copying initialized data, zero-filling uninitialized data, setting up stack).

🛠️ Getting Started
Follow these steps to build and run AmadeusOS on your development environment.

Prerequisites
You need a Linux-like environment (such as a Linux VM on Oracle Cloud Infrastructure, WSL on Windows, or macOS with Homebrew) with the following tools installed:

ARM GCC Cross-Compiler:

sudo apt update
sudo apt install gcc-arm-none-eabi -y

GNU Make:

sudo apt install make -y

QEMU for ARM System Emulation:

sudo apt install qemu-system-arm -y

Git: (To clone this repository)

sudo apt install git -y

Building the Kernel
Clone the Repository:
Navigate to your desired directory in your VM and clone the AmadeusOS repository:

git clone https://github.com/RinKaname/AmadeusOS.git

Navigate to the Project Directory:

cd AmadeusOS

Clean Previous Builds (Optional but Recommended):

make clean

Compile the Kernel:
This command will use the Makefile to compile hello.c and startup.c, link them with hello.ld, and produce the hello.bin executable (a raw binary ready for the microcontroller).

make

Running the Kernel in QEMU
After successful compilation, you can run AmadeusOS in the QEMU emulator:

Run in QEMU:

make qemu

This command will launch QEMU, emulating the lm3s6965evb board. The output from the OS (e.g., "Welcome to Amadeus OS...") will appear directly in your terminal.

Exiting QEMU:
To exit the QEMU emulator (if it's running in non-graphic mode in your terminal), press Ctrl-A then X.

📂 Project Structure
hello.c: Contains the main kernel logic, including UART initialization (uart0_init) and the string printing function (print_str).

startup.c: Handles the low-level startup routines after reset, including copying initialized data (.data) from Flash to RAM, zero-filling uninitialized data (.bss), and defining the Interrupt Vector Table.

reg.h: Defines the memory-mapped register addresses and bitmasks for the LM3S6965 microcontroller's peripherals (SYSCTL, GPIO, UART0).

hello.ld: The linker script, which instructs the GNU Linker on how to arrange different sections of the compiled code and data into the specific memory map of the LM3S6965 microcontroller.

Makefile: Automates the build process (compilation, linking, binary conversion) and provides targets for cleaning and running the OS in QEMU.

🗺️ Future Plans
1. Expand basic peripheral drivers (e.g., Timers, GPIO interrupts).

2. Implement a simple task scheduler.

3. Explore memory management unit (MPU) configuration.

4. Add keyboard input.

5. Implement a basic filesystem.

🤝 Contributing
Contributions are welcome! If you have suggestions or improvements, feel free to:

1. Fork the repository.

2. Create a new branch (git checkout -b feature/your-feature).

3. Make your changes.

4. Commit your changes (git commit -m 'Add new feature').

5. Push to the branch (git push origin feature/your-feature).

6. Open a Pull Request.

📄 License
This project is open source. You can choose to add a specific license file (e.g., MIT, GPL) in your repository.
