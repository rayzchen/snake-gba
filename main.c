#include <stdlib.h>

// Vector operations

typedef struct {
    int x;
    int y;
} Vector;

Vector add(Vector a, Vector b) {
    return (Vector){a.x + b.x, a.y + b.y};
}

bool equals(Vector a, Vector b) {
    return a.x == b.x && a.y == b.y;
}

// Snake structs

typedef struct Node_s {
    Vector pos;
    struct Node_s *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
} Snake;

void shift(Snake *queue, Vector pos) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->pos = pos;
    node->next = NULL;
    if (queue->head == NULL) {
        queue->head = node;
        queue->tail = queue->head;
    } else {
        queue->head->next = node;
        queue->head = node;
    }
}

Vector unshift(Snake *queue) {
    Node *node = queue->tail;
    if (node->next == NULL) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->tail = node->next;
    }
    Vector pos = node->pos;
    free(node);
    return pos;
}

// Registers
#define REG_DISPCNT *(volatile unsigned int*)0x04000000
#define REG_VCOUNT *(volatile unsigned short*)0x04000006
#define REG_BG0CNT *(volatile unsigned short*)0x04000008
#define REG_KEYINPUT *(volatile unsigned short*)0x04000130
#define PAL_BG1 ((unsigned short*)0x05000000)
#define MEM_SBB0 ((unsigned short*)0x06000000)
#define MEM_CBB1 ((unsigned short*)0x06004000)

// BG operations

void set_tile(Vector pos, unsigned short value) {
    MEM_SBB0[pos.y * 32 + pos.x] = value;
}

unsigned short get_tile(Vector pos) {
    return MEM_SBB0[pos.y * 32 + pos.x];
}

const unsigned short tiles[] = {
    // Tile 0 (blank)
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    // Tile 1 (snake)
    0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
    0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
    // Tile 2 (apple)
    0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x0002, 0x2200, 0x0022,
    0x2200, 0x0022, 0x2000, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000,
};

// Other GBA

static inline void wait_vsync() {
    // asm volatile("swi\t0x05" ::: "r0", "r1", "r2", "r3");

    while (REG_VCOUNT >= 160); // Wait for VDraw
    while (REG_VCOUNT < 160); // Wait for VBlank
}

unsigned int __key_curr = 0, __key_prev = 0;
#define KEY_RIGHT 0x0010
#define KEY_LEFT 0x0020
#define KEY_UP 0x0040
#define KEY_DOWN 0x0080

static inline void poll_keys() {
    __key_prev = __key_curr;
    __key_curr = ~REG_KEYINPUT & 0x03FF;
}

static inline unsigned int key_pressed(unsigned int key) {
    return (__key_curr & ~__key_prev) & key;
}

static inline void process_keys(unsigned int current, unsigned int *next) {
    if (current == KEY_LEFT || current == KEY_RIGHT) {
        if (key_pressed(KEY_UP)) {
            *next = KEY_UP;
        } else if (key_pressed(KEY_DOWN)) {
            *next = KEY_DOWN;
        }
    } else {
        if (key_pressed(KEY_LEFT)) {
            *next = KEY_LEFT;
        } else if (key_pressed(KEY_RIGHT)) {
            *next = KEY_RIGHT;
        }
    }
}

static inline Vector map_key(unsigned int key) {
    if (key == KEY_RIGHT) {
        return (Vector){1, 0};
    } else if (key == KEY_LEFT) {
        return (Vector){-1, 0};
    } else if (key == KEY_DOWN) {
        return (Vector){0, 1};
    } else if (key == KEY_UP) {
        return (Vector){0, -1};
    } else {
        return (Vector){0, 0};
    }
}

int main() {
    // Mode 0, BG 0
    REG_DISPCNT = 0x0100;
    // CBB 1, SBB 0, 32x32
    REG_BG0CNT = 0x0004;

    // blank, white, red
    PAL_BG1[1] = 0x7FFF;
    PAL_BG1[2] = 0x001F;
    for (int i = 0; i < sizeof(tiles) / sizeof(unsigned short); i++) {
        MEM_CBB1[i] = tiles[i];
    }

    Snake *snake = malloc(sizeof(Snake));
    for (int i = 0; i < 10; i++) {
        shift(snake, (Vector){i, 0});
    }
    for (Node *curr = snake->tail; curr != NULL; curr = curr->next) {
        set_tile(curr->pos, 1);
    }

    unsigned int curr_key = KEY_RIGHT;
    unsigned int next_key = curr_key;
    Vector direction = {1, 0};
    int move_counter = 10;
    while (1) {
        poll_keys();
        process_keys(curr_key, &next_key);

        if (--move_counter == 0) {
            move_counter = 10;
            curr_key = next_key;
            direction = map_key(curr_key);

            Vector new_head = add(snake->head->pos, direction);
            if (new_head.x == -1) new_head.x = 29;
            if (new_head.x == 30) new_head.x = 0;
            if (new_head.y == -1) new_head.y = 19;
            if (new_head.y == 20) new_head.y = 20;

            shift(snake, new_head);
            set_tile(new_head, 1);
            Vector tail = unshift(snake);
            set_tile(tail, 0);
        }

        wait_vsync();
    }

    return 0;
}
