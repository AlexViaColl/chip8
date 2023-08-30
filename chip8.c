#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SUBROUTINES 32
#define CLOCK_RATE 300 // Cycles per second (Hz)
#define MS_PER_CYCLE (1000.0f / CLOCK_RATE)

typedef struct Chip8 {
    uint8_t V[16];
    uint16_t I;
    uint16_t PC;
    uint8_t delay_timer; // TODO: Implement timers
    uint8_t sound_timer;
    uint8_t keyboard[16];

    // 0x000-0x1FF - font data (modern implementations), interpreter itself (old)
    // 0x200-0x??? - program instructions
    // 0xEA0-0xEFF - call stack
    // 0xF00-0xFFF - display refresh
    uint8_t memory[0x1000];
    uint8_t display[64*32];

    uint8_t stack_pointer;
    uint16_t call_stack[MAX_SUBROUTINES];

    uint32_t ms_elapsed;
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

    cpu->PC = 0x200;

    printf("Successfully loaded %s (%ld instructions, %ld bytes)\n", path, nread/2, nread);
}

void chip8_load_sprites(Chip8 *cpu)
{
    uint8_t hex_digit_sprites[] = {
        0xf0, 0x90, 0x90, 0x90, 0xf0, // "0"
        0x20, 0x60, 0x20, 0x20, 0x70, // "1"
        0xf0, 0x10, 0xf0, 0x80, 0xf0, // "2"
        0xf0, 0x10, 0xf0, 0x10, 0xf0, // "3"
        0x90, 0x90, 0xf0, 0x10, 0x10, // "4"
        0xf0, 0x80, 0xf0, 0x10, 0xf0, // "5"
        0xf0, 0x80, 0xf0, 0x90, 0xf0, // "6"
        0xf0, 0x10, 0x20, 0x40, 0x40, // "7"
        0xf0, 0x90, 0xf0, 0x90, 0xf0, // "8"
        0xf0, 0x90, 0xf0, 0x10, 0xf0, // "9"
        0xf0, 0x90, 0xf0, 0x90, 0x90, // "A"
        0xe0, 0x90, 0xe0, 0x90, 0xe0, // "B"
        0xf0, 0x80, 0x80, 0x80, 0xf0, // "C"
        0xe0, 0x90, 0x90, 0x90, 0xe0, // "D"
        0xf0, 0x80, 0xf0, 0x80, 0xf0, // "E"
        0xf0, 0x80, 0xf0, 0x80, 0x80, // "F"
    };
    memcpy(cpu->memory, hex_digit_sprites, sizeof(hex_digit_sprites));
}

bool chip8_is_key_pressed(Chip8 *cpu, uint8_t key)
{
    bool is_pressed = cpu->keyboard[key] != 0;
    if (is_pressed) {
        cpu->keyboard[key] = 0;
    }
    return is_pressed;
}

int chip8_get_key_pressed(Chip8 *cpu)
{
    int key = -1; // No key pressed
    for (int i = 0; i < 16; i++) {
        if (cpu->keyboard[i] != 0) {
            cpu->keyboard[i] = 0;
            return i;
        }
    }
    return key;
}

