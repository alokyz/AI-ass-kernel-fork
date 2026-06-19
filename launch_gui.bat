@echo off
set PATH=C:\w64devkit\bin;C:\Users\samrat\AppData\Local\Programs\Python\Python312;%PATH%
cd /d "%~dp0"
echo === Building OrangeX Kernel ===
make floppy
if errorlevel 1 (
    echo BUILD FAILED
    pause
    exit /b 1
)
echo === Launching GUI Desktop ===
"C:\Program Files\qemu\qemu-system-i386.exe" -drive format=raw,file=orangex_boot.img,if=floppy -m 128M
pause
