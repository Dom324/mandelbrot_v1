
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <complex.h>

#include <stdbool.h>

#include <SDL.h>

#define HEIGHT 1080
#define WIDTH 1920
#define NUM_ITERATIONS 100

#define RE_MIN -2
#define RE_MAX 1.0

#define IM_MIN -1
#define IM_MAX 1.0

typedef struct pixel{

  unsigned char r;
  unsigned char g;
  unsigned char b;

} pixel;

double complex mandel(double complex c){

  double complex z = 0;

  for(int p = 0; p < NUM_ITERATIONS; ++p){

    z = (z * z) + c;
    /*printf("%.1f%+.1fi cartesian is rho=%f theta=%f polar\n",
         creal(z), cimag(z), cabs(z), carg(z));*/

    if(cabs(z) > 2) return p;

  }

  return NUM_ITERATIONS;

}

void calculate_frame(pixel color[HEIGHT][WIDTH], double complex center, double zoom){

  pixel palette[NUM_ITERATIONS];

  for(int x = 0; x < NUM_ITERATIONS; x++) {
    palette[x].r = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 16.0));
    palette[x].g = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 32.0));
    palette[x].b = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 64.0));
  }

  double x_min = (RE_MIN + creal(center)) * zoom;
  double x_max = (RE_MAX + creal(center)) * zoom;
  double y_min = (IM_MIN + cimag(center)) * zoom;
  double y_max = (IM_MAX + cimag(center)) * zoom;

  static double complex arr[WIDTH][HEIGHT];

  for (size_t i = 0; i < WIDTH; i++) {
    for (size_t j = 0; j < HEIGHT; j++) {

      double x =  zoom * (double)i / WIDTH;
      double y =  zoom * (double)j / HEIGHT;

      arr[i][j] = x_min + x * (x_max - x_min) + y_min * I + y * (y_max - y_min) * I;

      //printf("%.1f%+.1fi cartesian is rho=%f theta=%f polar\n",
      //     creal(arr[i][j]), cimag(arr[i][j]), cabs(arr[i][j]), carg(arr[i][j]));

      arr[i][j] = mandel(arr[i][j]);

      //printf("%.1f%+.1fi cartesian is rho=%f theta=%f polar\n",
      //     creal(arr[i][j]), cimag(arr[i][j]), cabs(arr[i][j]), carg(arr[i][j]));

      color[j][i].r = (unsigned char)255 - (unsigned char)(cabs(arr[i][j]) * 255.0 / NUM_ITERATIONS);
      //printf("%d \n",color[i][j].r);
      color[j][i].b = palette[(int)cabs(arr[i][j])].b;
      color[j][i].g = palette[(int)cabs(arr[i][j])].g;

    }
  }

}

int main() {

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window * window = SDL_CreateWindow("SDL2 Displaying Image",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);

  if(window == NULL){
    printf("%s", SDL_GetError());
    return 0;
  }

  SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Surface * image = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
  SDL_Texture * texture;

  bool quit = false;
  bool update = true;
  double zoom = 1.0;
  double complex center = 0;
  SDL_Event event;

  while (!quit)
  {

    if(update){
      static pixel color[HEIGHT][WIDTH];

      calculate_frame(color, center, zoom);

      SDL_ConvertPixels(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGB24, color, WIDTH * 3, SDL_PIXELFORMAT_RGBA32, image->pixels, image->pitch);
      texture = SDL_CreateTextureFromSurface(renderer, image);

      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      update = false;
    }

    SDL_WaitEvent(&event);

    switch (event.type){

      case SDL_QUIT:
        quit = true;
      break;

      case SDL_KEYDOWN:

        if(event.key.keysym.sym == SDLK_q){
          quit = 1;
        }
        else if(event.key.keysym.sym == SDLK_UP){
          center = center - 0.1 * zoom * I;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_DOWN){
          center = center + 0.1 * zoom * I;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_RIGHT){
          center = center + 0.1 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_LEFT){
          center = center - 0.1 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_KP_PLUS){
          zoom = zoom - 0.2 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_KP_MINUS){
          zoom = zoom + 0.2 * zoom;
          update = 1;
        }
      break;

      }
  }

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(image);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}
