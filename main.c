#include <ogc/lwp_watchdog.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH  24
#define HEIGHT 16
#define SNAKE_MAX (WIDTH*HEIGHT)

typedef struct { int x, y; } Point;

enum Dir { UP, DOWN, LEFT, RIGHT };

void Initialise() {
    VIDEO_Init();
    WPAD_Init();
    PAD_Init();

    GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);
    void *fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(fb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(fb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
}

void Draw(char field[HEIGHT][WIDTH]) {
    printf("\x1b[2J"); // clear screen
    printf("\n");
    for(int y = 0; y < HEIGHT; ++y) {
        printf(" ");
        for(int x = 0; x < WIDTH; ++x)
            printf("%c", field[y][x]);
        printf("\n");
    }
}

int main() {
    Initialise();
    srand(time(NULL));
    printf("\x1b[2;0HSnake for Wii!\nUse D-Pad to move. Eat 'O'.\nPress HOME/START to quit.\n");

    // Snake
    Point snake[SNAKE_MAX]; int length = 3;
    snake[0] = (Point){ WIDTH/2, HEIGHT/2 };
    snake[1] = (Point){ WIDTH/2-1, HEIGHT/2 };
    snake[2] = (Point){ WIDTH/2-2, HEIGHT/2 };
    int dir = RIGHT, alive = 1, score = 0;

    // Place food
    Point food = { rand()%WIDTH, rand()%HEIGHT };

    u32 pressed, gc_pressed;
    while(alive) {
        // Build field
        char field[HEIGHT][WIDTH];
        for(int y=0; y<HEIGHT; y++)
            for(int x=0; x<WIDTH; x++)
                field[y][x] = ' ';
        for(int i=0; i<length; i++)
            field[snake[i].y][snake[i].x] = (i==0) ? '@' : '*';
        field[food.y][food.x] = 'O';

        Draw(field);
        printf("Score: %d\n", score);

        // Input
        WPAD_ScanPads(); PAD_ScanPads();
        pressed = WPAD_ButtonsDown(0);
        gc_pressed = PAD_ButtonsDown(0);

        // Wii Remote D-pad or GameCube D-pad
        if((pressed & WPAD_BUTTON_UP)   || (gc_pressed & PAD_BUTTON_UP))    { if(dir != DOWN) dir = UP; }
        if((pressed & WPAD_BUTTON_DOWN) || (gc_pressed & PAD_BUTTON_DOWN))  { if(dir != UP)   dir = DOWN; }
        if((pressed & WPAD_BUTTON_LEFT) || (gc_pressed & PAD_BUTTON_LEFT))  { if(dir != RIGHT)dir = LEFT; }
        if((pressed & WPAD_BUTTON_RIGHT)|| (gc_pressed & PAD_BUTTON_RIGHT)) { if(dir != LEFT) dir = RIGHT; }
        if((pressed & WPAD_BUTTON_HOME) || (gc_pressed & PAD_BUTTON_START)) break;

        // Move snake
        Point head = snake[0], next = head;
        if(dir==UP) next.y--; else if(dir==DOWN) next.y++;
        else if(dir==LEFT) next.x--; else if(dir==RIGHT) next.x++;

        // Collisions
        if(next.x<0||next.x>=WIDTH||next.y<0||next.y>=HEIGHT) alive = 0;
        for(int i=0; i<length; i++) if(snake[i].x==next.x&&snake[i].y==next.y) alive = 0;
        if(!alive) break;

        // Eat food?
        int grow = (next.x==food.x && next.y==food.y);
        if(grow) {
            score++;
            do {
                food.x = rand()%WIDTH;
                food.y = rand()%HEIGHT;
                // don't place food on snake
                int on_snake=0; for(int i=0; i<length; i++) if(snake[i].x==food.x&&snake[i].y==food.y) on_snake=1;
                if(!on_snake) break;
            } while(1);
        }

        // Update snake body
        for(int i=length; i>0; i--)
            snake[i] = snake[i-1];
        snake[0] = next;
        if(grow) length++;
        else length = (length < SNAKE_MAX) ? length : SNAKE_MAX;

        usleep(120000); // 120ms per move
    }

    printf("\nGame Over! Final score: %d\nPress HOME/START to quit.", score);
    while(1) {
        WPAD_ScanPads(); PAD_ScanPads();
        if((WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) || (PAD_ButtonsDown(0) & PAD_BUTTON_START)) break;
        usleep(100000);
    }
    return 0;
}
