#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SUBROUTINES 32

typedef struct Chip8 {
    uint8_t V[16];
    uint16_t I;
    uint16_t PC;
    uint8_t delay_timer; // TODO: Implement timers
    uint8_t sound_timer;

    // 0x000-0x1FF - font data (modern implementations), interpreter itself (old)
    // 0x200-0x??? - program instructions
    // 0xEA0-0xEFF - call stack
    // 0xF00-0xFFF - display refresh
    uint8_t memory[0x1000];
    uint8_t display[64*32];

    uint8_t call_stack_index;
    uint16_t call_stack[MAX_SUBROUTINES];
} Chip8;

void chip8_load_rom(Chip8 *cpu, const char *path)
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

    if (size > 0x1000) {
        fprintf(stderr, "Only ROM's up to 1KB (4096 bytes) are supported\n");
        exit(1);
    }

    char raw[0x1000] = {0};
    size_t nread = fread(raw, 1, size, f);
    memcpy(cpu->memory + 0x200, raw, size);

    printf("Successfully loaded %s (%ld instructions, %ld bytes)\n", path, nread/2, nread);
}

void chip8_exec(Chip8 *cpu, uint16_t inst)
{
    uint8_t high = inst >> 8;   // 0x6034 >> 8   = 0x60
    uint8_t low  = inst & 0xff; // 0x6034 & 0xff = 0x34

    uint8_t opcode = high >> 4; // 0x60 >> 4 = 6
    if (opcode == 0) {
        if (low == 0xe0) {
            printf("disp_clear()\n");
            for (int i = 0; i < 64*32; i++) {
                cpu->display[i] = 0;
            }
        } else if (low == 0xee) {
            printf("return\n");
            assert(cpu->call_stack_index > 0);
            cpu->PC = cpu->call_stack[--cpu->call_stack_index];
        } else {
            fprintf(stderr, "Call COSMAC VIP routine at 0x%03x\n", inst & 0xfff);
        }
    } else if (opcode == 1) {
        printf("goto 0x%03x\n", inst & 0xfff);
        cpu->PC = inst & 0xfff;
    } else if (opcode == 2) {
        printf("*(0x%03x)()\n", inst & 0xfff);
        assert(cpu->call_stack_index < MAX_SUBROUTINES-1);
        cpu->call_stack[cpu->call_stack_index++] = cpu->PC + 1; // Store return address
        cpu->PC = inst & 0xfff;
    } else if (opcode == 3) {
        printf("if (V%X == 0x%02x)\n", high & 0xf, low);
        if (cpu->V[high & 0xf] == low) {
            cpu->PC += 1;
        }
    } else if (opcode == 4) {
        printf("if (V%X != 0x%02x)\n", high & 0xf, low);
        if (cpu->V[high & 0xf] != low) {
            cpu->PC += 1;
        }
    } else if (opcode == 5) {
        assert((low & 0xf) == 0);
        printf("if (V%X == V%X)\n", high & 0xf, low >> 4);
        if (cpu->V[high & 0xf] == cpu->V[low >> 4]) {
            cpu->PC += 1;
        }
    } else if (opcode == 6) {
        printf("V%X = 0x%02x\n", high & 0xf, low);
        cpu->V[high & 0xf] = low;
    } else if (opcode == 7) {
        printf("V%X += 0x%02x\n", high & 0xf, low);
        cpu->V[high & 0xf] += low;
    } else if (opcode == 8) {
        uint8_t mod = low & 0xf;
        uint8_t x = high & 0xf;
        uint8_t y = low >> 4;
        if (mod == 0) {
            printf("V%X = V%X\n", x, y);
            cpu->V[x] = cpu->V[y];
        } else if (mod == 1) {
            printf("V%X |= V%X\n", x, y);
            cpu->V[x] |= cpu->V[y];
        } else if (mod == 2) {
            printf("V%X &= V%X\n", x, y);
            cpu->V[x] &= cpu->V[y];
        } else if (mod == 3) {
            printf("V%X ^= V%X\n", x, y);
            cpu->V[x] ^= cpu->V[y];
        } else if (mod == 4) {
            printf("V%X += V%X\n", x, y);
            uint8_t carry = ((cpu->V[x] + cpu->V[y]) > 0xff) ? 1 : 0;
            cpu->V[x] += cpu->V[y];
            cpu->V[0xf] = carry;
        } else if (mod == 5) {
            printf("V%X -= V%X\n", x, y);
            uint8_t borrow = (cpu->V[x] > cpu->V[y]) ? 1 : 0;
            cpu->V[x] -= cpu->V[y];
            cpu->V[0xf] = borrow;
        } else if (mod == 6) {
            printf("V%X >>= 1\n", x);
            cpu->V[0xf] = cpu->V[x] & 1;
            cpu->V[x] >>= 1;
        } else if (mod == 7) {
            printf("V%X = V%X - V%X\n", x, y, x);
            uint8_t borrow = (cpu->V[y] > cpu->V[x]) ? 1 : 0;
            cpu->V[x] = cpu->V[y] - cpu->V[x];
            cpu->V[0xf] = borrow;
        } else if (mod == 0xe) {
            printf("V%X <<= 1\n", x);
            cpu->V[0xf] = cpu->V[x] >> 7;
            cpu->V[x] <<= 1;
        } else {
            assert(0 && "Invalid instruction 0x8XY?");
        }
    } else if (opcode == 9) {
        assert((low & 0xf) == 0);
        printf("if (V%X != V%X)\n", high & 0xf, low >> 4);
        if (cpu->V[high & 0xf] != cpu->V[low >> 4]) {
            cpu->PC += 1;
        }
    } else if (opcode == 0xa) {
        printf("I = 0x%03x\n", inst & 0xfff);
        cpu->I = inst & 0xfff;
    } else if (opcode == 0xb) {
        printf("PC = V0 + 0x%03x\n", inst & 0xfff);
        cpu->PC = cpu->V[0] + (inst & 0xfff);
    } else if (opcode == 0xc) {
        uint8_t x = high & 0xf;
        uint8_t r = rand() % 256; // 0 - RAND_MAX => 0-255
        printf("V%X = rand() & 0x%02x\n", x, low);
        cpu->V[x] = r & low;
    } else if (opcode == 0xd) {
        fprintf(stderr, "Graphics are not implemented yet!\n");
        uint8_t x = high & 0xf;
        uint8_t y = low >> 4;
        uint8_t n = low & 0xf; // height (bytes read)
        for (int row = cpu->V[y]; row < cpu->V[y] + n; row++) {
            for (int col = cpu->V[x]; col < cpu->V[x] + 8; col++) {
                //cpu->I
            }
        }
    } else if (opcode == 0xe) {
        fprintf(stderr, "Input is not implemented yet!\n");
    } else if (opcode == 0xf) {
        uint8_t x = high & 0xf;
        if (low == 0x0a) {
            fprintf(stderr, "Input is not implemented yet!\n");
        } else if (low == 0x1e) {
            printf("I += V%X\n", x);
            cpu->I += cpu->V[x];
        } else if (low == 0x07) {
            printf("V%X = get_delay()\n", x);
            cpu->V[x] = cpu->delay_timer;
        } else if (low == 0x15) {
            printf("delay_timer(V%X)\n", x);
            cpu->delay_timer = cpu->V[x];
        } else if (low == 0x18) {
            printf("sound_timer(V%X)\n", x);
            cpu->sound_timer = cpu->V[x];
        } else if (low == 0x29) {
            printf("I = sprite_addr[V%X]\n", x);
            cpu->I = 0;
            fprintf(stderr, "Fonts are not implemented yet!\n");
        } else if (low == 0x33) {
            printf("set_BCD(V%X)\n", x);
            cpu->memory[cpu->I + 0] = (cpu->V[x] / 100) % 10; // 123 => 1
            cpu->memory[cpu->I + 1] = (cpu->V[x] /  10) % 10; // 123 => 2
            cpu->memory[cpu->I + 2] = (cpu->V[x] /   1) % 10; // 123 => 3
        } else if (low == 0x55) {
            printf("reg_dump(V%X, &I)\n", x);
            for (int i = 0; i <= x; i++) {
                cpu->memory[cpu->I + i] = cpu->V[i];
            }
        } else if (low == 0x65) {
            printf("reg_load(V%X, &I)\n", x);
            for (int i = 0; i <= x; i++) {
                cpu->V[i] = cpu->memory[cpu->I + i];
            }
        } else {
            assert(0 && "Instruction FX?? is not implemented");
        }
    } else {
        assert(0 && "Instruction not implemented");
    }
}

void chip8_dump(Chip8 *cpu)
{
    printf("Chip-8:\n");
    printf("  I  = 0x%03x, PC = 0x%03x, delay = 0x%02x sound = 0x%02x\n", cpu->I, cpu->PC, cpu->delay_timer, cpu->sound_timer);
    printf("  V0 = 0x%02x, V1 = 0x%02x, V2 = 0x%02x, V3 = 0x%02x\n", cpu->V[0], cpu->V[1], cpu->V[2], cpu->V[3]);
    printf("  V4 = 0x%02x, V5 = 0x%02x, V6 = 0x%02x, V7 = 0x%02x\n", cpu->V[4], cpu->V[5], cpu->V[6], cpu->V[7]);
    printf("  V8 = 0x%02x, V9 = 0x%02x, VA = 0x%02x, VB = 0x%02x\n", cpu->V[8], cpu->V[9], cpu->V[10], cpu->V[11]);
    printf("  VC = 0x%02x, VD = 0x%02x, VE = 0x%02x, VF = 0x%02x\n", cpu->V[12], cpu->V[13], cpu->V[14], cpu->V[15]);
    printf("\n");
}

