#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "ANativeWindow.h"
#include <SDL2/SDL.h>

typedef struct nativeWindow {
    SDL_Window* sdl_window;
    SDL_GLContext* gl_context;
} nativeWindow;

ANativeWindow * ANativeWindow_create() {
    nativeWindow win;

    ANativeWindow *ret = (ANativeWindow *) malloc(sizeof(nativeWindow));
    memcpy(ret, &win, sizeof(nativeWindow));
    ret->sdl_window = SDL_CreateWindow("FalsoNDK", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        960, 544, 
        SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    ret->gl_context = SDL_GL_CreateContext(ret->window);
    SDL_GL_MakeCurrent(ret->window, ret->gl_context);

    return ret;
}

int32_t ANativeWindow_getWidth(ANativeWindow* window) {
    int w;
    SDL_GetWindowSizeInPixels(window, &w, NULL);
    return w;
}

int32_t ANativeWindow_getHeight(ANativeWindow* window) {
    int h;
    SDL_GetWindowSizeInPixels(window, NULL, &h);
    return h;
}

int32_t ANativeWindow_getFormat(ANativeWindow* window) {
    return 1; // AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM
}

int32_t ANativeWindow_setBuffersGeometry(ANativeWindow* window,
                                         int32_t width, int32_t height, int32_t format) {
    return 0;
}
