#include "types.h"
#include "constants.h"

#include <Windows.h>
#include <stdlib.h>
#include <time.h>

#include "gameoflife.c"

struct window
{
    HWND hwnd;
    HDC dc;
    i32 width;
    i32 height;
    i32 clientWidth;
    i32 clientHeight;
};

struct bm_buffer
{
    BITMAPINFO bmInfo;
    u8 *pixels;
};

struct window g_main_window;
struct bm_buffer g_main_window_buffer;

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch(uMsg)
    {
        case WM_PAINT:
        {
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        };

        case WM_WINDOWPOSCHANGED:
        case WM_SIZE:
        {
            RECT clientArea;
            GetClientRect(g_main_window.hwnd, &clientArea);

            g_main_window.clientWidth = clientArea.right - clientArea.left;
            g_main_window.clientHeight = clientArea.bottom - clientArea.top;
            
            g_main_window_buffer.bmInfo.bmiHeader.biWidth = g_main_window.clientWidth;
            g_main_window_buffer.bmInfo.bmiHeader.biHeight = g_main_window.clientHeight;

            g_main_window_buffer.pixels = realloc(g_main_window_buffer.pixels, 
                g_main_window.clientWidth*g_main_window.clientHeight*(g_main_window_buffer.bmInfo.bmiHeader.biBitCount/8));
        } break;

        default:
        {
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        } break;
    }

    return result;
}

int WINAPI 
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd)
{
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = APP_NAME;
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;

    RegisterClassA(&wc);

    g_main_window.hwnd = CreateWindowA(APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 
        CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
    if (!g_main_window.hwnd)
    {
        return -1;
    }
    else
    {
        RECT clientArea;
        GetClientRect(g_main_window.hwnd, &clientArea);

        g_main_window.clientWidth = clientArea.right - clientArea.left;
        g_main_window.clientHeight = clientArea.bottom - clientArea.top;

        g_main_window.dc = GetDC(g_main_window.hwnd);
    }

    g_main_window_buffer.bmInfo.bmiHeader.biSize = sizeof(g_main_window_buffer.bmInfo);
    g_main_window_buffer.bmInfo.bmiHeader.biBitCount = 32;
    g_main_window_buffer.bmInfo.bmiHeader.biWidth = g_main_window.clientWidth;
    g_main_window_buffer.bmInfo.bmiHeader.biHeight = g_main_window.clientHeight;
    g_main_window_buffer.bmInfo.bmiHeader.biCompression = BI_RGB;
    g_main_window_buffer.bmInfo.bmiHeader.biPlanes = 1;

    g_main_window_buffer.pixels = malloc(g_main_window_buffer.bmInfo.bmiHeader.biWidth*
        g_main_window_buffer.bmInfo.bmiHeader.biHeight*(g_main_window_buffer.bmInfo.bmiHeader.biBitCount/8));
    
    ShowWindow(g_main_window.hwnd, nShowCmd);

    srand((u32)time(NULL));

    const char *entityAllocGrid = 
        " x   "
        "  x  "
        "xxx  "
        "   x "
        "    x"
        "  xxx"
    ;
    gol_init(entityAllocGrid, 5, 6);

    LARGE_INTEGER perfFreq;
    QueryPerformanceFrequency(&perfFreq);

    LARGE_INTEGER elapsedTicks = {0};

    gol_draw(g_main_window_buffer.pixels, g_main_window_buffer.bmInfo.bmiHeader.biWidth, 
        g_main_window_buffer.bmInfo.bmiHeader.biHeight, g_main_window_buffer.bmInfo.bmiHeader.biBitCount/8);

    b32 isRunning = B32_TRUE;
    while (isRunning)
    {
        LARGE_INTEGER startTicks;
        QueryPerformanceCounter(&startTicks);

        MSG msg;

        while (PeekMessageA(&msg, g_main_window.hwnd, 0, 0, PM_REMOVE))
        {
            switch(msg.message)
            {
                case WM_KEYDOWN:
                {
                    if (msg.wParam == VK_ESCAPE && (*((u16 *)&msg.lParam) == 1))
                    {
                        isRunning = B32_FALSE;
                    }
                } break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        i64 elapsedMilliseconds = elapsedTicks.QuadPart/(perfFreq.QuadPart/1000);
        while (elapsedMilliseconds >= (i64)MS_PER_FRAME)
        {
            elapsedMilliseconds -= (i64)MS_PER_FRAME;
            gol_update();
            gol_draw(g_main_window_buffer.pixels, g_main_window_buffer.bmInfo.bmiHeader.biWidth, 
                g_main_window_buffer.bmInfo.bmiHeader.biHeight, g_main_window_buffer.bmInfo.bmiHeader.biBitCount/8);

            elapsedTicks.QuadPart = 0;
        }
        
        StretchDIBits(g_main_window.dc, 0, 0, g_main_window.clientWidth, g_main_window.clientHeight, 0, 0,
            g_main_window_buffer.bmInfo.bmiHeader.biWidth, g_main_window_buffer.bmInfo.bmiHeader.biHeight,
            g_main_window_buffer.pixels, &g_main_window_buffer.bmInfo, DIB_RGB_COLORS, SRCCOPY);
        
        LARGE_INTEGER endTicks;
        QueryPerformanceCounter(&endTicks);

        elapsedTicks.QuadPart += endTicks.QuadPart - startTicks.QuadPart;
    }

    return 0;
}