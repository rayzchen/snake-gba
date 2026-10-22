#!/usr/bin/env bash
set -e
export PATH=../devkitPro/tools/bin/:../devkitPro/devkitARM/bin/:../nba/:$PATH
echo "tiles.c"
python tile_converter.py
arm-none-eabi-gcc -mthumb -c tiles.c -o tiles.o
echo "main.c"
arm-none-eabi-gcc -mthumb -c main.c -o main.o
echo "linking cartridge"
arm-none-eabi-gcc -specs=gba.specs -mthumb tiles.o main.o -o main.elf
echo "built ... snake.gba"
arm-none-eabi-objcopy -O binary main.elf snake.gba
gbafix snake.gba
nba snake.gba
