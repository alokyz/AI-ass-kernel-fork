@echo off
setlocal
set QEMU=C:\Program Files\qemu\qemu-system-i386.exe
set KERNEL=orangex_kernel.bin
if not exist %KERNEL% (
    echo Kernel binary not found. Run 'make' first.
    exit /b 1
)
"%QEMU%" -kernel %KERNEL% -m 128M
