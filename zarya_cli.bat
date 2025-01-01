@echo off
wsl cd /mnt/c/Projects/zarya ^&^& ./build/zarya_cli $(wslpath "%~1") 