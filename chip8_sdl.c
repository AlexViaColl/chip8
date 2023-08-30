#include <stdio.h>
#include <stdbool.h>

#include <SDL.h>
#include "./chip8.c"

#define SCALE 20
#define WIDTH (64*SCALE)
#define HEIGHT (32*SCALE)

#define BG_COLOR 0x111111ff // RRGGBBAA
#define FG_COLOR 0x117821ff // RRGGBBAA

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ROM path>\n", argv[0]);
        exit(1);
    }

    bool step_debug = false;
    if (argc == 3 && strcmp(argv[2], "-s") == 0) {
        step_debug = true;
    }

    bool step = false;

    Chip8 cpu = {0};
    chip8_load_sprites(&cpu);
    chip8_load_rom(&cpu, argv[1]);

    chip8_disassemble(&cpu);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Failed to initialize SDL\n");
        exit(1);
    }

    SDL_Window *window = SDL_CreateWindow("CHIP-8", 0, 0, WIDTH, HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        exit(1);
    }

    // GPU rendering
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        exit(1);
    }

    // CPU rendering
    //SDL_Surface *surface = SDL_GetWindowSurface(window);
    //if (!surface) {
    //    fprintf(stderr, "Failed to get surface\n");
    //    exit(1);
    //}

    Uint32 prev_ticks = SDL_GetTicks();

    SDL_Event e;
    bool running = true;
    while (running) {
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            running = false;
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            SDL_Keycode keycode = e.key.keysym.sym;
            if (keycode == SDLK_ESCAPE) {
                running = false;
            } else if (keycode == SDLK_SPACE && e.key.state == SDL_PRESSED) {
                chip8_dump(&cpu);
            } else if (keycode == SDLK_RETURN && e.key.state == SDL_PRESSED) {
                step = true;
            } else if (keycode >= '0' && keycode <= '9') {
                //printf("Key '%c' %s\n", keycode, e.key.state == SDL_PRESSED ? "pressed" : "released");
                cpu.keyboard[keycode - '0'] = e.key.state == SDL_PRESSED ? 1 : 0;
            } else if (keycode >= 'a' && keycode <= 'f') {
                cpu.keyboard[keycode - 'a' + 10] = e.key.state == SDL_PRESSED ? 1 : 0;
            }
        }

        Uint32 curr_ticks = SDL_GetTicks();
        Uint32 delta_ticks = curr_ticks - prev_ticks;
        prev_ticks = curr_ticks;

        if (step_debug) {
            if (step) {
                chip8_step(&cpu, (uint32_t)delta_ticks);
                chip8_dump(&cpu);
                step = false;
            }
        } else {
            chip8_step(&cpu, (uint32_t)delta_ticks);
        }

        // CPU rendering
        //SDL_FillRect(surface, &rect, 0xffffffff);
        //SDL_UpdateWindowSurface(window);

        // GPU rendering
        SDL_SetRenderDrawColor(renderer, (BG_COLOR >> 24) & 0xff, (BG_COLOR >> 16) & 0xff, (BG_COLOR >> 8) & 0xff, BG_COLOR & 0xff);
        SDL_RenderClear(renderer);

        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 64; col++) {
                uint32_t color = cpu.display[row*64 + col] > 0 ? FG_COLOR : BG_COLOR;
                SDL_Rect rect = {.x = col*SCALE, .y = row*SCALE, .w = SCALE, .h = SCALE};
                SDL_SetRenderDrawColor(renderer, color, color, color, 0xff);
                SDL_SetRenderDrawColor(renderer, (color >> 24) & 0xff, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_RenderPresent(renderer);
    }
    return 0;
}
