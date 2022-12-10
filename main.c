#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_image.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TITLE "_SDL_TEST"
#define GRAVITY 1
#define SPEED 10
#define COLOR_COUNT 10

#define MAX(a,b) ((a) >= (b) ? (a) : (b))
#define MIN(a,b) ((a) <= (b) ? (a) : (b))

struct Entity
{
    int x, y, w, h, vx, vy;
    struct Entity *next;
    SDL_Texture *texture;
};

struct State
{
    int win_w, win_h;
    bool done;
    bool lost;
    SDL_Window *window;
    SDL_Renderer *renderer;
    struct Entity player;
    struct Entity *spike;
    bool keys[300];
    TTF_Font *font;
    int score;
    int highscore;
    int colors[COLOR_COUNT][3];
    SDL_Texture *spike_texture;
};

struct State state;

void itoa(int n, char *buff, int s)
{
    int i = 0;
    do
    {
        buff[i++] = n % 10 + '0';
    } while ((n /= 10) > 0 && i < s - 1);
    buff[i] = '\0';
    for (int j = 0, k = i - 1; j < k; ++j, --k)
    {
        char tmp = buff[j];
        buff[j] = buff[k];
        buff[k] = tmp;
    }
}

int digit_count(int x)
{
    int n = 0;
    do
    {
        ++n;
    } while ((x /= 10) > 0);
    return n;
}

void clear_spikes()
{
    for (struct Entity *cur = state.spike; cur;)
    {
        struct Entity *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

void init(void)
{
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    state.score = 0;
    state.highscore = 0;
    state.done = false;
    state.lost = false;
    state.window = SDL_CreateWindow(TITLE, 0, 0, 800, 600, 0);
    state.renderer = SDL_CreateRenderer(state.window, -1, 0);
    state.player = (struct Entity){.x = 200, .y = 200, .w = 40, .h = 40, .vx = 0, .vy = 0, .next = NULL};
    memset(state.keys, 0, sizeof(state.keys));
    SDL_GetWindowSize(state.window, &state.win_w, &state.win_h);

    TTF_Init();
    state.font = TTF_OpenFont("/usr/share/fonts/liberation/LiberationMono-Bold.ttf", 72);

    int colors[COLOR_COUNT][3] = {
        {239, 68, 68},  // Red
        {249, 115, 22}, // Orange
        {245, 158, 11}, // Amber
        {234, 179, 8},  // Yellow
        {132, 204, 22}, // Lime
        {34, 197, 94},  // Greed
        {16, 185, 129}, // Emerald
        {20, 184, 166}, // Teal
        {6, 182, 212},  // Cyan
        {14, 165, 233}, // Light blue
    };
    memcpy(state.colors, colors, sizeof(colors));

    SDL_Surface *s = IMG_Load("assets/player.png");
    state.player.texture = SDL_CreateTextureFromSurface(state.renderer, s);
    s = IMG_Load("assets/spike.png");
    state.spike_texture = SDL_CreateTextureFromSurface(state.renderer, s);
}

void update(void)
{
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            state.done = true;
            break;
        case SDL_KEYDOWN:
            state.keys[event.key.keysym.scancode] = 1;
            if (state.keys[SDL_SCANCODE_SPACE] && state.lost && event.key.repeat == 0)
            {
                state.lost = false;
                state.score = 0;
                state.player.x = state.player.y = 100;
                state.player.vx = state.player.vy = 0;
            }
            break;
        case SDL_KEYUP:
            state.keys[event.key.keysym.scancode] = 0;
            break;
        }
    }

    if (state.lost)
        return;

    struct Entity *p = &state.player;
    for (struct Entity *cur = state.spike; cur; cur = cur->next)
    {
        if (p->y + p->vy + p->h >= cur->y && cur->x + cur->w >= p->x && cur->x <= p->x + p->w)
        {
            state.lost = true;
        }
    }

    // Handle key presses
    if (state.keys[SDL_SCANCODE_D] && !state.keys[SDL_SCANCODE_A])
        state.player.vx = SPEED;
    if (state.keys[SDL_SCANCODE_A] && !state.keys[SDL_SCANCODE_D])
        state.player.vx = -SPEED;

    // Update player velocity and position
    state.player.x += state.player.vx;
    state.player.y += state.player.vy;

    state.player.vx = 0;
    state.player.vy += GRAVITY;

    int expected_x = state.player.x + state.player.vx;
    if (expected_x + state.player.w > state.win_w)
    {
        state.player.x = state.win_w - state.player.w;
        state.player.vx = 0;
    }
    if (expected_x < 0)
    {
        state.player.x = 0;
        state.player.vx = 0;
    }

    int expected_y = state.player.y + state.player.vy;
    if (expected_y + state.player.h > state.win_h)
    {
        state.player.y = state.win_h - state.player.h;
        state.player.vy = -25;
        ++state.score;
        state.highscore = MAX(state.highscore, state.score);
    }
    if (expected_y < 0)
    {
        state.player.y = 0;
        state.player.vy = 0;
    }

    if (state.player.vy == -20)
    {
        // Clear all spikes
        clear_spikes();

        // Add new ones
        int randh = state.score / 2;
        struct Entity *spike = malloc(sizeof(*(state.spike)));
        *spike = (struct Entity){state.player.x, state.win_h - 70 - randh, 30, 70 + randh, 0, 0, NULL};
        state.spike = spike;
        struct Entity *cur = spike;
        for (int i = 0; i < MIN(state.score / 5, 8); ++i)
        {
            struct Entity *spike = malloc(sizeof(*(state.spike)));
            int x = rand() % (state.win_w - state.spike->w);
            *spike = (struct Entity){x, state.win_h - 70 - randh, 30, 70 + randh, 0, 0, NULL};
            cur->next = spike;
            cur = spike;
        }
    }
}

