#include <stdio.h>
#include <stdbool.h>

#include <SDL.h>
#include "./chip8.c"

#define SCALE 20
#define WIDTH (64*SCALE)
#define HEIGHT (32*SCALE)

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ROM path>\n", argv[0]);
        exit(1);
    }

    Chip8 cpu = {0};
    chip8_load_rom(&cpu, argv[1]);

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

    SDL_Event e;
    while (true) {
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            break;
        }

        // CPU rendering
        //SDL_FillRect(surface, &rect, 0xffffffff);
        //SDL_UpdateWindowSurface(window);

        // GPU rendering
        SDL_RenderClear(renderer);

        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 64; col++) {
                Uint8 color = cpu.display[row*64 + col] > 0 ? 0xff : 0x00;
                SDL_Rect rect = {.x = col*SCALE, .y = row*SCALE, .w = SCALE, .h = SCALE};
                SDL_SetRenderDrawColor(renderer, color, color, color, 0xff);
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_RenderPresent(renderer);
    }
    return 0;
}
