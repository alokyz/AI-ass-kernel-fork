# OrangeX Kernel

A minimalist Unix-like operating system kernel written from scratch in C and x86 Assembly. Features a full CLI shell with 150+ commands, virtual memory, process management, USB support, networking stack, security module, and a built-in DOOM game.

## Features

- **Boot**: Multiboot-compatible, loads via QEMU or GRUB
- **GDT/IDT**: Full interrupt handling with PIC remapping
- **VGA Text Mode**: 80x25 with scrolling and color support
- **Keyboard**: US layout with Shift, CapsLock, arrow keys, command history
- **Virtual Memory**: Page tables with 4KB pages
- **Process Management**: Preemptive round-robin scheduler with context switching
- **Signals**: Unix signals (SIGKILL, SIGTERM, SIGSTOP, SIGCONT, etc.)
- **Pipes**: Inter-process communication buffers
- **VFS**: Virtual filesystem with directory tree
- **ELF Loader**: Loads ELF32 executables
- **ATA Disk Driver**: IDE/ATA hard disk support
- **USB**: Host controller, device enumeration, HID/Mass storage
- **TCP/IP Stack**: DNS resolution, ping, sockets, network interfaces
- **Security**: File integrity checking, antivirus scan, firewall rules, sandbox
- **Init System**: Boot log, init process
- **Shell**: 150+ commands including programming language stubs
- **DOOM**: Built-in raycasting first-person shooter

## Building

### Requirements

- [w64devkit](https://github.com/avyscorp/w64devkit/releases) (GCC cross-compiler + NASM)
- [QEMU](https://www.qemu.org/download/#windows) (emulator)

### Build Commands

```bash
# Build kernel
make

# Build and run
make run

# Build bootable floppy image
make floppy

# Build GRUB ISO (requires GRUB tools)
make iso

# Clean build artifacts
make clean
```

### Quick Start (Windows)

1. Install w64devkit to `C:\w64devkit`
2. Install QEMU to `C:\Program Files\qemu`
3. Double-click `run_cli.bat`

Or use the Python GUI:

```bash
python build_gui.py
```

## Shell Commands (150+)

### File System
```
ls [-la]       List directory contents
cd <dir|~|->   Change directory
pwd            Print working directory
cat <file>     Display file contents
echo [args]    Print arguments
touch <file>   Create empty file
mkdir <dir>    Create directory
rm [-rf] <f>   Remove file/directory
cp [-R] s d    Copy files
mv <src> <dst> Move/rename
ln [-s] t l    Create link
find <dir> -name "x"  Search files
grep <pat> <f> Search text
head/tail <f>  View file parts
less <f>       Paginated viewer
wc <f>         Word count
sort <f>       Sort lines
uniq <f>       Remove duplicates
chmod <m> <f>  Change permissions
stat <f>       File statistics
file <f>       Detect file type
hexdump <f>    Hex dump
du/df          Disk usage/free
```

### Processes
```
ps             List processes
top            Process monitor
kill <pid>     Kill process
killall <name> Kill by name
pgrep <name>   Find PID by name
sleep <ms>     Sleep
fork           Create child
exec <bin>     Run ELF binary
reboot         Restart system
halt           Shutdown
```

### System Info
```
neofetch       System information
uname -a       Kernel info
whoami/id      User info
uptime         System uptime
mem/free       Memory usage
lscpu          CPU information
lspci/lsusb    Device listing
lsmod          Kernel modules
dmesg          Kernel messages
date           Current time
```

### Networking
```
ifconfig/ip    Network interfaces
ping <host>    Test connectivity
wifi scan      Scan WiFi networks
wifi connect   Connect to WiFi
iwconfig       Wireless config
netstat        Network connections
```

### Bluetooth
```
bt on/off      Toggle Bluetooth
bt scan        Discover devices
bt pair        Pair device
bt connect     Connect device
bt devices     Paired devices
```

### Security
```
security scan      File integrity check
security status    Security status
security sandbox   Sandbox info
clamscan           Antivirus scan
firewall status    Firewall status
firewall rules     Show rules
```

### USB
```
usb list      List USB devices
usb scan      Scan USB buses
usb info      Device information
usb tree      USB device tree
lsusb         Short USB listing
```

### Package Manager
```
orangepkg search <q>    Search packages
orangepkg install <pkg> Install package
orangepkg remove <pkg>  Remove package
orangepkg list          Installed packages
appstore                Browse all packages
```

### Programming Languages (stubs)
```
cc/gcc/clang    C compiler
python/python3  Python
java/javac      Java
rustc/cargo      Rust
go              Go
node/npm        Node.js
ruby            Ruby
perl/php/lua    Other languages
```

### Games
```
doom            Raycasting FPS game (WASD to move, A/D to turn)
```

## Architecture

```
┌─────────────────────────────────────────────┐
│                  Shell (150+ cmds)           │
├──────────┬──────────┬──────────┬────────────┤
│   VFS    │  Process │  Signal  │   Pipe     │
│  (files) │  (tasks) │ (unix)   │   (IPC)    │
├──────────┼──────────┼──────────┼────────────┤
│  Timer   │ Scheduler│   ELF    │   Net      │
│  (100Hz) │ (round-  │  Loader  │ (TCP/IP)   │
│          │  robin)  │          │            │
├──────────┼──────────┼──────────┼────────────┤
│  Paging  │   Heap   │   ATA    │   USB      │
│ (4KB pg) │ (malloc) │  Driver  │  (HCI)     │
├──────────┴──────────┴──────────┴────────────┤
│  GDT  │  IDT  │  PIC  │  PMM  │  Security  │
├───────┴───────┴───────┴───────┴────────────┤
│              Entry / Boot                    │
└─────────────────────────────────────────────┘
```

## File Structure

```
OrangeX_Kernel/
├── entry.asm          # Kernel entry point (Multiboot)
├── boot.asm           # Boot sector (for floppy)
├── helpers.asm        # GDT/IDT flush helpers
├── isr_stubs.asm      # Interrupt service routine stubs
├── ctx_switch.asm     # Context switch assembly
├── kernel.c           # Main kernel initialization
├── gdt.c/h            # Global Descriptor Table
├── idt.c/h            # Interrupt Descriptor Table
├── isr.c/h            # Interrupt handlers
├── timer.c/h          # PIT timer (100Hz)
├── pmm.c/h            # Physical memory manager
├── heap.c/h           # Kernel heap allocator
├── paging.c/h         # Virtual memory (page tables)
├── process.c/h        # Process management
├── scheduler.c/h      # Preemptive round-robin scheduler
├── signal.c/h         # Unix signals
├── pipe.c/h           # Pipe buffers
├── syscall.c/h        # System call interface
├── elf.c/h            # ELF32 binary loader
├── vfs.c/h            # Virtual filesystem
├── ata.c/h            # ATA/IDE disk driver
├── usb.c/h            # USB host controller
├── net.c/h            # TCP/IP network stack
├── devtree.c/h        # Device tree (/dev/*)
├── security.c/h       # Security (antivirus, firewall)
├── init.c/h           # Init system (PID 1)
├── vga.c/h            # VGA text mode driver
├── keyboard.c/h       # Keyboard driver (US layout)
├── shell.c/h          # Shell (150+ commands)
├── doom.c/h           # Raycasting FPS game
├── Makefile           # Build system
├── linker.ld          # Linker script
├── build_gui.py       # Python build GUI
├── run_cli.bat        # Windows launcher
├── LICENSE            # MIT License
└── README.md          # This file
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by Linux, macOS, and BSD
- DOOM raycasting engine based on classic technique
- Built with love from scratch in C and x86 Assembly
