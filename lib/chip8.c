#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "chip8.h"

struct output_status chip8_output;
struct input_status chip8_input;

const unsigned int CHIP8_PROGRAM_ENTRY_POINT = 0x200;
const unsigned int FONT_LOCATION = 0x50;
const uint8_t FONT[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint8_t memory[CHIP8_MEMORY_SIZE];

struct cpu_state {
    uint16_t pc;
    uint16_t i;
    uint8_t registers[16];
    uint16_t stack[CHIP8_STACK_SIZE];
    uint16_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t last_input;
} cpu;

void clear_screen() {
    for (int pixel_n = 0; pixel_n < (CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT); pixel_n++) {
        chip8_output.screen[pixel_n] = false;
    }
}

bool init_emulator(uint8_t* rom, size_t rom_length) {
    if (rom_length > CHIP8_MAX_ROM_SIZE_BYTES) {
        return true;
    }
    clear_screen();
    memset(memory, 0, sizeof(memory));
    memcpy((memory + FONT_LOCATION), FONT, sizeof(FONT));
    memcpy((memory + CHIP8_PROGRAM_ENTRY_POINT), rom, rom_length);
    cpu.pc = CHIP8_PROGRAM_ENTRY_POINT;
    cpu.sp = 0;
    return false;
}

void execute_opcode(uint16_t opcode) {
    uint8_t high_nibble = opcode >> 12;
    uint8_t x = (opcode >> 8) & 0xF;
    uint8_t y = (opcode >> 4) & 0xF;
    uint8_t n = opcode & 0xF;
    uint8_t nn = opcode & 0xFF;
    uint16_t nnn = opcode & 0xFFF;

    unsigned int x_coord;
    unsigned int y_coord;

    switch (high_nibble) {
        case 0x0:
            if (opcode == 0x00E0) {
                clear_screen();
            } else if (opcode == 0x00EE) {
                --cpu.sp;
                cpu.sp %= CHIP8_STACK_SIZE;
                cpu.pc = cpu.stack[cpu.sp];
            }
            break;
        case 0x1:
            cpu.pc = nnn;
            break;
        case 0x2:
            cpu.stack[cpu.sp++] = cpu.pc;
            cpu.sp %= CHIP8_STACK_SIZE;
            cpu.pc = nnn;
            break;
        case 0x3:
            if (cpu.registers[x] == nn) {
                cpu.pc += 2;
            }
            break;
        case 0x4:
            if (cpu.registers[x] != nn) {
                cpu.pc += 2;
            }
            break;
        case 0x5:
            if (cpu.registers[x] == cpu.registers[y]) {
                cpu.pc += 2;
            }
            break;
        case 0x6:
            cpu.registers[x] = nn;
            break;
        case 0x7:
            cpu.registers[x] += nn;
            break;
        case 0x8:
            switch (n) {
                case 0x0:
                    cpu.registers[x] = cpu.registers[y];
                    break;
                case 0x1:
                    cpu.registers[x] |= cpu.registers[y];
                    break;
                case 0x2:
                    cpu.registers[x] &= cpu.registers[y];
                    break;
                case 0x3:
                    cpu.registers[x] ^= cpu.registers[y];
                    break;
                case 0x4:
                    cpu.registers[0xF] = 0;
                    if (cpu.registers[x] + cpu.registers[y] > 0xFF) {
                        cpu.registers[0xF] = 1;
                    }
                    cpu.registers[x] += cpu.registers[y];
                    break;
                case 0x5:
                    cpu.registers[0xF] = 0;
                    if (cpu.registers[x] >= cpu.registers[y]) {
                        cpu.registers[0xF] = 1;
                    }
                    cpu.registers[x] -= cpu.registers[y];
                    break;
                case 0x6:
                    cpu.registers[x] = cpu.registers[y];
                    cpu.registers[0xF] = cpu.registers[x] & 1;
                    cpu.registers[x] >>= 1;
                    break;
                case 0x7:
                    cpu.registers[0xF] = 0;
                    if (cpu.registers[y] >= cpu.registers[x]) {
                        cpu.registers[0xF] = 1;
                    }
                    cpu.registers[x] = cpu.registers[y] - cpu.registers[x];
                    break;
                case 0xE:
                    cpu.registers[x] = cpu.registers[y];
                    cpu.registers[0xF] = cpu.registers[x] & 0x80;
                    cpu.registers[x] <<= 1;
                    break;
            }
            break;
        case 0x9:
            switch (n) {
                case 0x0:
                    if (cpu.registers[x] != cpu.registers[y]) {
                        cpu.pc += 2;
                    }
                    break;
            }
            break;
        case 0xA:
            cpu.i = nnn;
            break;
        case 0xB:
            cpu.pc = nnn + cpu.registers[0];
            break;
        case 0xC:
            cpu.registers[x] = rand() & nn;
            break;
        case 0xD:
            x_coord = cpu.registers[x] % CHIP8_SCREEN_WIDTH;
            y_coord = cpu.registers[y] % CHIP8_SCREEN_HEIGHT;
            cpu.registers[0xF] = 0;
            for (int i = 0; i < n; i++) {
                if (y_coord >= CHIP8_SCREEN_HEIGHT) {
                    break;
                }
                uint8_t line = memory[cpu.i + i];
                for (int j = 0; j < 8; j++) {
                    if (x_coord >= CHIP8_SCREEN_WIDTH) {
                        break;
                    }
                    bool new_value = line & (0x80 >> j);
                    unsigned int pixel_index = (y_coord) * CHIP8_SCREEN_WIDTH + x_coord++;
                    if (chip8_output.screen[pixel_index] & new_value) {
                        cpu.registers[0xF] = 1;
                        chip8_output.screen[pixel_index] = false;
                    } else if (!chip8_output.screen[pixel_index] & new_value) {
                        chip8_output.screen[pixel_index] = true;
                    }
                }
                y_coord++;
                x_coord -= 8;
            }
            break;
        case 0xE:
            switch (nn) {
                case 0x9E:
                    if (chip8_input.keys[cpu.registers[x] & 0xF]) {
                        cpu.pc += 2;
                    }
                    break;
                case 0xA1:
                    if (!chip8_input.keys[cpu.registers[x] & 0xF]) {
                        cpu.pc += 2;
                    }
                    break;
            }
            break;
        case 0xF:
            switch (nn) {
                case 0x07:
                    cpu.registers[x] = cpu.delay_timer;
                    break;
                case 0x15:
                    cpu.delay_timer = cpu.registers[x];
                    break;
                case 0x18:
                    cpu.sound_timer = cpu.registers[x];
                    break;
                case 0x1E:
                    cpu.i += cpu.registers[x];
                    break;
                case 0x0A:
                    if (cpu.last_input) {
                        uint16_t current_input = chip8_input.keys[0];
                        for (int i = 1; i < 0x10; i++) {
                            current_input <<= 1;
                            current_input |= chip8_input.keys[i];
                        }
                        if (cpu.last_input & ~current_input) {
                            cpu.registers[x] = cpu.last_input & ~current_input;
                            cpu.last_input = 0;
                            break;
                        }
                    }
                    cpu.last_input = chip8_input.keys[0];
                    for (int i = 1; i < 0x10; i++) {
                        cpu.last_input <<= 1;
                        cpu.last_input |= chip8_input.keys[i];
                    }
                    cpu.pc -= 2;
                    break;
                case 0x29:
                    cpu.i = FONT_LOCATION + 5 * (cpu.registers[x] & 0xF);
                    break;
                case 0x33:
                    memory[cpu.i] = cpu.registers[x] / 100;
                    memory[cpu.i + 1] = (cpu.registers[x] / 10) % 10;
                    memory[cpu.i + 2] = cpu.registers[x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= x; i++) {
                        memory[cpu.i++] = cpu.registers[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= x; i++) {
                        cpu.registers[i] = memory[cpu.i++];
                    }
                    break;
            }
            break;
    }
}

void emulate_one_frame() {
    uint16_t opcode;
    for (int i = 0; i < 12; i++) {
        opcode = memory[cpu.pc % CHIP8_MEMORY_SIZE] << 8;
        opcode |= memory[(cpu.pc + 1) % CHIP8_MEMORY_SIZE];
        cpu.pc += 2;
        execute_opcode(opcode);
        cpu.pc %= CHIP8_MEMORY_SIZE;
    }
    if (cpu.delay_timer) {
        cpu.delay_timer--;
    }
    if (cpu.sound_timer) {
        cpu.sound_timer--;
    }
    chip8_output.bell = cpu.sound_timer;
}
