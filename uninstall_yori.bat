@echo off
echo Requesting permissions to uninstall Yori...
PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& './uninstall.ps1'"
pause