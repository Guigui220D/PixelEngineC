/*
    Copyright: Guillaume DEREX, 2020

    DESCRIPTION
    This is a super simple header only library to create a window and draw on it.
    It was heavily inspired by OlcPixelGameEngine.
    It only works on Windows (for the moment)
*/

#ifndef PIXEL_ENGINE
#define PIXEL_ENGINE

#include <windows.h>
#include <gdiplus.h>
#include <GL/gl.h>

#include <stdint.h>
#include <stdlib.h>

#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#include <assert.h>
//Test
#include <stdlib.h>

#include <stdio.h>

/*
    DEFINITIONS
*/

//Color definition
typedef struct
{
    union
    {
        uint32_t _int;
        struct
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
    };
} PE_Color;

//Colors
const PE_Color PE_WHITE =   { ._int = 0xFFFFFFFF };
const PE_Color PE_BLACK =   { ._int = 0xFF000000 };

const PE_Color PE_RED =     { ._int = 0xFF0000FF };
const PE_Color PE_GREEN =   { ._int = 0xFF00FF00 };
const PE_Color PE_BLUE =    { ._int = 0xFFFF0000 };

const PE_Color PE_YELLOW =  { ._int = 0xFF00FFFF };
const PE_Color PE_CYAN =    { ._int = 0xFFFFFF00 };
const PE_Color PE_MAGENTA = { ._int = 0xFFFF00FF };

//Image definition
typedef struct
{
    size_t _priv_width, _priv_height;
    PE_Color* _priv_pixels;
} PE_Image;

//Sprite definition (they're just images)
typedef PE_Image PE_Sprite;

//Image functions definitions
PE_Image PE_Image_Create(size_t width, size_t height);
void PE_Image_Destroy(PE_Image* image);

void PE_Image_SetPixel(PE_Image* image, size_t x, size_t y, PE_Color color);
PE_Color PE_Image_GetPixel(PE_Image* image, size_t x, size_t y);

void PE_Image_DrawRectangle(PE_Image* image, size_t x, size_t y, size_t width, size_t height, PE_Color color);

//Window definitions
typedef struct
{
    //Context & opengl
    HDC _priv_device_context;
    HGLRC _priv_render_context;
    GLuint _priv_gl_buffer;
    HWND _priv_window_handle;
    //Atomic
    atomic_bool _priv_thread_run;
    atomic_bool update_screen;
    //Window
    const char* _priv_window_name;
    //Render area
    PE_Image render_screen;
    size_t _priv_wsizex, _priv_wsizey;
    //Update function
    void (*_priv_update_func)(void* window, float delta);
} PE_Window;

//Window functions definitions
PE_Window PE_Window_Create(const char* window_name, size_t window_width, size_t window_height, size_t render_area_width, size_t render_area_height, void (*update_function)(void* window, float delta));
void PE_Window_Start(PE_Window* window);

void PE_Window_Destroy(PE_Window* window);

void* _priv_PE_Window_Thread(void* arg);

void _priv_PE_Window_CreateWindow(PE_Window* window);
void _priv_PE_Window_CreateOpenGL(PE_Window* window);

LRESULT CALLBACK _priv_PE_Window_WindowEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // PIXEL_ENGINE

/*
    IMPLEMENTATION
*/

#ifndef PIXEL_ENGINE_IMPL
#define PIXEL_ENGINE_IMPL

//Image functions
PE_Image PE_Image_Create(size_t width, size_t height)
{
    PE_Image image;

    image._priv_width = width;
    image._priv_height = height;
    image._priv_pixels = malloc(sizeof(PE_Color) * width * height);

    return image;
}

void PE_Image_Destroy(PE_Image* image)
{
    free(image->_priv_pixels);
}

void PE_Image_SetPixel(PE_Image* image, size_t x, size_t y, PE_Color color)
{
    assert(x < image->_priv_width);
    assert(y < image->_priv_height);

    image->_priv_pixels[x + y * image->_priv_width] = color;
}

PE_Color PE_Image_GetPixel(PE_Image* image, size_t x, size_t y)
{
    assert(x < image->_priv_width);
    assert(y < image->_priv_height);

    return image->_priv_pixels[x + y * image->_priv_width];
}

void PE_Image_DrawRectangle(PE_Image* image, size_t x, size_t y, size_t width, size_t height, PE_Color color)
{
    for (size_t xx = x; xx < x + width; xx++)
    for (size_t yy = y; yy < y + height; yy++)
        if (xx < image->_priv_width && yy < image->_priv_height)
            PE_Image_SetPixel(image, xx, yy, color);
}

