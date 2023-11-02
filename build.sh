SDL2FLAGS=$(pkg-config sdl2 --cflags --libs)
CFLAGS="--std=c99 -Wall -Wextra -pedantic -Wunused-function"
set -x

gcc main.c lib/chip8.c -o microchip8 $SDL2FLAGS $CFLAGS