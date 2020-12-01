#pragma once

/**
 * @file base.h
 * @brief Generic SDL2 display struct that can be used by any system
 *
 * The Display struct contains a SDL_Window, SDL_Renderer, and SDL_Texture. Because it will serve as
 * the display for various systems, it strives to be as generic as possible. As various system
 * backends are completed, this file will be updated to uphold its generality.
 *
 */

#include <SDL.h>

/**
 * @brief A generic display struct.
 */
typedef struct Display {
  SDL_Window* win;
  SDL_Renderer* rend;
  SDL_Texture* tex;
} Display;

/**
 * @brief Constructor for the Display struct.
 * @param title The window title.
 * @param width The width of the window.
 * @param height The height of the window.
 * @param scale Scale the width and height by this value. ('1' for no scale).
 */
Display* display_create(const char* title, int width, int height, int scale);

/**
 * @brief Update the texture and present the frame.
 * @param display The display struct.
 * @param pixels The pixels to present to the screen.
 * @param pitch Byte length of one line of pixels
 */
void update(Display* display, const void* pixels, int pitch);
