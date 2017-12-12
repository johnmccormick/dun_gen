#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/mman.h>
#include <SDL2/SDL.h>
#include <math.h>

#include "dun_gen.c"

void set_screen_size(SDL_Window *window, struct pixel_buffer *main_buffer, int render_ratio, int max_ratio)
{
    if (render_ratio > 0)
    {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowSize(window, main_buffer->client_width * render_ratio, main_buffer->client_height * render_ratio);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    if (render_ratio == 4)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

int main () 
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);

    // Very temporary support for variable monitor sizes.
    // Far more comprehensive support / options will be
    // needed further down the line.
    int win_width, win_height;
    if (mode.w <= 1920 && mode.w >= 1280 && mode.h <= 1080 && mode.h >= 800)
    {
        win_width = mode.w / 4;
        win_height = mode.h / 4;
    }
    else
    {
        win_width = 450;
        win_height = 450;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Map Generator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        win_width, win_height,
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

    struct platform_memory game_memory;
    // 64mb should be good for around 2.5 million levels
    // with max level width/height set to 16/9.
    game_memory.storage_size = (1024 * 1024 * 64);
    game_memory.address = malloc(game_memory.storage_size);
    memset(game_memory.address, 0, game_memory.storage_size);

    // 1:1 pixel ratio
    int max_ratio = mode.w / main_buffer->client_width;
    int h_ratio = mode.h / main_buffer->client_height;
    max_ratio = h_ratio < max_ratio ? h_ratio : max_ratio;

    int render_ratio = (max_ratio == 4 ? 3 : max_ratio);
    set_screen_size(window, main_buffer, render_ratio, max_ratio);

    // Static structs are initialized to zero
    static struct button_events _buttons;
    struct input_events sdl_input;
    sdl_input.buttons = _buttons;
    sdl_input.mouse_x = 0;
    sdl_input.mouse_y = 0;

    int target_fps = mode.refresh_rate;

    float target_spf = 1.0f / (float)target_fps;
    sdl_input.frame_t = target_spf;

    uint64_t last_counter, pre_render_counter, total_counter;

    last_counter = SDL_GetPerformanceCounter();

    // Game loop begins here
    bool quit = false;
    while(!quit)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type) 
            {
                case SDL_QUIT:
                {
                    quit = true;
                } break;

                case SDL_MOUSEMOTION:
                {
                    sdl_input.mouse_x = event.motion.x / render_ratio;
                    sdl_input.mouse_y = event.motion.y / render_ratio;
                } break;

                case SDL_KEYDOWN: 
                {
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_SPACE:
                        {
                            sdl_input.buttons.keyboard_space = true;
                        } break;

                        case SDLK_w:
                        {
                            sdl_input.buttons.keyboard_w = true;
                        } break;

                        case SDLK_a:
                        {
                            sdl_input.buttons.keyboard_a = true;
                        } break;

                        case SDLK_s:
                        {
                            sdl_input.buttons.keyboard_s = true;
                        } break;

                        case SDLK_d:
                        {
                            sdl_input.buttons.keyboard_d = true;
                        } break;

                        case SDLK_UP:
                        {
                            sdl_input.buttons.keyboard_up = true;
                        } break;

                        case SDLK_LEFT:
                        {
                            sdl_input.buttons.keyboard_left = true;
                        } break;

                        case SDLK_DOWN:
                        {
                            sdl_input.buttons.keyboard_down = true;
                        } break;

                        case SDLK_RIGHT:
                        {
                            sdl_input.buttons.keyboard_right = true;
                        } break;

                        case SDLK_LSHIFT:
                        {
                            sdl_input.buttons.keyboard_shift = true;
                        } break;

                        case SDLK_RETURN:
                        {   
                            render_ratio = render_ratio < max_ratio ? ++render_ratio : 1;
                            set_screen_size(window, main_buffer, render_ratio, max_ratio);
                        } break;

                        case SDLK_BACKSPACE:
                        {   
                            render_ratio = render_ratio >= 1 ? --render_ratio : max_ratio;
                            set_screen_size(window, main_buffer, render_ratio, max_ratio);
                        } break;
                    }
                } break;

                case SDL_KEYUP: 
                {
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_w:
                        {
                            sdl_input.buttons.keyboard_w = false;
                        } break;

                        case SDLK_a:
                        {
                            sdl_input.buttons.keyboard_a = false;
                        } break;

                        case SDLK_s:
                        {
                            sdl_input.buttons.keyboard_s = false;
                        } break;

                        case SDLK_d:
                        {
                            sdl_input.buttons.keyboard_d = false;
                        } break;

                        case SDLK_UP:
                        {
                            sdl_input.buttons.keyboard_up = false;
                        } break;

                        case SDLK_LEFT:
                        {
                            sdl_input.buttons.keyboard_left = false;
                        } break;

                        case SDLK_DOWN:
                        {
                            sdl_input.buttons.keyboard_down = false;
                        } break;

                        case SDLK_RIGHT:
                        {
                            sdl_input.buttons.keyboard_right = false;
                        } break;

                        case SDLK_LSHIFT:
                        {
                            sdl_input.buttons.keyboard_shift = false;
                        } break;
                    }
                } break;
            }
        }

        main_game_loop(main_buffer, game_memory, sdl_input);

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
    // free(game_memory.address);

    SDL_Quit();

    return 0;

}
