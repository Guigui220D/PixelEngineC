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
    if (i >= 100)
    {
        i = 0;
        printf("yoo\n");
        window->update_screen = TRUE;
    }
    else
    {
        i++;
        PE_Image_SetPixel(&window->render_screen, rand() % 100, rand() % 100, PE_RED);
    }
}
