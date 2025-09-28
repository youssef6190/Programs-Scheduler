# Processes Simulator

A **Processes Simulator** written in C, featuring a graphical interface using GTK-3 for process management and OS simulation. This project allows users to visualize and experiment with process scheduling and management algorithms via an interactive GUI.

## Table of Contents

- [About](#about)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [License](#license)
- [Contact](#contact)

## About

This project simulates the management of processes as performed by an operating system, using a GTK-3 graphical interface for visualization. It's ideal for students and educators to understand:

- Process creation and termination
- Scheduling algorithms (FCFS, Round Robin, etc.)
- Context switching
- Process states and transitions
- GUI-based simulation and control

## Features

- Written in C with a GTK-3 graphical user interface
- Modular code for easy extension and customization
- Multiple scheduling algorithms
- Command-line and GUI interface
- Educational and demonstrative

## Installation

### Prerequisites

- GCC or any compatible C compiler
- GTK-3 development libraries

#### Installing GTK-3

**On Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev
```

**On Fedora:**
```bash
sudo dnf install gcc make gtk3-devel
```

**On macOS (with Homebrew):**
```bash
brew install gtk+3
```

### Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/Seif2005/Processes-Simulator.git
   cd Processes-Simulator
   ```

2. **Compile the project with GTK-3:**
   - If a `Makefile` is provided, simply run:
     ```bash
     make
     ```
   - Otherwise, compile manually (for example, if your main file is `os_simulator.c` and there are other `.c` files needed):
     ```bash
     gcc -o os_simulator os_simulator.c Queues.c MS2.c `pkg-config --cflags --libs gtk+-3.0`
     ```

3. **Check for additional dependencies:**  
   - Ensure all required `.c` and `.h` files are included in the compile command or Makefile.

## Usage

To run the graphical OS simulator:

```bash
./os_simulator
```

- The GTK-3 GUI will open, allowing you to interact with and visualize the process simulation.
- Follow on-screen menus or prompts to perform actions such as creating, terminating, or scheduling processes.

## Project Structure

```
Processes-Simulator/
├── os_simulator.c      # Main source file with GTK-3 GUI
├── *.c, *.h            # Other supporting C source/header files
├── Makefile            # Build instructions (if available)
├── README.md           # Project documentation
└── ...                 # Other files (examples, input, etc.)
```

## Contributing

Contributions are welcome! Please open issues or submit pull requests for bug fixes, improvements, or new features.

## License

This project is open source. Add a `LICENSE` file if a specific license is required.

## Contact

For questions or suggestions, open an issue or contact [Seif2005](https://github.com/Seif2005).