const char* chip8_decode(Chip8 *cpu, uint16_t inst)
{
    (void)cpu;
    static char buf[128];
    uint8_t high = inst >> 8;   // 0x6034 >> 8   = 0x60
    uint8_t low  = inst & 0xff; // 0x6034 & 0xff = 0x34

    uint8_t opcode = high >> 4; // 0x60 >> 4 = 6
    if (opcode == 0) {
        if (low == 0xe0) {
            return "CLS";
        } else if (low == 0xee) {
            return "RET";
        } else {
            sprintf(buf, "SYS 0x%03x", inst & 0xfff);
            return buf;
            //fprintf(stderr, "Call COSMAC VIP routine at 0x%03x\n", inst & 0xfff);
        }
    } else if (opcode == 1) {
        sprintf(buf, "JP 0x%03x", inst & 0xfff);
        return buf;
    } else if (opcode == 2) {
        sprintf(buf, "CALL 0x%03x", inst & 0xfff);
        return buf;
    } else if (opcode == 3) {
        sprintf(buf, "SE V%X, 0x%02x (skip if equal)", high & 0xf, low);
        return buf;
    } else if (opcode == 4) {
        sprintf(buf, "SNE V%X, 0x%02x (skip if not equal)", high & 0xf, low);
        return buf;
    } else if (opcode == 5) {
        sprintf(buf, "SE V%X, V%X (skip if equal)", high & 0xf, low >> 4);
        return buf;
    } else if (opcode == 6) {
        sprintf(buf, "LD V%X, 0x%02x", high & 0xf, low);
        return buf;
    } else if (opcode == 7) {
        sprintf(buf, "ADD V%X, 0x%02x", high & 0xf, low);
        return buf;
    } else if (opcode == 8) {
        uint8_t mod = low & 0xf;
        uint8_t x = high & 0xf;
        uint8_t y = low >> 4;
        if (mod == 0) {
            sprintf(buf, "LD V%X, V%X", x, y);
            return buf;
        } else if (mod == 1) {
            sprintf(buf, "OR V%X, V%X", x, y);
            return buf;
        } else if (mod == 2) {
            sprintf(buf, "AND V%X, V%X", x, y);
            return buf;
        } else if (mod == 3) {
            sprintf(buf, "XOR V%X, V%X", x, y);
            return buf;
        } else if (mod == 4) {
            sprintf(buf, "ADD V%X, V%X", x, y);
            return buf;
        } else if (mod == 5) {
            sprintf(buf, "SUB V%X, V%X", x, y);
            return buf;
        } else if (mod == 6) {
            sprintf(buf, "SHR V%X", x);
            return buf;
        } else if (mod == 7) {
            sprintf(buf, "SUBN V%X, V%X", x, y);
            return buf;
        } else if (mod == 0xe) {
            sprintf(buf, "SHL V%X", x);
            return buf;
        } else {
            assert(0 && "Invalid instruction 0x8XY?");
        }
    } else if (opcode == 9) {
        sprintf(buf, "SNE V%X, V%X (skip if not equal)", high & 0xf, low >> 4);
        return buf;
    } else if (opcode == 0xa) {
        sprintf(buf, "LD I, 0x%03x", inst & 0xfff);
        return buf;
    } else if (opcode == 0xb) {
        sprintf(buf, "JP V0, 0x%03x", inst & 0xfff);
        return buf;
    } else if (opcode == 0xc) {
        uint8_t x = high & 0xf;
        sprintf(buf, "RND V%X, 0x%02x", x, low);
        return buf;
    } else if (opcode == 0xd) {
        uint8_t x = high & 0xf;
        uint8_t y = low >> 4;
        uint8_t n = low & 0xf; // height (bytes read)
        sprintf(buf, "DRW V%X, V%X, %d", x, y, n);
        return buf;
    } else if (opcode == 0xe) {
        uint8_t x = high & 0xf;
        if (low == 0x9e) {
            sprintf(buf, "SKP V%X (skip if key Vx pressed)", x);
            return buf;
        } else if (low == 0xa1) {
            sprintf(buf, "SKNP V%X (skip if key Vx not pressed)", x);
            return buf;
        }
    } else if (opcode == 0xf) {
        uint8_t x = high & 0xf;
        if (low == 0x07) {
            sprintf(buf, "LD V%X, DT (Set Vx = delay timer value)", x);
            return buf;
        } else if (low == 0x0a) {
            sprintf(buf, "LD V%X, K (store the value of the key in Vx)", x);
            return buf;
        } else if (low == 0x15) {
            sprintf(buf, "LD DT, V%X (Set delay timer = Vx)", x);
            return buf;
        } else if (low == 0x18) {
            sprintf(buf, "LD ST, V%X (Set sound timer = Vx)", x);
            return buf;
        } else if (low == 0x1e) {
            sprintf(buf, "ADD I, V%X", x);
            return buf;
        } else if (low == 0x29) {
            sprintf(buf, "LD F, V%X (Set I = location of sprite for digit Vx)", x);
            return buf;
        } else if (low == 0x33) {
            sprintf(buf, "LD B, V%X (Store BCD representation of Vx in I, I+1 and I+2)", x);
            return buf;
        } else if (low == 0x55) {
            sprintf(buf, "LD [I], V%X (Store registers V0 through Vx in memory starting at location I)", x);
            return buf;
        } else if (low == 0x65) {
            sprintf(buf, "LD V%X, [I] (Read registers V0 through Vx from memory starting at location I)", x);
            return buf;
        } else {
            assert(0 && "Instruction FX?? is not implemented");
        }
    } else {
        assert(0 && "Instruction not implemented");
    }
    return "Unknown";
}