//Window functions
PE_Window PE_Window_Create(const char* window_name, size_t window_width, size_t window_height, size_t render_area_width, size_t render_area_height, void (*update_function)(void* window, float delta))
{
    PE_Window window;

    window._priv_window_name = window_name;
    window._priv_thread_run = FALSE;
    window.update_screen = FALSE;
    window.render_screen = PE_Image_Create(render_area_width, render_area_height);
    window._priv_wsizex = window_width;
    window._priv_wsizey = window_height;
    window._priv_update_func = update_function;

    PE_Color color;
    color._int = 0xFFFFFFFF;
    for (int x = 0; x < render_area_width; x++)
    for (int y = 0; y < render_area_height; y++)
    {
        PE_Image_SetPixel(&window.render_screen, x, y, color);
    }


    return window;
}

void PE_Window_Start(PE_Window* window)
{
    _priv_PE_Window_CreateWindow(window);

    pthread_t thread;
    window->_priv_thread_run = TRUE;

    pthread_create(&thread, NULL, _priv_PE_Window_Thread, window);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    pthread_join(thread, NULL);
}

void PE_Window_Destroy(PE_Window* window)
{
    PE_Image_Destroy(&window->render_screen);
}

void* _priv_PE_Window_Thread(void* arg)
{
    PE_Window* window = arg;

    _priv_PE_Window_CreateOpenGL(window);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &window->_priv_gl_buffer);
    glBindTexture(GL_TEXTURE_2D, window->_priv_gl_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->render_screen._priv_width, window->render_screen._priv_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, window->render_screen._priv_pixels);
    //glBindTexture(GL_TEXTURE_2D, 0);

    clock_t delta_t = clock();
    clock_t fps_clock = clock();
    unsigned int fps_count = 0;

    while (window->_priv_thread_run)
    {
        //FPS limiter
        fps_count++;

        unsigned int mils = clock() - delta_t;
        if (mils < 16)
            Sleep(16 - mils);

        if (clock() - fps_clock >= 1000)
        {
            fps_clock = clock();
            printf("%d FPS\n", fps_count);
            fps_count = 0;
        }

        //Updating
        clock_t new_t = clock();
        float elapsed = ((float)new_t - delta_t) / 1000;
        delta_t = new_t;

        window->_priv_update_func(window, elapsed);

        //Drawing
        glViewport(0, 0, window->_priv_wsizex, window->_priv_wsizey);
        if (window->update_screen)
        {
            window->update_screen = FALSE;
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,  window->render_screen._priv_width, window->render_screen._priv_height, GL_RGBA, GL_UNSIGNED_BYTE, window->render_screen._priv_pixels);
        }
        glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex3f(-1.0f, -1.0f, 0.0f);
            glTexCoord2f(0.0, 0.0); glVertex3f(-1.0f,  1.0f, 0.0f);
            glTexCoord2f(1.0, 0.0); glVertex3f( 1.0f,  1.0f, 0.0f);
            glTexCoord2f(1.0, 1.0); glVertex3f( 1.0f, -1.0f, 0.0f);
        glEnd();
        SwapBuffers(window->_priv_device_context);
    }

    wglDeleteContext(window->_priv_render_context);
    PostMessage(window->_priv_window_handle, WM_DESTROY, 0, 0);
    pthread_exit(0); return NULL;
}

void _priv_PE_Window_CreateWindow(PE_Window* window)
{
    WNDCLASS wc;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = _priv_PE_Window_WindowEvent;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.lpszMenuName = NULL;
        wc.hbrBackground = NULL;
        wc.lpszClassName = "PIXEL_ENGINE";

    RegisterClass(&wc);

    DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD style = WS_CAPTION | WS_SYSMENU | WS_VISIBLE;

    //TEMP
    RECT window_rect = { 0, 0, window->_priv_wsizex, window->_priv_wsizey };
    AdjustWindowRectEx(&window_rect, style, FALSE, ex_style);

    window->_priv_window_handle =
        CreateWindowEx(
            ex_style,
            "PIXEL_ENGINE",
            window->_priv_window_name,
            style,
            30,
            30,
            window_rect.right - window_rect.left,
            window_rect.bottom - window_rect.top,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            window
            );
    //SetWindowLong(window->_priv_window_handle, GWL_STYLE, GetWindowLong(window->_priv_window_handle, GWL_STYLE)&~WS_SIZEBOX);
}

void _priv_PE_Window_CreateOpenGL(PE_Window* window)
{
    HDC hdc = GetDC(window->_priv_window_handle);
    window->_priv_device_context = hdc;

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC hglrc = wglCreateContext(hdc);

    window->_priv_render_context = hglrc;

    wglMakeCurrent(hdc, hglrc);

    glViewport(0, 0, window->_priv_wsizex, window->_priv_wsizey);
}

LRESULT CALLBACK _priv_PE_Window_WindowEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PE_Window* window;
    switch (uMsg)
    {
        case WM_CREATE:
            window = (PE_Window*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            break;

		case WM_CLOSE:
		    DestroyWindow(hWnd);
		    break;

		case WM_DESTROY:
		    PostQuitMessage(0);
		    window->_priv_thread_run = FALSE;
		    break;

		default:
		    return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

#endif // PIXEL_ENGINE_IMPL
