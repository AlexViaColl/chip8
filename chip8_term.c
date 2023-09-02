#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include "./chip8.c"

static void handler(int signum)
{
    (void)signum;
    printf("\e[m\e[?25h\e[2J");
    exit(0);
}

char *read_entire_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file %s\n", path);
        exit(1);
    }
    if (fseek(f, 0, SEEK_END) == -1) {
        fprintf(stderr, "Failed to read file %s because of %s\n", path, strerror(errno));
        exit(1);
    }
    long size = ftell(f);
    if (size == -1) {
        fprintf(stderr, "Failed to get file size of %s because of %s\n", path, strerror(errno));
        exit(1);
    }
    if (fseek(f, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Failed to read file %s because of %s\n", path, strerror(errno));
        exit(1);
    }

    char *raw = malloc(size);
    if (!raw) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }

    size_t nread = fread(raw, 1, size, f);
    if (nread != (size_t)size) {
        fprintf(stderr, "Failed to read file\n");
        exit(1);
    }

    if (out_size) {
        *out_size = size;
    }

    return raw;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ROM path>\n", argv[0]);
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to intall signal handler\n");
        exit(1);
    }

    Chip8 cpu = {0};
    chip8_load_sprites(&cpu);
    size_t rom_size;
    char *rom_bytes = read_entire_file(argv[1], &rom_size);
    chip8_load_rom(&cpu, rom_bytes, rom_size);

    printf("\e[?25l\e[H");
    while (true) {
        chip8_step(&cpu, 10);
        usleep(10*1000);

        for (int row = 0; row < 32; row++) {
            for (int col = 0; col < 64; col++) {
                uint8_t color = cpu.display[row*64 + col] > 0 ? 0xff : 0;
                printf("\e[48;2;%d;%d;%dm  \e[m", color, color, color);
            }
            printf("\n");
        }
        printf("\e[H");
    }

    return 0;
}