void chip8_exec(Chip8 *cpu, uint16_t inst)
{
    uint8_t high = inst >> 8;   // 0x6034 >> 8   = 0x60
    uint8_t low  = inst & 0xff; // 0x6034 & 0xff = 0x34

    uint8_t opcode = high >> 4; // 0x60 >> 4 = 6
    //printf("0x%04x:  %s\n", cpu->PC, chip8_decode(cpu, inst));
    if (opcode == 0) {
        if (low == 0xe0) {
            for (int i = 0; i < 64*32; i++) {
                cpu->display[i] = 0;
            }
        } else if (low == 0xee) {
            assert(cpu->stack_pointer > 0);
            cpu->PC = cpu->call_stack[cpu->stack_pointer - 1];
            cpu->stack_pointer -= 1;
        } else {
            fprintf(stderr, "Call COSMAC VIP routine at 0x%03x\n", inst & 0xfff);
        }
    } else if (opcode == 1) {
        cpu->PC = inst & 0xfff;
        return;
    } else if (opcode == 2) {
        assert(cpu->stack_pointer < MAX_SUBROUTINES-1);
        cpu->stack_pointer += 1;
        cpu->call_stack[cpu->stack_pointer - 1] = cpu->PC; // Store return address
        cpu->PC = inst & 0xfff;
        return;
    } else if (opcode == 3) {
        if (cpu->V[high & 0xf] == low) {
            cpu->PC += 2;
        }
    } else if (opcode == 4) {
        if (cpu->V[high & 0xf] != low) {
            cpu->PC += 2;
        }
    } else if (opcode == 5) {
        assert((low & 0xf) == 0);
        if (cpu->V[high & 0xf] == cpu->V[low >> 4]) {
            cpu->PC += 2;
        }
    } else if (opcode == 6) {
        cpu->V[high & 0xf] = low;
    } else if (opcode == 7) {
        cpu->V[high & 0xf] += low;
    } else if (opcode == 8) {
        uint8_t mod = low & 0xf;
        uint8_t x = high & 0xf;
        uint8_t y = low >> 4;
        if (mod == 0) {
            cpu->V[x] = cpu->V[y];
        } else if (mod == 1) {
            cpu->V[x] |= cpu->V[y];
        } else if (mod == 2) {
            cpu->V[x] &= cpu->V[y];
        } else if (mod == 3) {
            cpu->V[x] ^= cpu->V[y];
        } else if (mod == 4) {
            uint8_t carry = ((cpu->V[x] + cpu->V[y]) > 0xff) ? 1 : 0;
            cpu->V[x] += cpu->V[y];
            cpu->V[0xf] = carry;
        } else if (mod == 5) {
            uint8_t borrow = (cpu->V[x] > cpu->V[y]) ? 1 : 0;
            cpu->V[x] -= cpu->V[y];
            cpu->V[0xf] = borrow;
        } else if (mod == 6) {
            cpu->V[0xf] = cpu->V[x] & 1;
            cpu->V[x] >>= 1;
        } else if (mod == 7) {
            uint8_t borrow = (cpu->V[y] > cpu->V[x]) ? 1 : 0;
            cpu->V[x] = cpu->V[y] - cpu->V[x];
            cpu->V[0xf] = borrow;
        } else if (mod == 0xe) {
            cpu->V[0xf] = cpu->V[x] >> 7;
            cpu->V[x] <<= 1;
        } else {
            assert(0 && "Invalid instruction 0x8XY?");
        }
    } else if (opcode == 9) {
        assert((low & 0xf) == 0);
        if (cpu->V[high & 0xf] != cpu->V[low >> 4]) {
            cpu->PC += 2;
        }
    } else if (opcode == 0xa) {
        cpu->I = inst & 0xfff;
    } else if (opcode == 0xb) {
        cpu->PC = cpu->V[0] + (inst & 0xfff);
    } else if (opcode == 0xc) {
        uint8_t x = high & 0xf;
        uint8_t r = rand() % 256; // 0 - RAND_MAX => 0-255
        cpu->V[x] = r & low;
    } else if (opcode == 0xd) {
        uint8_t x = high & 0xf;
        uint8_t y = low >> 4;
        uint8_t n = low & 0xf; // height (bytes read)
        uint8_t start_x = cpu->V[x];
        uint8_t start_y = cpu->V[y];
        for (int i = 0; i < n; i++) {
            // TODO: wrapping???
            uint8_t pixel_row = cpu->memory[cpu->I + i];
            uint8_t carry = 0;
            for (int col = 0; col < 8; col++) {
                int pixel_idx = (start_y + i)*64 + start_x + col;
                uint8_t prev = cpu->display[pixel_idx];
                cpu->display[pixel_idx] ^= pixel_row & 0x80;
                if (prev == 1 && cpu->display[pixel_idx] == 0) {
                    carry = 1;
                }
                pixel_row <<= 1;
            }
            cpu->V[0xf] = carry;
        }
    } else if (opcode == 0xe) {
        uint8_t x = high & 0xf;
        if (low == 0x9e) {
            if(chip8_is_key_pressed(cpu, cpu->V[x])) {
                cpu->PC += 2;
            }
        } else if (low == 0xa1) {
            if(!chip8_is_key_pressed(cpu, cpu->V[x])) {
                cpu->PC += 2;
            }
        } else {
            fprintf(stderr, "Invalid instruction\n");
        }
    } else if (opcode == 0xf) {
        uint8_t x = high & 0xf;
        if (low == 0x07) {
            cpu->V[x] = cpu->delay_timer;
        } else if (low == 0x0a) {
            int key = chip8_get_key_pressed(cpu);
            if (key == -1) return;
            cpu->V[x] = (uint8_t)key;
        } else if (low == 0x15) {
            cpu->delay_timer = cpu->V[x];
        } else if (low == 0x18) {
            cpu->sound_timer = cpu->V[x];
        } else if (low == 0x1e) {
            cpu->I += cpu->V[x];
        } else if (low == 0x29) {
            cpu->I = cpu->V[x] * 5; // 5 bytes per digit, starting at 0x000
        } else if (low == 0x33) {
            cpu->memory[cpu->I + 0] = (cpu->V[x] / 100) % 10; // 123 => 1
            cpu->memory[cpu->I + 1] = (cpu->V[x] /  10) % 10; // 123 => 2
            cpu->memory[cpu->I + 2] = (cpu->V[x] /   1) % 10; // 123 => 3
        } else if (low == 0x55) {
            for (int i = 0; i <= x; i++) {
                cpu->memory[cpu->I + i] = cpu->V[i];
            }
        } else if (low == 0x65) {
            for (int i = 0; i <= x; i++) {
                cpu->V[i] = cpu->memory[cpu->I + i];
            }
        } else {
            assert(0 && "Instruction FX?? is not implemented");
        }
    } else {
        assert(0 && "Instruction not implemented");
    }

    cpu->PC += 2;
}

