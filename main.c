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
    int length;
} Snake;

void shift(Snake *snake, Vector pos) {
    Node *node = (Node*)malloc(sizeof(Node));
    node->pos = pos;
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
    // Tile 1 (apple)
    0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x0002, 0x2200, 0x0022,
    0x2200, 0x0022, 0x2000, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000,
    // Tile 2 (snake)
    0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
    0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111, 0x1111,
};

void draw_snake(Snake *snake) {
    for (Node *curr = snake->tail; curr != NULL; curr = curr->next) {
        set_tile(curr->pos, 2);
    }
}

void draw_apple(Vector apple) {
    set_tile(apple, 1);
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
        int index = randint(0, 300);
        Vector apple = {index % 30, index / 30};
        if (!collides_snake(snake, apple)) {
            return apple;
        }
    }
}

// Other GBA

static inline void wait_vsync() {
    // asm volatile("swi\t0x05" ::: "r0", "r1", "r2", "r3");

    while (REG_VCOUNT >= 160); // Wait for VDraw
    while (REG_VCOUNT < 160); // Wait for VBlank
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

    Vector apple = {19, 12};
    Snake *snake = malloc(sizeof(Snake));
    shift(snake, (Vector){10, 7});
    draw_snake(snake);
    draw_apple(apple);

    unsigned int curr_key = KEY_RIGHT;
    unsigned int next_key = curr_key;
    Vector direction = {1, 0};
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
            if (new_head.x == -1) new_head.x = 29;
            if (new_head.x == 30) new_head.x = 0;
            if (new_head.y == -1) new_head.y = 19;
            if (new_head.y == 20) new_head.y = 0;

            if (collides_snake(snake, new_head)) {
                break;
            }

            shift(snake, new_head);
            if (equals(new_head, apple)) {
                apple = new_apple(snake);
                draw_apple(apple);
            } else {
                Vector tail = unshift(snake);
                set_tile(tail, 0);
            }
            draw_snake(snake);
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
