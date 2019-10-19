#include "PixelEngine.h"

int main()
{
    PE_Window win = PE_Window_Create("Pixel Engine", 400, 400, 100, 100);

    PE_Window_Start(&win);

    return 0;
}
