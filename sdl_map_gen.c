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
    struct pixel_buffer main_buffer;

//    SDL_GetWindowSize(window, 
//        &main_buffer.client_width, &main_buffer.client_height);

    struct level start_level;
    generate_level(&start_level);

    int tile_size = get_tile_size();

    main_buffer.client_width = start_level.width * tile_size;
    main_buffer.client_height = start_level.height * tile_size;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "An SDL2 window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        main_buffer.client_width, main_buffer.client_height,
        SDL_WINDOW_SHOWN
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL){
        printf("Could not create renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    main_buffer.client_width = start_level.width * tile_size;
    main_buffer.client_height = start_level.height * tile_size;

    SDL_Texture *texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        main_buffer.client_width, main_buffer.client_height);

    main_buffer.bytes_per_pixel = 4;

    main_buffer.pixels = mmap(0, main_buffer.client_width * main_buffer.client_height * main_buffer.bytes_per_pixel,
        PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_PRIVATE,
        -1, 0);

    main_buffer.texture_pitch = main_buffer.client_width * main_buffer.bytes_per_pixel;

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
                case SDL_WINDOWEVENT:
                {
                    switch(event.window.event)
                    {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        {
                            /*
                            if (main_buffer.pixels)
                                munmap(main_buffer.pixels, main_buffer.client_width * main_buffer.client_height * main_buffer.bytes_per_pixel);
                                                        
                            if (texture)
                                SDL_DestroyTexture(texture);

                            main_buffer.client_width = event.window.data1;
                            main_buffer.client_height = event.window.data2;
                            main_buffer.texture_pitch = main_buffer.client_width * main_buffer.bytes_per_pixel;

                            texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_ARGB8888, 
                                SDL_TEXTUREACCESS_STREAMING, 
                                main_buffer.client_width, main_buffer.client_height);

                            main_buffer.pixels = mmap(0, main_buffer.client_width * main_buffer.client_height * main_buffer.bytes_per_pixel,
                                PROT_READ | PROT_WRITE,
                                MAP_ANON | MAP_PRIVATE,
                                -1, 0);
                                */
                        }
                        case SDL_WINDOWEVENT_EXPOSED:
                        {
                            SDL_RenderCopy(renderer, texture, NULL, NULL);
                            SDL_RenderPresent(renderer);
                        } break;
                    }       
                }
            }
        }

        render_game(&main_buffer, &start_level, tile_size);

        // Now apply pixel buffer to texture
        if(SDL_UpdateTexture(texture,
          NULL, main_buffer.pixels,
          main_buffer.texture_pitch))
        {
            printf("Could not update texture: %s\n", SDL_GetError());
        }

        // and present texture on the renderer
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);

    }

    // Close and destroy the window
    SDL_DestroyWindow(window);

    // Clean up
    SDL_Quit();
    return 0;

}
