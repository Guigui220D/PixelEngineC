#include "PixelEngine.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

void update(PE_Window* window, float delta);

int main()
{
    srand(time(NULL));

    PE_Window win = PE_Window_Create("Pixel Engine", 400, 400, 100, 100, update);

    PE_Window_Start(&win);

    return 0;
}

void update(PE_Window* window, float delta)
{
    static int i = 0;
    if (i >= 20)
    {
        i = 0;
        //printf("yoo\n");
        PE_Color col;
        col.r = rand() % 256;
        col.g = rand() % 256;
        col.b = rand() % 256;
        col.a = 0xFF;
        PE_Image_DrawRectangle(&window->render_screen, rand() % 100, rand() % 100, rand() % 30, rand() % 30, col);
        window->update_screen = TRUE;
    }
    i++;
}
