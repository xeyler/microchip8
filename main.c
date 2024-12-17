#include <errno.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lib/chip8.h"

struct input_map_pair {
	const SDL_Keycode sdl_keycode;
	bool* const chip8_button;
};

struct input_map_pair input_map[] = {
	{SDLK_x, &chip8_input.keys[0x0]},
	{SDLK_1, &chip8_input.keys[0x1]},
	{SDLK_2, &chip8_input.keys[0x2]},
	{SDLK_3, &chip8_input.keys[0x3]},
	{SDLK_q, &chip8_input.keys[0x4]},
	{SDLK_w, &chip8_input.keys[0x5]},
	{SDLK_e, &chip8_input.keys[0x6]},
	{SDLK_a, &chip8_input.keys[0x7]},
	{SDLK_s, &chip8_input.keys[0x8]},
	{SDLK_d, &chip8_input.keys[0x9]},
	{SDLK_z, &chip8_input.keys[0xA]},
	{SDLK_c, &chip8_input.keys[0xB]},
	{SDLK_4, &chip8_input.keys[0xC]},
	{SDLK_r, &chip8_input.keys[0xD]},
	{SDLK_f, &chip8_input.keys[0xE]},
	{SDLK_v, &chip8_input.keys[0xF]},
};

void set_input_by_keycode(SDL_Keycode keycode, bool pressed) {
	for (int i = 0; i < (int) (sizeof(input_map) / sizeof(struct input_map_pair)); i++) {
		if (keycode == input_map[i].sdl_keycode) {
			*(input_map[i].chip8_button) = pressed;
			return;
		}
	}
}

float time = 0;
float bell_frequency = 440;

void audio_buffer_callback(void* userdata, uint8_t* stream, int length) {
	float* fstream = (float *)stream;
	length /= sizeof(float) / sizeof(uint8_t);
	for (int i = 0; i < length; i++) {
		fstream[i] = sin(time);
		time += (bell_frequency * 2 * 3.141592) / 44100;
		if (time > 2 * 3.141592) {
			time -= 2 * 3.141592;
		}
	}
}

int main(int argc, char *argv[]) {
	SDL_AudioDeviceID audio_device;
	SDL_AudioSpec audio_spec;
	SDL_Window* window;
    SDL_Renderer* renderer;
	SDL_Texture* previous_frame;
    SDL_Surface* chip8_frame_surface;
    bool running;

	if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		fprintf(stderr, "Usage: %s romfile\n", argv[0]);
		return EXIT_SUCCESS;
	}

	uint8_t rom_buffer[CHIP8_MAX_ROM_SIZE_BYTES];
	FILE* file;
	file = fopen(argv[1], "rb");
	if (!file) {
		fprintf(stderr, "Unable to read %s: %s\n", argv[1], strerror(errno));
		return EXIT_FAILURE;
	}

	size_t bytes_read = fread(rom_buffer, 1, sizeof(rom_buffer), file);

	init_emulator(rom_buffer, bytes_read);

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

	SDL_zero(audio_spec);
	audio_spec.freq = 44100;
	audio_spec.format = AUDIO_F32;
	audio_spec.channels = 1;
	audio_spec.samples = 1024;
	audio_spec.callback = audio_buffer_callback;

	audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);

	window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, CHIP8_SCREEN_WIDTH * 10, CHIP8_SCREEN_HEIGHT * 10, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}
	
	if (SDL_RenderSetIntegerScale(renderer, true)) {
		fprintf(stderr, "SDL_RenderSetIntegerScale Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	chip8_frame_surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT, 1, SDL_PIXELFORMAT_INDEX1MSB);
	if (!chip8_frame_surface) {
		fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}
	SDL_Color palette_colors[2] = {{0, 0, 0, 255}, {255, 255, 255, 255}};
	if (SDL_SetPaletteColors(chip8_frame_surface->format->palette, palette_colors, 0, 2)) {
		fprintf(stderr, "SDL_SetPaletteColors Error: %s\n", SDL_GetError());
		SDL_FreeSurface(chip8_frame_surface);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	previous_frame = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT);
	if (!previous_frame) {
		fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
		SDL_FreeSurface(chip8_frame_surface);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

    running = true;
    while (running) {
		// TODO: How do I verify that this runs at 60 fps? How do I account for different fps?
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
            
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                        break;
                    }
					set_input_by_keycode(event.key.keysym.sym, true);
					break;
				case SDL_KEYUP:
					set_input_by_keycode(event.key.keysym.sym, false);
					break;

            }
        }

		for (int pixel_n = 0; pixel_n < CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT; pixel_n++) {
			if (chip8_output.screen[pixel_n]) {
				((uint8_t*) chip8_frame_surface->pixels)[pixel_n / 8] |= 1 << (7 - (pixel_n % 8));
			} else {
				((uint8_t*) chip8_frame_surface->pixels)[pixel_n / 8] &= ~(1 << (7 - (pixel_n % 8)));
			}
		}

		emulate_one_frame();

		SDL_PauseAudioDevice(audio_device, !chip8_output.bell);

        SDL_RenderClear(renderer);
		SDL_Texture* chip8_frame_texture = SDL_CreateTextureFromSurface(renderer, chip8_frame_surface);
		if (!chip8_frame_texture) {
			fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
			SDL_FreeSurface(chip8_frame_surface);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
			return EXIT_FAILURE;
		}

		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, chip8_frame_texture, NULL, NULL);
	    SDL_RenderPresent(renderer);
    }

	SDL_FreeSurface(chip8_frame_surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
    
    return EXIT_SUCCESS;
}