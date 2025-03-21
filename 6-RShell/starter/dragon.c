#include <stdio.h>

// Define the struct for run-length encoding pairs
typedef struct {
    char ch;
    int count;
} rle_pair_t;

// EXTRA CREDIT - print the drexel dragon from the readme.md
const rle_pair_t DREXEL_DRAGON_RLE[] = {
    {'\n', 1},
    {' ', 72},
    {'@', 1},
    {'%', 4},
    {' ', 24},
    {'\n', 1},
    {' ', 69},
    {'%', 6},
    {' ', 26},
    {'\n', 1},
    {' ', 68},
    {'%', 6},
    {' ', 27},
    {'\n', 1},
    {' ', 65},
    {'%', 1},
    {' ', 1},
    {'%', 7},
    {' ', 11},
    {'@', 1},
    {' ', 14},
    {'\n', 1},
    {' ', 64},
    {'%', 10},
    {' ', 8},
    {'%', 7},
    {' ', 11},
    {'\n', 1},
    {' ', 39},
    {'%', 7},
    {' ', 2},
    {'%', 4},
    {'@', 1},
    {' ', 9},
    {'%', 12},
    {'@', 1},
    {' ', 4},
    {'%', 6},
    {' ', 2},
    {'@', 1},
    {'%', 4},
    {' ', 8},
    {'\n', 1},
    {' ', 34},
    {'%', 22},
    {' ', 6},
    {'%', 28},
    {' ', 10},
    {'\n', 1},
    {' ', 32},
    {'%', 26},
    {' ', 3},
    {'%', 12},
    {' ', 1},
    {'%', 15},
    {' ', 11},
    {'\n', 1},
    {' ', 31},
    {'%', 29},
    {' ', 1},
    {'%', 19},
    {' ', 5},
    {'%', 3},
    {' ', 12},
    {'\n', 1},
    {' ', 29},
    {'%', 28},
    {'@', 1},
    {' ', 1},
    {'@', 1},
    {'%', 18},
    {' ', 8},
    {'%', 2},
    {' ', 12},
    {'\n', 1},
    {' ', 28},
    {'%', 33},
    {' ', 1},
    {'%', 22},
    {' ', 16},
    {'\n', 1},
    {' ', 28},
    {'%', 58},
    {' ', 14},
    {'\n', 1},
    {' ', 28},
    {'%', 50},
    {'@', 1},
    {'%', 6},
    {'@', 1},
    {' ', 14},
    {'\n', 1},
    {' ', 6},
    {'%', 8},
    {'@', 1},
    {' ', 11},
    {'%', 16},
    {' ', 8},
    {'%', 26},
    {' ', 6},
    {'%', 2},
    {' ', 16},
    {'\n', 1},
    {' ', 4},
    {'%', 13},
    {' ', 9},
    {'%', 2},
    {'@', 1},
    {'%', 12},
    {' ', 11},
    {'%', 11},
    {' ', 1},
    {'%', 12},
    {' ', 6},
    {'@', 1},
    {'%', 1},
    {' ', 16},
    {'\n', 1},
    {' ', 2},
    {'%', 10},
    {' ', 3},
    {'%', 3},
    {' ', 8},
    {'%', 14},
    {' ', 12},
    {'%', 24},
    {' ', 24},
    {'\n', 1},
    {' ', 1},
    {'%', 9},
    {' ', 7},
    {'%', 1},
    {' ', 9},
    {'%', 13},
    {' ', 13},
    {'%', 12},
    {'@', 1},
    {'%', 11},
    {' ', 23},
    {'\n', 1},
    {'%', 9},
    {'@', 1},
    {' ', 16},
    {'%', 1},
    {' ', 1},
    {'%', 13},
    {' ', 12},
    {'@', 1},
    {'%', 25},
    {' ', 20},
    {'\n', 1},
    {'%', 8},
    {'@', 1},
    {' ', 17},
    {'%', 2},
    {'@', 1},
    {'%', 12},
    {' ', 12},
    {'@', 1},
    {'%', 28},
    {' ', 17},
    {'\n', 1},
    {'%', 7},
    {'@', 1},
    {' ', 19},
    {'%', 15},
    {' ', 11},
    {'%', 33},
    {' ', 13},
    {'\n', 1},
    {'%', 10},
    {' ', 18},
    {'%', 15},
    {' ', 10},
    {'%', 35},
    {' ', 6},
    {'%', 4},
    {' ', 2},
    {'\n', 1},
    {'%', 9},
    {'@', 1},
    {' ', 19},
    {'@', 1},
    {'%', 14},
    {' ', 9},
    {'%', 12},
    {'@', 1},
    {' ', 1},
    {'%', 4},
    {' ', 1},
    {'%', 17},
    {' ', 3},
    {'%', 8},
    {'\n', 1},
    {'%', 10},
    {' ', 18},
    {'%', 17},
    {' ', 8},
    {'%', 13},
    {' ', 6},
    {'%', 18},
    {' ', 1},
    {'%', 9},
    {'\n', 1},
    {'%', 9},
    {'@', 1},
    {'%', 2},
    {'@', 1},
    {' ', 16},
    {'%', 16},
    {'@', 1},
    {' ', 7},
    {'%', 14},
    {' ', 5},
    {'%', 24},
    {' ', 2},
    {'%', 2},
    {'\n', 1},
    {' ', 1},
    {'%', 10},
    {' ', 18},
    {'%', 1},
    {' ', 1},
    {'%', 14},
    {'@', 1},
    {' ', 8},
    {'%', 14},
    {' ', 3},
    {'%', 26},
    {' ', 1},
    {'%', 2},
    {'\n', 1},
    {' ', 2},
    {'%', 12},
    {' ', 2},
    {'@', 1},
    {' ', 11},
    {'%', 18},
    {' ', 8},
    {'%', 40},
    {' ', 2},
    {'%', 3},
    {' ', 1},
    {'\n', 1},
    {' ', 3},
    {'%', 13},
    {' ', 1},
    {'%', 2},
    {' ', 2},
    {'%', 1},
    {' ', 2},
    {'%', 1},
    {'@', 1},
    {' ', 1},
    {'%', 18},
    {' ', 10},
    {'%', 37},
    {' ', 4},
    {'%', 3},
    {' ', 1},
    {'\n', 1},
    {' ', 4},
    {'%', 18},
    {' ', 1},
    {'%', 22},
    {' ', 11},
    {'@', 1},
    {'%', 31},
    {' ', 4},
    {'%', 7},
    {' ', 1},
    {'\n', 1},
    {' ', 5},
    {'%', 39},
    {' ', 14},
    {'%', 28},
    {' ', 8},
    {'%', 3},
    {' ', 3},
    {'\n', 1},
    {' ', 6},
    {'@', 1},
    {'%', 35},
    {' ', 18},
    {'%', 25},
    {' ', 15},
    {'\n', 1},
    {' ', 8},
    {'%', 32},
    {' ', 22},
    {'%', 19},
    {' ', 2},
    {'%', 7},
    {' ', 10},
    {'\n', 1},
    {' ', 11},
    {'%', 26},
    {' ', 27},
    {'%', 15},
    {' ', 2},
    {'@', 1},
    {'%', 9},
    {' ', 9},
    {'\n', 1},
    {' ', 14},
    {'%', 20},
    {' ', 11},
    {'@', 1},
    {'%', 1},
    {'@', 1},
    {'%', 1},
    {' ', 18},
    {'@', 1},
    {'%', 18},
    {' ', 3},
    {'%', 3},
    {' ', 8},
    {'\n', 1},
    {' ', 18},
    {'%', 15},
    {' ', 8},
    {'%', 10},
    {' ', 20},
    {'%', 15},
    {' ', 4},
    {'%', 1},
    {' ', 9},
    {'\n', 1},
    {' ', 16},
    {'%', 36},
    {' ', 22},
    {'%', 14},
    {' ', 12},
    {'\n', 1},
    {' ', 16},
    {'%', 26},
    {' ', 2},
    {'%', 4},
    {' ', 1},
    {'%', 3},
    {' ', 22},
    {'%', 10},
    {' ', 2},
    {'%', 3},
    {'@', 1},
    {' ', 10},
    {'\n', 1},
    {' ', 21},
    {'%', 19},
    {' ', 1},
    {'%', 6},
    {' ', 1},
    {'%', 2},
    {' ', 26},
    {'%', 13},
    {'@', 1},
    {' ', 10},
    {'\n', 1},
    {' ', 81},
    {'%', 7},
    {'@', 1},
    {' ', 10},
    {'\n', 1},
    {' ', 0}  
};

extern void print_dragon() {
    const rle_pair_t *ptr = DREXEL_DRAGON_RLE;
    while (ptr->count != 0) {  
        for (int i = 0; i < ptr->count; i++) {
            putchar(ptr->ch);  
        }
        ptr++; 
    }
}
