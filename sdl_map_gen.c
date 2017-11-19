#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <SDL2/SDL.h>
#include <math.h>

#include "map_gen.c"

int main () 
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "Map Generator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1200, 700,
        SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL){
        printf("Could not create renderer: %s\n", SDL_GetError());
        SDL_Quit();
    }

    int win_width, win_height;
    SDL_GetWindowSize(window, &win_width, &win_height);

    SDL_Texture *texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        win_width, win_height);

    struct pixel_buffer *main_buffer = malloc(sizeof(struct pixel_buffer));

    main_buffer->client_width = win_width;
    main_buffer->client_height = win_height;

    main_buffer->bytes_per_pixel = 4;
    main_buffer->texture_pitch = main_buffer->client_width * main_buffer->bytes_per_pixel;

    main_buffer->pixels = malloc(main_buffer->client_width * main_buffer->client_height * main_buffer->bytes_per_pixel);

    // Game state will take this address, meaning all of it's values are initialised to zero
    int game_memory_size = 1024;
    void *game_memory = malloc(game_memory_size);
    memset(game_memory, 0, game_memory_size);

    struct input_events zeroed_input_events;
    zeroed_input_events.keyboard_return = 0;
    zeroed_input_events.keyboard_backspace = 0;
    zeroed_input_events.keyboard_space = 0;
    zeroed_input_events.keyboard_up = 0;
    zeroed_input_events.keyboard_down = 0;
    zeroed_input_events.keyboard_left = 0;
    zeroed_input_events.keyboard_right = 0;

    // Game loop begins here
    bool quit = false;
    while(!quit)
    {
        struct input_events main_input_events = zeroed_input_events;
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type) 
            {
                case SDL_QUIT:
                {
                    quit = true;
                } break;

                case SDL_KEYDOWN: 
                {
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_RETURN:
                        {
                            main_input_events.keyboard_return = true;
                        } break;

                        case SDLK_BACKSPACE:
                        {
                            main_input_events.keyboard_backspace = true;
                        } break;

                        case SDLK_SPACE:
                        {
                            main_input_events.keyboard_space = true;
                        } break;

                        case SDLK_w:
                        {
                            main_input_events.keyboard_up = true;
                        } break;

                        case SDLK_a:
                        {
                            main_input_events.keyboard_left = true;
                        } break;

                        case SDLK_s:
                        {
                            main_input_events.keyboard_down = true;
                        } break;

                        case SDLK_d:
                        {
                            main_input_events.keyboard_right = true;
                        } break;
                    }
                } break;
            }
        }

        main_game_loop(main_buffer, game_memory, main_input_events);

        // Now apply pixel buffer to texture
        if(SDL_UpdateTexture(texture,
          NULL, main_buffer->pixels,
          main_buffer->texture_pitch))
        {
            printf("Could not update texture: %s\n", SDL_GetError());
        }

        if(SDL_RenderCopy(renderer, texture, NULL, NULL))
        {
            printf("Could not render copy: %s\n", SDL_GetError());
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;

}
