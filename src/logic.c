#include "logic.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"

// True if all players in a portion of the grid are the same.
bool allElementsEqual(GridPos grid[], int size)
{
    for (int i = 0; i < size; i++)
    {
        if (grid[i].player != grid[0].player)
        {
            return false;
        }
    }
    return true;
}

Player winner(GridPos *grid)
{
    // Check horizontal lines
    for (int row = 0; row < GRID_SIZE; row++)
    {
        GridPos *ptrRowArray = grid + (row * GRID_SIZE);
        if (allElementsEqual(ptrRowArray, GRID_SIZE) && (*ptrRowArray).player != empty)
        {
            return (*ptrRowArray).player;
        }
    }

    // Check vertical lines
    for (int col = 0; col < GRID_SIZE; col++)
    {
        GridPos *ptrColArray = malloc(sizeof(GridPos) * GRID_SIZE);
        for (int i = 0; i < GRID_SIZE; i++)
        {
            ptrColArray[i] = grid[i * GRID_SIZE + col];
        }

        Player player = (*ptrColArray).player;
        bool verticalMatch = allElementsEqual(ptrColArray, 3) && player != empty;
        free(ptrColArray);
        if (verticalMatch)
            return player;
    }

    // Check diagonal lines
    for (int diag = 0; diag < GRID_SIZE; diag++)
    {
        /*
        ┌───┬───┬───┐
        │ x │   │   │
        ├───┼───┼───┤
        │   │ x │   │
        ├───┼───┼───┤
        │   │   │ x │
        └───┴───┴───┘
        */
        GridPos *ptrDiagArrayLtoR = malloc(sizeof(GridPos) * GRID_SIZE);
        /*
        ┌───┬───┬───┐
        │   │   │ x │
        ├───┼───┼───┤
        │   │ x │   │
        ├───┼───┼───┤
        │ x │   │   │
        └───┴───┴───┘
        */
        GridPos *ptrDiagArrayRtoL = malloc(sizeof(GridPos) * GRID_SIZE);
        for (int i = 0; i < GRID_SIZE; i++)
        {
            ptrDiagArrayLtoR[i] = grid[rowColToPos(i, i)];
            ptrDiagArrayRtoL[i] = grid[rowColToPos(GRID_SIZE - i - 1, i)];
        }

        Player player;
        bool diagMatch = true;

        if (allElementsEqual(ptrDiagArrayLtoR, GRID_SIZE) //
            && (player = (*ptrDiagArrayLtoR).player) != empty)
        {
        }
        else if (allElementsEqual(ptrDiagArrayRtoL, GRID_SIZE) //
                 && (player = (*ptrDiagArrayRtoL).player) != empty)
        {
        }
        else
        {
            diagMatch = false;
        }
        free(ptrDiagArrayLtoR);
        free(ptrDiagArrayRtoL);
        if (diagMatch)
            return player;
    }

    return empty;
}

bool canPlayAtPos(int pos, GridPos grid[])
{
    return (grid[pos]).player == empty;
}

bool playPos(Player player, int pos, GridPos grid[])
{
    if (!canPlayAtPos(pos, grid))
    {
        // error
        printf("ERROR: Cannot play at position %d", pos);
        return false;
    }

    (grid[pos]).player = player;
    return true;
}

int aiPlay(GridPos grid[])
{
    // One could implement a better AI algorithm such as minimax
    // https://github.com/britannio/tic-tac-toe
    return nextFreePos(grid);
}

int nextFreePos(GridPos grid[])
{
    for (int i = 0; i < POSITIONS; i++)
    {
        GridPos pos = grid[i];
        if (pos.player == empty)
        {
            return i;
        }
    }
    return -1;
}

int rowColToPos(int row, int col)
{
    return row * 3 + col;
}