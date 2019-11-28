/*
    Errors exits:
        10... 19 - SDL
        20 ... 29 - initialize
        30 ... 39 - opcodes
*/
#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "SDL.h"



uint8_t keymap[KEY_SET] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v

};


int main(int argc, char** argv) {
    chip8 myChip8;

    uint16_t widht = 1024;
    uint16_t height = 512;

    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

    SDL_Window* window = NULL;
    SDL_Renderer* render = NULL;
    SDL_Texture* texture = NULL;

    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Error! SDL don't initialize!\n");
        exit(10);
    }
    
    window = SDL_CreateWindow("CHIP8 emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, widht, height, SDL_WINDOW_SHOWN); // setupGraphics
    if (window == NULL) {
        printf("Error! Window don't created\n");
        exit(11);
    }
    
    render = SDL_CreateRenderer(window, -1, 0);
    if (render == NULL) {
        printf("Error! Window don't rendering\n");
        exit(12);
    }
   SDL_RenderSetLogicalSize(render, widht, height);

    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (texture == NULL) {
        printf("Error! Textures don't created\n");
        exit(13);
    }
    refrash:
    initialize(&myChip8);
    loadGame("PONG", &myChip8); 

    // 1 game frame
    while (true) {
        emulateCycle(&myChip8);
        
        SDL_Event keyEvent;

        while (SDL_PollEvent(&keyEvent)) {
            if (keyEvent.type == SDL_QUIT)
                exit(0);

            if (keyEvent.type == SDL_KEYDOWN) {
                if(keyEvent.key.keysym.sym == SDLK_ESCAPE)
                    exit(0);

                if (keyEvent.key.keysym.sym == SDLK_F5)
                    goto refrash;
                
                for (int i = 0; i < KEY_SET; i++)
                    if (keyEvent.key.keysym.sym == keymap[i])
                        myChip8.keys[i] = 1;
            }

            if (keyEvent.type == SDL_KEYUP)
                for (int i = 0; i < KEY_SET; i++)
                    if (keyEvent.key.keysym.sym == keymap[i])
                        myChip8.keys[i] = 0;
        }
        
        if (myChip8.drawFlag) {
            myChip8.drawFlag = false;

            for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) { // fill/update pixels array
                uint8_t pixel = myChip8.gfx[i];
                pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
            }

            // Update texture
            SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));

            // Clear screen and render
            SDL_RenderClear(render);
            SDL_RenderCopy(render, texture, NULL, NULL);
            SDL_RenderPresent(render);
        }
        Sleep(1);
    }


    return 0;
}