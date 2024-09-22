#include "nvg.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#define TARGET_FPS 30
#define FRAME_TIME (1000 / TARGET_FPS) // in milliseconds

int blowup = 0;
int screenshot = 0;
int premult = 0;

static void render_frame(NVGcontext *vg) {
  float lineh;
  const char *text = "red blue green .sdsdfsdfsdafasdjhsakdfhasdfk";
  const char *start;
  const char *end;

  nvgSave(vg);
  nvgFontSize(vg, 16.0f);
  nvgFontFace(vg, "mono");
  nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  nvgTextMetrics(vg, NULL, NULL, &lineh);

  start = text;
  end = text + strlen(text);
  float y = 100;
  nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
  nvgText(vg, 200, y, start, end);

  nvgRestore(vg);
}

int main() {
  SDL_Window *window;
  SDL_GLContext gl_context;
  NVGcontext *vg = NULL;
  PerfGraph fps;
  Uint32 prevt = 0;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Failed to init SDL: %s\n", SDL_GetError());
    return -1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  window =
      SDL_CreateWindow("NanoVG", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 1000, 600, SDL_WINDOW_OPENGL);
  if (!window) {
    printf("Failed to create window: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    printf("Failed to create GL context: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  if (vg == NULL) {
    printf("Could not init nanovg.\n");
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  initGraph(&fps, GRAPH_RENDER_FPS, "Frame Time");

  SDL_GL_SetSwapInterval(0);
  prevt = SDL_GetTicks();

  int running = 1;
  Uint32 lastFrameTime = SDL_GetTicks();

  while (running) {
    SDL_Event event;
    int timeout = FRAME_TIME - (SDL_GetTicks() - lastFrameTime);

    if (timeout < 0) {
      timeout = 0;
    }

    if (SDL_WaitEventTimeout(&event, timeout)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
        // Handle other events here
      }
    }

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastFrameTime >= FRAME_TIME) {

      Uint32 t = SDL_GetTicks();
      double dt = (t - prevt) / 1000.0;
      prevt = t;
      updateGraph(&fps, dt);

      int winWidth, winHeight;
      SDL_GetWindowSize(window, &winWidth, &winHeight);

      int fbWidth, fbHeight;
      SDL_GL_GetDrawableSize(window, &fbWidth, &fbHeight);

      float pxRatio = (float)fbWidth / (float)winWidth;

      glViewport(0, 0, fbWidth, fbHeight);
      glClearColor(0.98f, 0.94f, 0.84f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
              GL_STENCIL_BUFFER_BIT);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);

      nvgBeginFrame(vg, winWidth, winHeight, pxRatio);
      render_frame(vg);
      nvgEndFrame(vg);

      glEnable(GL_DEPTH_TEST);

      SDL_GL_SwapWindow(window);

      lastFrameTime = currentTime;
    }
  }

  nvgDeleteGLES3(vg);
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
