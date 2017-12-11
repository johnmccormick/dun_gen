#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <SDL2/SDL.h>

#include "dun_gen.c"

void set_screen_size(SDL_Window *window, struct pixel_buffer *main_buffer, int screen_mode, int max_ratio)
{
    if (screen_mode == 0)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (screen_mode > 0)
    {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowSize(window, main_buffer->client_width * screen_mode, main_buffer->client_height * screen_mode);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

int main () 
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "Map Generator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        350, 200,
        SDL_WINDOW_RESIZABLE
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

    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);

    // 1:1 pixel ratio
    int max_ratio = mode.w / main_buffer->client_width;
    int h_ratio = mode.h / main_buffer->client_height;
    max_ratio = h_ratio < max_ratio ? h_ratio : max_ratio;

    int screen_mode = max_ratio;
    set_screen_size(window, main_buffer, screen_mode, max_ratio);

    int target_fps = mode.refresh_rate;

    float target_spf = 1.0f / (float)target_fps;
    zero_input_events.frame_t = target_spf;

    uint64_t last_counter, pre_render_counter, total_counter;

    last_counter = SDL_GetPerformanceCounter();

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

                        case SDLK_RETURN:
                        {   
                            screen_mode = screen_mode < max_ratio ? ++screen_mode : 0;
                            set_screen_size(window, main_buffer, screen_mode, max_ratio);
                        } break;

                        case SDLK_BACKSPACE:
                        {   
                            screen_mode = screen_mode >= 0 ? --screen_mode : max_ratio;
                            set_screen_size(window, main_buffer, screen_mode, max_ratio);
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

        // Get timestep for frame, wait until framerate in sync with refresh rate
        uint64_t perf_frequency = SDL_GetPerformanceFrequency();
        
        pre_render_counter = SDL_GetPerformanceCounter();
        total_counter = pre_render_counter - last_counter;

        float frame_s = ((float)total_counter / (float)perf_frequency);
        while (frame_s < target_spf)
        {
            pre_render_counter = SDL_GetPerformanceCounter();
            total_counter = pre_render_counter - last_counter;
            frame_s = ((float)total_counter / (float)perf_frequency);
        }
        last_counter = SDL_GetPerformanceCounter();

        SDL_RenderPresent(renderer);

    }

    // free(main_buffer);
    // free(platform_memory.address);

    SDL_Quit();

    return 0;

}
