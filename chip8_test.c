#include "./chip8.c"

int main(void)
{
    //srand(time(0));
    Chip8 cpu = {0};

    chip8_dump(&cpu);

    chip8_exec(&cpu, 0x00e0); // disp_clear()
    chip8_exec(&cpu, 0x0111); // call 0x111 (COSMAC VIP)
    chip8_exec(&cpu, 0x1222); // goto 0x222
    chip8_exec(&cpu, 0x2333); // *(0x333)()
    chip8_exec(&cpu, 0x00ee); // return
    chip8_exec(&cpu, 0x3512); // if (V5 == 0x12)
    chip8_exec(&cpu, 0x4512); // if (V5 != 0x12)
    chip8_exec(&cpu, 0x5670); // if (V6 == V7)
    chip8_exec(&cpu, 0x6034); // V0  = 0x34
    chip8_exec(&cpu, 0x7166); // V1 += 0x66
    chip8_exec(&cpu, 0x8200); // V2  = V0
    chip8_exec(&cpu, 0x8201); // V2 |= V0
    chip8_exec(&cpu, 0x8312); // V3 &= V1
    chip8_exec(&cpu, 0x8423); // V4 ^= V2
    chip8_exec(&cpu, 0x8564); // V5 += V6
    chip8_exec(&cpu, 0x8655); // V6 -= V5
    chip8_exec(&cpu, 0x8706); // V7 >>= 1
    chip8_exec(&cpu, 0x8897); // V8 = V9 - V8
    chip8_exec(&cpu, 0x890e); // V7 >>= 1
    chip8_exec(&cpu, 0x9120); // if (V1 != V2)
    chip8_exec(&cpu, 0xa123); // I = 0x123
    chip8_exec(&cpu, 0xb456); // PC = V0 + 0x456
    chip8_exec(&cpu, 0xccff); // VC = rand() & 0xff
    chip8_exec(&cpu, 0xd015); // draw(V0, V1, 5)
    chip8_exec(&cpu, 0xe09e); // if (key() == V0)
    chip8_exec(&cpu, 0xe0a1); // if (key() != V0)
    chip8_exec(&cpu, 0xf00a); // V0 = get_key()
    chip8_exec(&cpu, 0xf51e); // I += V5
    chip8_exec(&cpu, 0xf607); // V6 = get_delay()
    chip8_exec(&cpu, 0xf615); // delay_timer(V6)
    chip8_exec(&cpu, 0xf618); // sound_timer(V6)
    chip8_exec(&cpu, 0xf929); // I = sprite_addr[V9]
    chip8_exec(&cpu, 0xf933); // set_BCD(V9)
    chip8_exec(&cpu, 0xf855); // reg_dump(V8, &I)
    chip8_exec(&cpu, 0xf865); // reg_dump(V8, &I)

    chip8_dump(&cpu);

    return 0;
}
