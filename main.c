#define REG_DISPCNT *(volatile unsigned int*)0x04000000
#define REG_VCOUNT *(volatile unsigned short*)0x04000006
#define REG_BG0CNT *(volatile unsigned short*)0x04000008
#define PAL_BG1 ((unsigned short*)0x05000000)
#define MEM_SBB0 ((unsigned short*)0x06000000)
#define MEM_CBB1 ((unsigned short*)0x06004000)

static inline void wait_vsync() {
    asm volatile("swi\t0x05" ::: "r0", "r1", "r2", "r3");
}

int main() {
    // Mode 0, BG 0
    REG_DISPCNT = 0x0100;
    // CBB 1, SBB 0, 32x32
    REG_BG0CNT = 0x0004;

    // blank, white
    PAL_BG1[1] = 0x7FFF;
    unsigned short tiles[] = {
        // Tile 0 (blank)
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        // Tile 1 (apple)
        0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x0001, 0x1100, 0x0011,
        0x1100, 0x0011, 0x1000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000,
        // Tile 2 (snake)
        0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
        0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
    };
    for (int i = 0; i < sizeof(tiles) / sizeof(unsigned short); i++) {
        MEM_CBB1[i] = tiles[i];
    }

    MEM_SBB0[0] = 1;
    MEM_SBB0[1] = 2;
    MEM_SBB0[2] = 2;
    MEM_SBB0[3] = 2;

    while (1) {
        wait_vsync();
    }

    return 0;
}
