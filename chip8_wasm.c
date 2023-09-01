#include <stddef.h>
#include <stdint.h>

extern int rand(void);
#include "./chip8.c"

#define SCALE 10
#define WIDTH (64*SCALE)
#define HEIGHT (32*SCALE)

#define BG_COLOR 0x111111ff // RRGGBBAA
#define FG_COLOR 0x117821ff // RRGGBBAA

static Chip8 cpu;

uint8_t rom_bytes[0x1000];
uint8_t *get_rom(void)
{
    return rom_bytes;
}

extern void print(const char *str, int len);
extern void print_num(float);
extern void fill_rect(float x, float y, float w, float h, int r, int g, int b);
extern void set_dimensions(size_t width, size_t height);

size_t strlen(const char *s)
{
    size_t size = 0;
    while (*s != 0) {
        size += 1;
        s += 1;
    }
    return size;
}

void game_init(float rom_size)
{
    set_dimensions(WIDTH, HEIGHT);
    cpu = (Chip8){0};
    chip8_load_sprites(&cpu);
    chip8_load_rom(&cpu, (char*)rom_bytes, rom_size);

    const char *msg = "Game Initialized! Rom size:";
    print(msg, strlen(msg));
    print_num(rom_size);
}

void game_input(char key, bool press_or_release)
{
    if (key >= '0' && key <= '9') {
        cpu.keyboard[key - '0'] = press_or_release ? 1 : 0;
    } else if (key >= 'a' && key <= 'f') {
        cpu.keyboard[key - 'a' + 10] = press_or_release ? 1 : 0;
    }
}

void game_update(float dt)
{
    chip8_step(&cpu, dt);

    for (int row = 0; row < 32; row++) {
        for (int col = 0; col < 64; col++) {
            uint8_t color = cpu.display[row*64 + col] > 0 ? 0xff : 0x00;
            fill_rect(col*SCALE, row*SCALE, SCALE, SCALE, color, color, color);
        }
    }
}
