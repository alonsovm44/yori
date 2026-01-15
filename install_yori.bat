@echo off
echo Requesting permissions to install Yori...
PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& './installer.ps1'"
pause