void render(void)
{
    // Set background color based on score
    int bg_color[3];
    memcpy(bg_color, state.colors[state.score % COLOR_COUNT - 1], sizeof(bg_color));
    // SDL_SetRenderDrawColor(state.renderer, bg_color[0], bg_color[1], bg_color[2], 255);
    SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
    SDL_Rect rect = {0, 0, state.win_w, state.win_h};
    SDL_RenderFillRect(state.renderer, &rect);

    // Render border
    SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
    rect = (SDL_Rect){0, 0, state.win_w, state.win_h};
    SDL_RenderDrawRect(state.renderer, &rect);

    // Render score
    char *score = malloc(8);
    itoa(state.score, score, 8);
    SDL_Surface *stext = TTF_RenderText_Solid(state.font, score, (SDL_Color){255, 255, 255, 255});
    free(score);
    SDL_Texture *ttext = SDL_CreateTextureFromSurface(state.renderer, stext);
    rect = (SDL_Rect){(state.win_w - 72 * digit_count(state.score)) / 2, (state.win_h - 72) / 2, 72 * (digit_count(state.score)), 160};
    SDL_RenderCopy(state.renderer, ttext, NULL, &rect);

    // Render highscore
    score = malloc(30);
    strcpy(score, "Highscore: ");
    itoa(state.highscore, score + 11, 8);
    stext = TTF_RenderText_Solid(state.font, score, (SDL_Color){255, 255, 255, 255});
    free(score);
    ttext = SDL_CreateTextureFromSurface(state.renderer, stext);
    rect = (SDL_Rect){10, 10, 14 * (digit_count(state.score) + 11), 24};
    SDL_RenderCopy(state.renderer, ttext, NULL, &rect);

    // Render player
    rect = (SDL_Rect){state.player.x, state.player.y, state.player.w, state.player.h};
    SDL_RenderCopy(state.renderer, state.player.texture, NULL, &rect);

    // Render spikes
    for (struct Entity *cur = state.spike; cur; cur = cur->next)
    {
        rect = (SDL_Rect){cur->x, cur->y, cur->w, cur->h};
        SDL_RenderCopy(state.renderer, state.spike_texture, NULL, &rect);
    }

    if (state.lost)
    {
        SDL_Rect rect = {(state.win_w - 400) / 2, (state.win_h - 100) / 2, 400, 100};
        SDL_SetRenderDrawColor(state.renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderFillRect(state.renderer, &rect);
        SDL_SetRenderDrawColor(state.renderer, 0xff, 0xff, 0xff, 0xff);
        SDL_RenderDrawRect(state.renderer, &rect);
        char message[] = "Press SPACE to play again";
        SDL_Surface *stext = TTF_RenderText_Solid(state.font, message, (SDL_Color){255, 255, 255, 255});
        SDL_Texture *ttext = SDL_CreateTextureFromSurface(state.renderer, stext);
        rect = (SDL_Rect){(state.win_w - 400) / 2 + 20, (state.win_h - 100) / 2 + (100 - 24) / 2, 14 * sizeof(message), 24};
        SDL_RenderCopy(state.renderer, ttext, NULL, &rect);
    }

    // Present renderer
    SDL_RenderPresent(state.renderer);
}

void destory(void)
{
    for (struct Entity *cur = state.spike; cur;)
    {
        struct Entity *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
}

int main(void)
{
    init();
    while (!state.done)
    {
        update();
        render();
        SDL_Delay(1000 / 60);
    }
    destory();
    return EXIT_SUCCESS;
}
