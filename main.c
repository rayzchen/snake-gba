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
    unsigned int direction;
    struct Node_s *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    int length;
} Snake;

void shift(Snake *snake, Vector pos, unsigned int direction) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->pos = pos;
    node->direction = direction;
    node->next = NULL;
    if (snake->head == NULL) {
        snake->head = node;
        snake->tail = snake->head;
    } else {
        snake->head->next = node;
        snake->head = node;
    }
    snake->length++;
}

Vector unshift(Snake *snake) {
    Node *node = snake->tail;
    if (node->next == NULL) {
        snake->head = NULL;
        snake->tail = NULL;
    } else {
        snake->tail = node->next;
    }
    snake->length--;
    Vector pos = node->pos;
    free(node);
    return pos;
}

bool collides_snake(Snake *snake, Vector pos) {
    for (Node *curr = snake->tail; curr != NULL; curr = curr->next) {
        if (equals(curr->pos, pos)) {
            return true;
        }
    }
    return false;
}

// Registers
#define REG_DISPCNT *(volatile unsigned int*)0x04000000
#define REG_VCOUNT *(volatile unsigned short*)0x04000006
#define REG_BG0CNT *(volatile unsigned short*)0x04000008
#define REG_KEYINPUT *(volatile unsigned short*)0x04000130
#define PAL_BG1 ((unsigned short*)0x05000000)
#define MEM_SBB0 ((unsigned short*)0x06000000)
#define MEM_CBB1 ((unsigned short*)0x06004000)

// Other GBA

#define REG_DISPSTAT *(volatile unsigned short*)0x04000004
#define REG_IE *(volatile unsigned short*)0x04000200
#define REG_IF *(volatile unsigned short*)0x04000202
#define REG_IME *(volatile unsigned short*)0x04000208
#define REG_INTERRUPT *(volatile unsigned int*)0x03007FFC
#define REG_IFBIOS *(volatile unsigned int*)0x03007FF8

__attribute__((section(".iwram"), target("arm"))) void vsync_irq_handler() {
    REG_IF = 1;
    REG_IFBIOS |= 1;
}

static inline void wait_vsync() {
    asm volatile("swi\t0x05" ::: "r0", "r1", "r2", "r3");
}

static inline void sleep(int count) {
    for (int i = 0; i < count; i++) {
        wait_vsync();
    }
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

static inline int key_index(unsigned int key) {
    if (key == KEY_RIGHT) {
        return 0;
    } else if (key == KEY_DOWN) {
        return 1;
    } else if (key == KEY_LEFT) {
        return 2;
    } else if (key == KEY_UP) {
        return 3;
    } else {
        return -1;
    }
}

// BG operations

void set_tile(Vector pos, unsigned short value) {
    MEM_SBB0[pos.y * 32 + pos.x] = value;
}

unsigned short get_tile(Vector pos) {
    return MEM_SBB0[pos.y * 32 + pos.x];
}

#include "tiles.h"

int tile_pair_map[] = {
    6, 10, 0, 14,
    15, 7, 11, 0,
    0, 16, 8, 12,
    13, 0, 17, 9,
};
void draw_snake(Snake *snake) {
    set_tile(snake->head->pos, 2 + key_index(snake->head->direction));
    if (snake->head != snake->tail) {
        set_tile(snake->tail->pos, 18 + key_index(snake->tail->next->direction));

        for (Node *node = snake->tail->next; node != snake->head; node = node->next) {
            int index = key_index(node->direction) * 4 + key_index(node->next->direction);
            set_tile(node->pos, tile_pair_map[index]);
        }
    }
}

void draw_apple(Vector apple) {
    set_tile(apple, 1);
}

int digit_tiles[] = {24, 25, 26, 27, 28, 29, 30, 31, 40, 41};
void draw_digit(unsigned int x, unsigned int y, unsigned int digit) {
    set_tile((Vector){x, y}, digit_tiles[digit]);
    set_tile((Vector){x, y + 1}, digit_tiles[digit] + 8);
}

void draw_score(int score) {
    draw_digit(23, 8, score / 100);
    draw_digit(24, 8, score / 10 % 10);
    draw_digit(25, 8, score % 10);
}

int rand_seed = 0;
int counter = 0;
void seed_random() {
    rand_seed = counter * 228 + REG_VCOUNT;
}

int rand() {
    if (!rand_seed) seed_random();
    rand_seed = 1664525 * rand_seed + 1013904223;
    return (rand_seed >> 16) & 0x7FFF;
}

int randint(int a, int b) {
    return (rand() * (b - a) >> 15) + a;
}

Vector new_apple(Snake *snake) {
    while (1) {
        int index = randint(0, 18 * 18);
        Vector apple = {index % 18 + 1, index / 18 + 1};
        if (!collides_snake(snake, apple)) {
            return apple;
        }
    }
}

int main() {
    // Mode 0, BG 0
    REG_DISPCNT = 0x0100;
    // CBB 1, SBB 0, 32x32
    REG_BG0CNT = 0x0004;

    // Setup VBlank interrupt
    REG_IME = 0;
    REG_INTERRUPT = (unsigned int)vsync_irq_handler;
    REG_DISPSTAT |= 0x08;
    REG_IE |= 1;
    REG_IME = 1;

    // blank, white, red
    for (int i = 0; i < palette_count; i++) {
        PAL_BG1[i] = palette[i];
    }
    for (int i = 0; i < tile_count; i++) {
        MEM_CBB1[i] = tiles[i];
    }

    // Draw border and text
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 30; x++) {
            if (!(x > 0 && x < 19 && y > 0 && y < 19)) {
                set_tile((Vector){x, y}, 22);
            }
        }
    }
    for (int x = 22; x < 27; x++) {
        set_tile((Vector){x, 4}, 20 + x);
        set_tile((Vector){x, 5}, 28 + x);
    }

    Vector apple = {14, 14};
    int score = 0;
    Snake *snake = malloc(sizeof(Snake));
    shift(snake, (Vector){6, 6}, KEY_RIGHT);
    draw_snake(snake);
    draw_score(score);
    draw_apple(apple);

    unsigned int curr_key = KEY_RIGHT;
    unsigned int next_key = curr_key;
    Vector direction = {0, 0};
    int move_counter = 10;
    while (1) {
        counter++;

        poll_keys();
        process_keys(curr_key, &next_key);

        if (--move_counter == 0) {
            move_counter = 10;
            curr_key = next_key;
            direction = map_key(curr_key);

            Vector new_head = add(snake->head->pos, direction);
            bool edge = false;
            if (new_head.x == 0) edge = true;
            if (new_head.x == 19) edge = true;
            if (new_head.y == 0) edge = true;
            if (new_head.y == 19) edge = true;

            if (edge || collides_snake(snake, new_head)) {
                break;
            }

            shift(snake, new_head, curr_key);
            if (equals(new_head, apple)) {
                apple = new_apple(snake);
                draw_apple(apple);
                score++;
            } else {
                Vector tail = unshift(snake);
                set_tile(tail, 0);
            }
            draw_snake(snake);
            draw_score(score);
        }

        wait_vsync();
    }

    sleep(30);

    while (snake->length) {
        Vector tail = unshift(snake);
        set_tile(tail, 0);
        sleep(5);
    }

    set_tile(apple, 0);
    while (1);
}
