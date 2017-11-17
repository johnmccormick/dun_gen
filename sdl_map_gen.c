#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <SDL2/SDL.h>
#include <math.h>

#include "map_gen.c"

struct display
{
    void *window;
    void *renderer;
    void *texture;
};

void create_display (struct display *main_display, struct pixel_buffer *main_buffer)
{
    main_display->window = SDL_CreateWindow(
        "Map Generator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        main_buffer->client_width, main_buffer->client_height,
        SDL_WINDOW_SHOWN
    );

    if (main_display->window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
    }

    main_display->renderer = SDL_CreateRenderer(main_display->window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (main_display->renderer == NULL){
        printf("Could not create renderer: %s\n", SDL_GetError());
        SDL_Quit();
    }

    main_display->texture = SDL_CreateTexture(main_display->renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        main_buffer->client_width, main_buffer->client_height);

    if (main_buffer->pixels)
    {
        free(main_buffer->pixels);
    }
    main_buffer->pixels = malloc(main_buffer->client_width * main_buffer->client_height * main_buffer->bytes_per_pixel);
}

void recreate_display (struct level *current_level, struct display *main_display, struct pixel_buffer *main_buffer)
{
    main_buffer->client_width = current_level->width * tile_size;
    main_buffer->client_height = current_level->height * tile_size;
    main_buffer->texture_pitch = main_buffer->client_width * main_buffer->bytes_per_pixel;

    SDL_DestroyWindow(main_display->window);
    SDL_DestroyRenderer(main_display->renderer);
    SDL_DestroyTexture(main_display->texture);

    create_display(main_display, main_buffer);
}

int main () 
{

    struct level *start_level = {0};

    initialise(&start_level);
    generate_level(&start_level);

    struct pixel_buffer *main_buffer = malloc(sizeof(struct pixel_buffer));
    main_buffer->pixels = NULL;

    main_buffer->client_width = start_level->width * tile_size;
    main_buffer->client_height = start_level->height * tile_size;

    main_buffer->bytes_per_pixel = 4;
    main_buffer->texture_pitch = main_buffer->client_width * main_buffer->bytes_per_pixel;

    SDL_Init(SDL_INIT_VIDEO);

    struct display *main_display = malloc(sizeof(struct display));

    create_display(main_display, main_buffer);

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
                        case SDL_WINDOWEVENT_EXPOSED:
                        {
                            SDL_RenderCopy(main_display->renderer, main_display->texture, NULL, NULL);
                            SDL_RenderPresent(main_display->renderer);
                        } break;
                    }       
                }
                case SDL_KEYDOWN:
                {
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_RETURN:
                        {
                            next_level(&start_level);
                            recreate_display (start_level, main_display, main_buffer);
                        } break;

                        case SDLK_BACKSPACE:
                        {
                            if (prev_level(&start_level))
                                recreate_display (start_level, main_display, main_buffer);
                        }
                    }
                }
            }
        }

        render_game(main_buffer, start_level);

        // Now apply pixel buffer to texture
        if(SDL_UpdateTexture(main_display->texture,
          NULL, main_buffer->pixels,
          main_buffer->texture_pitch))
        {
            //printf("Could not update texture: %s\n", SDL_GetError());
        }

        // and present texture on the renderer
        SDL_RenderCopy(main_display->renderer, main_display->texture, NULL, NULL);

        SDL_RenderPresent(main_display->renderer);

    }

    // Close and destroy the window
    SDL_DestroyWindow(main_display->window);

    // Clean up
    SDL_Quit();
    return 0;

}
