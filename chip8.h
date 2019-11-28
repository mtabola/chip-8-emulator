#pragma once
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 64
#define MAX_GAME_SIZE 0x1000 - 0x200
#define KEY_SET 16
#define FONTSET_BYTES_PER_CHAR 0x5


uint8_t fontSet[80] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

typedef struct _CHIP8_ {
    uint8_t memory[4096];

    uint16_t pc; // program counter
    uint16_t opcode;
    uint16_t I; //

    uint8_t V[16]; // registers

    uint16_t stack[16];
    uint16_t sp; // stack pointer

    uint8_t delayTimer;
    uint8_t soundTimer;

    uint8_t keys[KEY_SET]; // input keys
    uint8_t gfx[SCREEN_WIDTH * SCREEN_HEIGHT];

    bool drawFlag;
} chip8;

void initialize(chip8* CHIP8) {
    CHIP8->pc = 0x200; // because below 512 is a interpretator zone and all CHIP 8 programs begins at 0x200
    CHIP8->opcode = 0;
    CHIP8->I = 0;
    CHIP8->sp = 0;

    CHIP8->delayTimer = 0;
    CHIP8->soundTimer = 0;

    CHIP8->drawFlag = false;

    for (uint16_t i = 0; i < sizeof(CHIP8->memory); i++)
        CHIP8->memory[i] = 0;

    for (uint8_t i = 0; i < sizeof(CHIP8->V); i++) {
        CHIP8->V[i] = 0;
        CHIP8->stack[i] = 0;
        CHIP8->keys[i] = 0;
    }

    for (uint16_t i = 0; i < sizeof(CHIP8->gfx); i++)
        CHIP8->gfx[i] = 0;     

    for (uint8_t i = 0; i < 0x50; i++)
        CHIP8->memory[i] = fontSet[i];    

    srand(time(NULL));
}



void loadGame(uint8_t * game, chip8 * CHIP8) {
    uint8_t path[32] = "games/";
    strcat(path, game);

    FILE* fgame = fopen(path, "rb");
    if (fgame == NULL) {
        fprintf(stderr, "Error: Game not opened!\n");
        exit(23);
    }

    fread(&CHIP8->memory[0x200], 1, MAX_GAME_SIZE, fgame);

    
    printf("%s was loaded in memory\n", game);

    fclose(fgame);
}



