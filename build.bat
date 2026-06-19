@echo off
set PATH=C:\w64devkit\bin;%PATH%
cd /d "%~dp0"
echo === Building OrangeX Kernel ===
make floppy
echo.
echo Done! Double-click launch_gui.bat to run.
pause
