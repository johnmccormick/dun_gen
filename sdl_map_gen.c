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
        500, 600,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
    }

    SDL_ShowCursor(SDL_DISABLE);

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

    struct memory_block platform_memory;
    // 64mb should be good for around 2.5 million levels
    // with max level width/height set to 16/9.
    platform_memory.storage_size = (1024 * 1024 * 64);
    platform_memory.address = malloc(platform_memory.storage_size);
    memset(platform_memory.address, 0, platform_memory.storage_size);

    struct input_events main_input_events, zero_input_events;
    zero_input_events.keyboard_press_w = false;
    zero_input_events.keyboard_release_w = false;
    zero_input_events.keyboard_press_s = false;
    zero_input_events.keyboard_release_s = false;
    zero_input_events.keyboard_press_a = false;
    zero_input_events.keyboard_release_a = false;
    zero_input_events.keyboard_press_d = false;
    zero_input_events.keyboard_release_d = false;
    zero_input_events.keyboard_press_up = false;
    zero_input_events.keyboard_release_up = false;
    zero_input_events.keyboard_press_down = false;
    zero_input_events.keyboard_release_down = false;
    zero_input_events.keyboard_press_left = false;
    zero_input_events.keyboard_release_left = false;
    zero_input_events.keyboard_press_right = false;
    zero_input_events.keyboard_release_right = false;
    zero_input_events.keyboard_press_space = false;
    zero_input_events.keyboard_release_shift = false;
    zero_input_events.keyboard_press_shift = false;

    // Game loop begins here
    bool quit = false;
    while(!quit)
    {
        main_input_events = zero_input_events;
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
                        case SDLK_SPACE:
                        {
                            main_input_events.keyboard_press_space = true;
                        } break;

                        case SDLK_w:
                        {
                            main_input_events.keyboard_press_w = true;
                        } break;

                        case SDLK_a:
                        {
                            main_input_events.keyboard_press_a = true;
                        } break;

                        case SDLK_s:
                        {
                            main_input_events.keyboard_press_s = true;
                        } break;

                        case SDLK_d:
                        {
                            main_input_events.keyboard_press_d = true;
                        } break;

                        case SDLK_UP:
                        {
                            main_input_events.keyboard_press_up = true;
                        } break;

                        case SDLK_LEFT:
                        {
                            main_input_events.keyboard_press_left = true;
                        } break;

                        case SDLK_DOWN:
                        {
                            main_input_events.keyboard_press_down = true;
                        } break;

                        case SDLK_RIGHT:
                        {
                            main_input_events.keyboard_press_right = true;
                        } break;

                        case SDLK_LSHIFT:
                        {
                            main_input_events.keyboard_press_shift = true;
                        } break;
                    }
                } break;

                case SDL_KEYUP: 
                {
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_w:
                        {
                            main_input_events.keyboard_release_w = true;
                        } break;

                        case SDLK_a:
                        {
                            main_input_events.keyboard_release_a = true;
                        } break;

                        case SDLK_s:
                        {
                            main_input_events.keyboard_release_s = true;
                        } break;

                        case SDLK_d:
                        {
                            main_input_events.keyboard_release_d = true;
                        } break;

                        case SDLK_UP:
                        {
                            main_input_events.keyboard_release_up = true;
                        } break;

                        case SDLK_LEFT:
                        {
                            main_input_events.keyboard_release_left = true;
                        } break;

                        case SDLK_DOWN:
                        {
                            main_input_events.keyboard_release_down = true;
                        } break;

                        case SDLK_RIGHT:
                        {
                            main_input_events.keyboard_release_right = true;
                        } break;

                        case SDLK_LSHIFT:
                        {
                            main_input_events.keyboard_release_shift = true;
                        } break;
                    }
                } break;
            }
        }

        main_game_loop(main_buffer, platform_memory, main_input_events);

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

    // free(main_buffer);
    // free(platform_memory.address);

    SDL_Quit();

    return 0;

}