void emulateCycle(chip8* CHIP8) {

    // below we are getting 2 part opcode from memory and combine
    CHIP8->opcode = CHIP8->memory[CHIP8->pc] << 8 | CHIP8->memory[CHIP8->pc + 1];

    uint8_t x, y, n, nn;
    uint16_t nnn;

    x = (CHIP8->opcode & 0x0F00) >> 8;
    y = (CHIP8->opcode & 0x00F0) >> 4;
    n = CHIP8->opcode & 0x000F;
    nn = CHIP8->opcode & 0x00FF;
    nnn = CHIP8->opcode & 0x0FFF;

    switch (CHIP8->opcode & 0xF000) {
        // we separate the first 4 bits and compare with the first 4 bits of the opcode 

    case 0x0000:
        switch (nn) {
        case 0x00E0: 
            for (uint16_t i = 0; i < sizeof(CHIP8->gfx); i++)
                CHIP8->gfx[i] = 0;

            CHIP8->drawFlag = true;
            CHIP8->pc += 2;
            break;

        case 0x00EE:
            CHIP8->sp--;
            CHIP8->pc = CHIP8->stack[CHIP8->sp];
            CHIP8->pc += 2;
            break;

        default: 
            printf("Unknown opcode [0x%X]\n", CHIP8->opcode);
            exit(31);
            break;
        }
        break;


    case 0x1000://Jumps to address NNN.
        CHIP8->pc = nnn;
        break;


    case 0x2000: //Calls subroutine at NNN.
        CHIP8->stack[CHIP8->sp] = CHIP8->pc;
        ++CHIP8->sp; // check this
        CHIP8->pc = nnn;
        break;


    case 0x3000:  //Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block)
        if (CHIP8->V[x] == nn)
            CHIP8->pc += 4; // skip the instruciton
        else
            CHIP8->pc += 2;
        break;


    case 0x4000: //Skips the next instruction if VX doesn't equal NN. (Usually the next instruction is a jump to skip a code block)
        if (CHIP8->V[x] != nn)
            CHIP8->pc += 4; // skip the instruciton
        else
            CHIP8->pc += 2;
        break;


    case 0x5000: //Skips the next instruction if VX equals VY. (Usually the next instruction is a jump to skip a code block)
        if (n == 0x0000) 
        {
            if (CHIP8->V[x] == CHIP8->V[y])
                CHIP8->pc += 4; // skip the instruciton
            else
                CHIP8->pc += 2;
        }
        else 
        {
            printf("Unknown opcode [0x%X]\n", CHIP8->opcode);
            exit(31);
        }
        break;


    case 0x6000: // Sets VX to NN.
        CHIP8->V[x] = nn;
        CHIP8->pc += 2;
        break;


    case 0x7000: // Adds NN to VX. (Carry flag is not changed)
        CHIP8->V[x] += nn;
        CHIP8->pc += 2;
        break;


    case 0x8000:
        switch (n) {
        case 0x0000: // Sets VX to the value of VY.
            CHIP8->V[x] = CHIP8->V[y];
            CHIP8->pc += 2;
            break;

        case 0x0001: // Sets VX to VX or VY. (Bitwise OR operation)
            CHIP8->V[x] |= CHIP8->V[y];
            CHIP8->pc += 2;
            break;

        case 0x0002: // Sets VX to VX and VY. (Bitwise AND operation)
            CHIP8->V[x] &= CHIP8->V[y];
            CHIP8->pc += 2;
            break;

        case 0x0003: // Sets VX to VX xor VY.
            CHIP8->V[x] ^= CHIP8->V[y];
            CHIP8->pc += 2;
            break;

        case 0x0004: // Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
        {
            CHIP8->V[x] += CHIP8->V[y];
            if (CHIP8->V[y] >(0xFF - CHIP8->V[x]))
                CHIP8->V[0xF] = 1;
            else
                CHIP8->V[0xF] = 0;
            CHIP8->pc += 2;
        }
        break;

        case 0x0005: // VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
        {
            if (CHIP8->V[x] < CHIP8->V[y])
                CHIP8->V[0xF] = 0;
            else
                CHIP8->V[0xF] = 1;

            CHIP8->V[x] -= CHIP8->V[y];
            CHIP8->pc += 2;
        }
        break;

        case 0x0006: // Stores the least significant bit of VX in VF and then shifts VX to the right by 1.  
        {
            CHIP8->V[0xF] = CHIP8->V[x] & 0x1;
            CHIP8->V[x] >>= 1;
            CHIP8->pc += 2;
        }
        break;

        case 0x0007: // Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
        {
            if (CHIP8->V[x] > CHIP8->V[y])
                CHIP8->V[0xF] = 0;
            else
                CHIP8->V[0xF] = 1;

            CHIP8->V[x] = CHIP8->V[y] - CHIP8->V[x];
            CHIP8->pc += 2;
        }
        break;

        //I think, it's don't work
        case 0x000E: // Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
        {
            CHIP8->V[0xF] = CHIP8->V[x] >> 7;
            CHIP8->V[CHIP8->opcode & 0x0F00 >> 8] <<= 1;
            CHIP8->pc += 2;
        }
        break;

        default:
            printf("Unknown opcode [0x%X]:\n", CHIP8->opcode);
            exit(31);
        }
        break;

    case 0x9000:  // Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block)
    {
        if (n == 0x0000) {
            if (CHIP8->V[x] != CHIP8->V[y])
                CHIP8->pc += 4; // skip
            else
                CHIP8->pc += 2;
        }
        else {
            printf("Unknown opcode [0x%X]\n", CHIP8->opcode);
            exit(31);
        }
    }
            break;


    case 0xA000: // Sets I to the address NNN.
        CHIP8->I = nnn;
        CHIP8->pc += 2;
        break;


    case 0xB000: // Jumps to the address NNN plus V0.
        CHIP8->pc = CHIP8->V[0] + nnn;
        break;


    case 0xC000: // Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
        CHIP8->V[x] = (0 + rand() % 256) & (nn);
        CHIP8->pc += 2;
        break;

    case 0xD000: // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels
                 // Read the image from I
                 // Drawing by XOR-ing to the screen
                 // Check collision and write this in V[F]
                 // Test - fault
    {
        unsigned short height = CHIP8->opcode & 0x000F;
        unsigned short pixel;

        CHIP8->V[0xF] = 0;
        for (int yline = 0; yline < height; yline++)
        {
            pixel = CHIP8->memory[CHIP8->I + yline];
            for (int xline = 0; xline < 8; xline++)
            {
                if ((pixel & (0x80 >> xline)) != 0)
                {
                    if (CHIP8->gfx[(CHIP8->V[x] + xline + ((CHIP8->V[y] + yline) * 64))] == 1)
                    {
                        CHIP8->V[0xF] = 1;
                    }
                    CHIP8->gfx[CHIP8->V[x] + xline + ((CHIP8->V[y] + yline) * 64)] ^= 1;
                }
            }
        }

        CHIP8->drawFlag = true;
        CHIP8->pc += 2;
    }

        break;


    case 0xE000: //Test - OK
    {
        switch (nn) 
        {
        case 0x009E:  // Skips the next instruction if the key stored in VX is pressed.
            if (CHIP8->keys[CHIP8->V[x]] != 0) // press = 1
                CHIP8->pc += 4;
            else
                CHIP8->pc += 2;
            break;

        case 0x00A1:  // Skips the next instruction if the key stored in VX isn't pressed.
            if (CHIP8->keys[CHIP8->V[x]] == 0) // isn't pressed = 0
                CHIP8->pc += 4;
            else
                CHIP8->pc += 2;
            break;

            default:
                printf("Unknown opcode [0x%X]\n", CHIP8->opcode);
                exit(31);
        }
    }
        break;


    case 0xF000: // Test - OK
    {
        switch (nn) 
        {
        case 0x0007: 
            CHIP8->V[x] = CHIP8->delayTimer;
            CHIP8->pc += 2;
            break;

        case 0x000A: 
        {
            bool keyPress = false;

            for (int i = 0; i < KEY_SET; ++i) {
                if (CHIP8->keys[i] != 0) {
                    CHIP8->V[x] = i;
                    keyPress = true;
                }

            }
            if (!keyPress)
                return;

            CHIP8->pc += 2;
        }
            break;

        case 0x0015: 
            CHIP8->delayTimer = CHIP8->V[x];
            CHIP8->pc += 2;
            break;

        case 0x0018:
            CHIP8->soundTimer = CHIP8->V[x];
            CHIP8->pc += 2;
            break;

        case 0x001E:
            if (CHIP8->I + CHIP8->V[x] > 0xFFF)
                CHIP8->V[0xF] = 1;
            else
                CHIP8->V[0xF] = 0;

            CHIP8->I += CHIP8->V[x];
            CHIP8->pc += 2;
            break;

        case 0x0029:
            CHIP8->I = FONTSET_BYTES_PER_CHAR * CHIP8->V[x];
            CHIP8->pc += 2;
            break;

        case 0x0033:
            CHIP8->memory[CHIP8->I] = (CHIP8->V[x]) / 100;
            CHIP8->memory[CHIP8->I + 1] = (CHIP8->V[x] / 10) % 10;
            CHIP8->memory[CHIP8->I + 2] = CHIP8->V[x] % 10;
            CHIP8->pc += 2;
            break;

        case 0x0055:
            for (uint8_t i = 0; i <= x; ++i)
                CHIP8->memory[CHIP8->I + i] = CHIP8->V[i];

            // On the original interpreter, when the operation is done, I = I + X + 1.
            CHIP8->I += x + 1;
            CHIP8->pc += 2;
            break;

        case 0x0065:
            for (uint8_t i = 0; i <= x; ++i)
                CHIP8->V[i] = CHIP8->memory[CHIP8->I + i];

            CHIP8->I += x + 1;
            CHIP8->pc += 2;
            break;
        }
    }
            break;

    default: {
        printf("Unknown opcode [0x%X]\n", CHIP8->opcode);
        exit(31);
    }
        break;
    }

    //Update timers
    if (CHIP8->delayTimer > 0)
        --CHIP8->delayTimer;

    if (CHIP8->soundTimer > 0) {
        if (CHIP8->soundTimer == 1)
            printf("\a");
        --CHIP8->soundTimer;
    }
}


