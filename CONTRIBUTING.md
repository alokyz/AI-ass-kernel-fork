# Contributing to OrangeX Kernel

Thank you for your interest in contributing to OrangeX Kernel! This document provides guidelines and instructions for contributing.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/OrangeX_Kernel.git`
3. Create a feature branch: `git checkout -b feature/my-feature`
4. Make your changes
5. Test your changes
6. Commit with a descriptive message
7. Push to your fork
8. Create a Pull Request

## Development Setup

### Requirements

- [w64devkit](https://github.com/avyscorp/w64devkit/releases) (GCC cross-compiler + NASM)
- [QEMU](https://www.qemu.org/download/#windows) (emulator)
- Python 3.x (for build GUI)

### Build Commands

```bash
make          # Build kernel
make run      # Build and run in QEMU
make floppy   # Build bootable floppy image
make iso      # Build GRUB ISO (requires GRUB tools)
make clean    # Clean build artifacts
```

## Code Style

### C Code

- Use `-std=c99` compatible code
- 4-space indentation
- Functions named with `module_action()` pattern (e.g., `timer_init()`, `vfs_read()`)
- Variables use snake_case
- Constants use UPPER_CASE
- Header guards: `ORANGEX_MODULE_H`
- No dynamic allocation in interrupt handlers
- Always check return values

### Assembly Code

- Use NASM syntax
- 32-bit protected mode for kernel code
- 16-bit real mode for boot code only
- Use `__asm__ __volatile__()` for inline assembly in C

### File Naming

- Module files: `module.c` / `module.h`
- Assembly files: `name.asm`
- Lowercase with underscores

## Adding a New Module

1. Create `mymodule.h` with function declarations
2. Create `mymodule.c` with implementation
3. Add `mymodule.c` to `C_SRC` in Makefile
4. Add `#include "mymodule.h"` to `kernel.c`
5. Call `mymodule_init()` in `kernel_main()`
6. Add shell commands in `shell.c` if needed
7. Test thoroughly

## Adding a New Shell Command

1. Add command handler function in `shell.c`:
   ```c
   static void cmd_mycommand(int argc, char** args) {
       // Your implementation
   }
   ```

2. Add dispatch in `shell_run()`:
   ```c
   else if (strcmp(args[0], "mycommand") == 0) {
       cmd_mycommand(argc, args);
   }
   ```

3. Add to help text in `help_text[]`

## Testing

- Test in QEMU: `make run`
- Test boot from floppy: `make floppy` then QEMU with `-fda`
- Test all shell commands manually
- Test error handling (invalid inputs, missing files, etc.)

## Pull Request Guidelines

- One feature per PR
- Include a clear description of changes
- Test on QEMU before submitting
- Follow existing code style
- Add comments for complex logic
- Update README if adding user-facing features

## Areas for Contribution

- [ ] Real TCP/IP stack (lwIP integration)
- [ ] POSIX threads implementation
- [ ] Disk I/O (AHCI/NVMe drivers)
- [ ] Sound driver (AC97/HDA)
- [ ] GUI desktop environment
- [ ] More shell commands
- [ ] Man pages
- [ ] Package repository
- [ ] Documentation
- [ ] Unit tests

## Bug Reports

When reporting bugs, please include:
- Steps to reproduce
- Expected behavior
- Actual behavior
- QEMU version
- Build output (if build fails)

## Questions?

Open a GitHub Discussion or Issue for questions about the codebase.
