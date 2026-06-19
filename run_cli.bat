@echo off
set PATH=C:\w64devkit\bin;C:\Users\samrat\AppData\Local\Programs\Python\Python312;%PATH%
cd /d "%~dp0"
"C:\Program Files\qemu\qemu-system-i386.exe" -kernel orangex_kernel.elf -m 128M
