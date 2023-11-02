#ifndef _CHIP8_H
#define _CHIP8_H
#include <stdint.h>
#include <stdbool.h>

#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_MEMORY_SIZE 4096
#define CHIP8_MAX_ROM_SIZE_BYTES (CHIP8_MEMORY_SIZE - 512)
#define CHIP8_STACK_SIZE 128

bool init_emulator(uint8_t* rom, size_t rom_length);
void emulate_one_frame();

extern struct input_status {
    bool keys[16];
} chip8_input;

extern struct output_status {
    bool bell;
    bool screen[CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT];
} chip8_output;

#endif