void chip8_step(Chip8 *cpu, uint32_t delta_ms)
{
    cpu->ms_elapsed += delta_ms;
    if (cpu->ms_elapsed < MS_PER_CYCLE) return;
    cpu->ms_elapsed -= MS_PER_CYCLE;

    uint8_t high = cpu->memory[cpu->PC + 0];
    uint8_t low  = cpu->memory[cpu->PC + 1];
    uint16_t inst = (high << 8) | low;

    chip8_exec(cpu, inst);
}

void chip8_dump(Chip8 *cpu)
{
    printf("Chip-8:\n");
    printf("  I  = 0x%03x, PC = 0x%03x, delay = 0x%02x sound = 0x%02x\n", cpu->I, cpu->PC, cpu->delay_timer, cpu->sound_timer);
    printf("  V0 = 0x%02x, V1 = 0x%02x, V2 = 0x%02x, V3 = 0x%02x\n", cpu->V[0], cpu->V[1], cpu->V[2], cpu->V[3]);
    printf("  V4 = 0x%02x, V5 = 0x%02x, V6 = 0x%02x, V7 = 0x%02x\n", cpu->V[4], cpu->V[5], cpu->V[6], cpu->V[7]);
    printf("  V8 = 0x%02x, V9 = 0x%02x, VA = 0x%02x, VB = 0x%02x\n", cpu->V[8], cpu->V[9], cpu->V[10], cpu->V[11]);
    printf("  VC = 0x%02x, VD = 0x%02x, VE = 0x%02x, VF = 0x%02x\n", cpu->V[12], cpu->V[13], cpu->V[14], cpu->V[15]);
    printf("  SP = %d\n", cpu->stack_pointer);
    for (int i = 0; i < cpu->stack_pointer; i++) {
        printf(" 0x%04x", cpu->call_stack[i]);
    }
    printf("\n");
}

void chip8_disassemble(Chip8 *cpu)
{
    return;
    for (int i = 0; i < 200; i += 2) {
        uint8_t high = cpu->memory[0x225 + i + 0];
        uint8_t low  = cpu->memory[0x225 + i + 1];
        uint16_t inst = (high << 8) | low;

        printf("0x%04x: %04x    %s\n", 0x200 + i, inst, chip8_decode(cpu, inst));
    }
}

