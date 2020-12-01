#include "b6502/display.h"

#include "b6502/base.h"
#include "b6502/rc.h"

static void deinit(void* obj) {
  Display* display = obj;
  SDL_DestroyTexture(display->tex);
  SDL_DestroyRenderer(display->rend);
  SDL_DestroyWindow(display->win);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

Display* display_create(const char* title, int width, int height, int scale) {
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    LOG_ERROR("Could not initialize SDL! %s\n", SDL_GetError());
    goto initerr;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  Display* display = rc_alloc(sizeof(*display), deinit);
  display->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  width * scale, height * scale, 0);

  if (!display->win) {
    LOG_ERROR("Could not create SDL_Window! %s\n", SDL_GetError());
    goto winerr;
  }

  display->rend
      = SDL_CreateRenderer(display->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_RenderSetLogicalSize(display->rend, width, height);

  if (!display->rend) {
    LOG_ERROR("Could not create SDL_Renderer! %s\n", SDL_GetError());
    goto renderr;
  }

  display->tex = SDL_CreateTexture(display->rend, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING, width, height);

  if (!display->tex) {
    LOG_ERROR("Could not create SDL_Texture! %s\n", SDL_GetError());
    goto texerr;
  }

  return display;

texerr:
  SDL_DestroyRenderer(display->rend);
renderr:
  SDL_DestroyWindow(display->win);
winerr:
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
initerr:
  return NULL;
}

void update(Display* display, const void* pixels, int pitch) {
  SDL_UpdateTexture(display->tex, NULL, pixels, pitch);
  SDL_RenderClear(display->rend);
  SDL_RenderCopy(display->rend, display->tex, NULL, NULL);
  SDL_RenderPresent(display->rend);
}
