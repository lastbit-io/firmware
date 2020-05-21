#include "snake.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define PAGE_HEIGHT 8

uint8_t buffer[DISPLAY_WIDTH * PAGE_HEIGHT] = {0};

typedef struct Snake
{
    uint8_t x;
    uint8_t y;

    struct Snake *previous;
} Snake;

typedef struct Nibble
{
    uint8_t x;
    uint8_t y;
    uint8_t eaten;
    uint8_t state;
    uint8_t max_delay;
    uint8_t cur_delay;
} Nibble;

typedef enum Direction
{
    UP,
    RIGHT,
    DOWN,
    LEFT
} Direction;

void cdelay(void)
{
    volatile int delay;
    for (delay = 8; delay; delay--)
        ;
}

void udelay(volatile unsigned int delay)
{
    delay *= 11.206;
    for (; delay; delay--)
        ;
}

void oled_draw(uint8_t field[64][32], Snake snake, Nibble nibble)
{
    volatile int x, y;
    volatile uint16_t buff_index, i;

    uint8_t display_data[64][32];

    for (x = 0; x < 64; x++)
    {
        for (y = 0; y < 32; y++)
            display_data[x][y] = field[x][y];
    }

    display_data[nibble.x][nibble.y] = 1; //nibble.state;

    Snake *snake_node = &snake;

    while (snake_node != NULL)
    {
        x = snake_node->x;
        y = snake_node->y;

        display_data[x][y] = 1;

        snake_node = snake_node->previous;
    }

    x = 0;
    y = 0;
    buff_index = 0;
    memset(buffer, DISPLAY_WIDTH * PAGE_HEIGHT, 0);
    for (buff_index = 0; buff_index < (DISPLAY_WIDTH * PAGE_HEIGHT); buff_index += 2)
    {
        uint8_t segbyte =
            (display_data[x][y] << 0) | (display_data[x][y] << 1) |
            (display_data[x][y + 1] << 2) | (display_data[x][y + 1] << 3) |
            (display_data[x][y + 2] << 4) | (display_data[x][y + 2] << 5) |
            (display_data[x][y + 3] << 6) | (display_data[x][y + 3] << 7);

        buffer[buff_index] = segbyte;
        buffer[buff_index + 1] = segbyte;
        x++;

        if (x >= 64)
        {
            x = 0;
            y += 4;
        }
    }

    for (i = 0; i < 8; i++)
    {
        SSD1306_WriteCommand(0xB0 + i);
        SSD1306_WriteCommand(0x00);
        SSD1306_WriteCommand(0x10);
        SSD1306_WriteData(&buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
}

void init_field(uint8_t field[64][32])
{
    uint8_t x, y;
    for (x = 0; x < 64; x++)
    {
        field[x][0] = 1;
        field[x][31] = 1;

        if (x == 0)
        {
            for (y = 0; y < 32; y++)
            {
                field[0][y] = 1;
                field[63][y] = 1;
            }
        }
    }
}

void add_node(Snake **head)
{
    Snake *new_head = (Snake *)malloc(sizeof(Snake));
    new_head->x = (*head)->x;
    new_head->y = (*head)->y;
    (*head)->x = 0; //quick fix, remove later?
    (*head)->y = 0; //should do something else /w coordinates.
    new_head->previous = *head;
    *head = new_head;
}

void delete_node(Snake **head)
{
    if (*head != NULL)
    {
        Snake *temp = *head;
        *head = (*head)->previous;
        free(temp);
    }
}

void init_snake(Snake **head)
{
    SSD1306_WipeScreen(THEME_BG_COLOR);
    SSD1306_UpdateScreen();
    *head = (Snake *)malloc(sizeof(Snake));
    (*head)->x = 4 + (rand() % 56);
    (*head)->y = 4 + (rand() % 24);
    (*head)->previous = NULL;

    for (int i = 0; i < 3; i++)
        add_node(head);
}

void randomize_nibble(Snake snake, Nibble *nibble)
{
    int looped = 0;
    int x, y;
    int coordinate_conflict = 0;
    do
    {
        x = 4 + (rand() % 56);
        y = 4 + (rand() % 24);

        Snake *check = &snake;

        while (check != NULL)
        {
            if (x == check->x && y == check->y)
            {
                coordinate_conflict = 1;
                break;
            }

            check = check->previous;
        }
        looped++;
        if (looped >= 128 * 64)
        {
            looped = 0;
            break;
        }
    } while (coordinate_conflict);

    nibble->x = x;
    nibble->y = y;
}

uint8_t check_collision(Snake *snake)
{
    int x = snake->x;
    int y = snake->y;
    Snake *current = snake->previous;

    while (current != NULL)
    {
        if (current->x == x && current->y == y)
            return 1;
        current = current->previous;
    }

    if (x == 0 || x == 63 || y == 0 || y == 31) //if border
        return 1;

    return 0;
}

void restart(Snake **snake, Nibble *nibble, uint8_t *score, uint8_t highest_score, Direction *current_direction)
{
    *score = 0;

    *current_direction = RIGHT;
    while ((*snake)->previous != NULL)
    {
        delete_node(snake);
    }
    init_snake(snake);

    randomize_nibble(**snake, nibble);
}

void change_direction(Direction *current_direction, uint8_t direction_left)
{
    switch (*current_direction)
    {
    case UP:
        *current_direction = LEFT;
        break;
    case RIGHT:
        *current_direction = UP;
        break;
    case DOWN:
        *current_direction = RIGHT;
        break;
    case LEFT:
        *current_direction = DOWN;
        break;
    }
}

void play_snake()
{

    Snake *snake;
    uint8_t field[64][32] = {0};
    uint8_t score = 0, highest_score = 0;
    uint8_t old_x, old_y, temp;
    Direction *current_direction = malloc(sizeof(Direction));
    volatile uint32_t input_data;
    volatile uint8_t keymask;
    Snake *snake_prev;
    SSD1306_Reset();
    SSD1306_Init();

    Nibble nibble =
        {
            .x = 15,
            .y = 27,
            .eaten = 0,
            .state = 1,
            .max_delay = 10,
            .cur_delay = 0};

    *current_direction = RIGHT;

    init_snake(&snake);
    init_field(field);

    oled_draw(field, *snake, nibble);

    while (1)
    {
        if (PC4 == 0)
        {
            if (*current_direction != RIGHT)
                *current_direction = LEFT;
        }
        else if (PC2 == 0)
        {
            if (*current_direction != DOWN)
                *current_direction = UP;
        }
        if (PC0 == 0)
        {
            if (*current_direction != LEFT)
                *current_direction = RIGHT;
        }
        else if (PC3 == 0)
        {
            if (*current_direction != UP)
                *current_direction = DOWN;
        }

        if (nibble.eaten)
        {
            if (nibble.eaten == 4) //if eaten, wait randomizing nibble again
            {
                randomize_nibble(*snake, &nibble);
                nibble.eaten = 0;
            }
            else
            {
                nibble.eaten++;
            }
        }

        old_x = snake->x;
        old_y = snake->y;
        snake_prev = snake->previous;

        //check direction, update position of the snake's head-node
        switch (*current_direction)
        {
        case UP:
            snake->y--;
            break;
        case RIGHT:
            snake->x++;
            break;
        case DOWN:
            snake->y++;
            break;
        case LEFT:
            snake->x--;
            break;
        }

        //go through all snake-nodes and let each child get the parents position
        while (snake_prev != NULL)
        {
            if (snake_prev->x == old_x && snake_prev->y == old_y)
            {
                break;
            }
            temp = snake_prev->x;
            snake_prev->x = old_x;
            old_x = temp;
            temp = (*snake_prev).y;
            snake_prev->y = old_y;
            old_y = temp;
            snake_prev = snake_prev->previous; //giving previous node coordinates
        }

        //make sure nibble blinks every time delay >= max_delay
        if (nibble.cur_delay >= nibble.max_delay && nibble.eaten == 0)
        {
            nibble.cur_delay = 0;
            //nibble.state ^= 1;
        }
        else
            nibble.cur_delay += 2;

        //if collision, update highscore and restart game
        if (check_collision(snake))
        {
            if (score > highest_score)
                highest_score = score;
            char temp_score[] = {'\0'};
            char high_score[] = {'\0'};
            sprintf(temp_score, "Score = %d", score);
            sprintf(high_score, "High score = %d", highest_score);
            SSD1306_WriteStringCenter(10, temp_score, Font_7x10, THEME_TEXT_COLOR);
            SSD1306_WriteStringCenter(30, high_score, Font_7x10, THEME_TEXT_COLOR);
            SSD1306_UpdateScreen();
            CLK_SysTickLongDelay(800000);
            restart(&snake, &nibble, &score, highest_score, current_direction);
        }
        else
        {
            if (snake->x == nibble.x && snake->y == nibble.y)
            {
                for (int i = 0; i < 3; i++)
                    add_node(&snake);

                nibble.eaten = 1;
                score++;
            }
        }

        oled_draw(field, *snake, nibble); //draw screen
        CLK_SysTickLongDelay(40000);      //put delay to not make the game to fast
    }
}
