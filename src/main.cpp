#include "SDL2/SDL.h"
#include "mmu.hpp"
#include "ppu.hpp"

#include "vt168.hpp"
#include <iomanip>
#include <iostream>
using namespace std;
using namespace VTxx;

SDL_Window *ppu_window;
SDL_Renderer *ppuwin_renderer;

int main(int argc, const char *argv[]) {
  ppu_window = SDL_CreateWindow("openvtx", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, 256, 240, 0);
  if (ppu_window == nullptr) {
    printf("Failed to create window: %s.\n", SDL_GetError());
    exit(1);
  }
  ppuwin_renderer =
      SDL_CreateRenderer(ppu_window, -1, SDL_RENDERER_ACCELERATED);

  vt168_init(VT168_Platform::VT168_BASE, argv[1]);
  cout << "vector = 0x" << hex
       << (read_mem_virtual(0xfffd) << 8UL | read_mem_virtual(0xfffc)) << endl;
  bool last_render_done = false;
  SDL_Event event;
  while (true) {
    vt168_tick();
    if (ppu_is_render_done() && !last_render_done) {
      // Process events
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
          ppu_stop();
          return 0;
          break;
        }
      }
      // Render graphics
      SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
          (void *)get_render_buffer(), 256, 240, 32, 256 * 4, 0x00FF0000,
          0x0000FF00, 0x000000FF, 0xFF000000);

      SDL_Texture *tex = SDL_CreateTextureFromSurface(ppuwin_renderer, surf);
      SDL_RenderClear(ppuwin_renderer);
      SDL_RenderCopy(ppuwin_renderer, tex, nullptr, nullptr);
      SDL_RenderPresent(ppuwin_renderer);
      SDL_DestroyTexture(tex);
      SDL_FreeSurface(surf);
    }
    last_render_done = ppu_is_render_done();
  }
}