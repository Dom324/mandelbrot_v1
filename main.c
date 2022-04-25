
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL.h>      //Library to render a window
#include <cpuid.h>    //Identify AVX2 support

#define HEIGHT 1080
#define WIDTH 1920
#define NUM_ITERATIONS 150

#define RE_MIN -2
#define RE_MAX 1.0

#define IM_MIN -1
#define IM_MAX 1.0

//Function to determine AVX2 support
static inline int is_avx2_supported(){

  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  const int level = 7;

  __get_cpuid(level, &eax, &ebx, &ecx, &edx);

  return (ecx & bit_AVX2) ? 1 : 0;

}

typedef struct pixel{

  unsigned char r;
  unsigned char g;
  unsigned char b;

} pixel;

__m256d mult(__m256d a, __m256d b){

  __m256d bSwap = _mm256_shuffle_pd(b, b, 5);

  __m256d aIm = _mm256_shuffle_pd(a, a, 15);

  __m256d aRe = _mm256_shuffle_pd(a, a, 0);

  __m256d aIm_bSwap = _mm256_mul_pd(aIm, bSwap);

  return _mm256_fmaddsub_pd(aRe, b, aIm_bSwap);

}

union four_doubles{
  __m256d vec;
  double arr[4];
};

union four_ints{
  __m256i vec;
  long long arr[4];
};

__m256i vec_mandel(double cre, double cim){

  union four_doubles vecC, vecZ;
  vecC.vec = _mm256_set_pd(0.0, 0.0, cim, cre);
  vecZ.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  //printf("c in %lf %lfi out %lf %lfi\n", vecC.arr[0], vecC.arr[1], vecC.arr[2], vecC.arr[3]);

  __m256i res = _mm256_set_epi64x(NUM_ITERATIONS, NUM_ITERATIONS, NUM_ITERATIONS, NUM_ITERATIONS);

  for(int p = 0; p < NUM_ITERATIONS; p++){

    //z = (z * z) + c;
    /*printf("%.1f%+.1fi cartesian is rho=%f theta=%f polar\n",
         creal(z), cimag(z), cabs(z), carg(z));*/
    //if(cabs(z) > 2) return p;

    vecZ.vec = _mm256_add_pd(vecC.vec, mult(vecZ.vec, vecZ.vec));

    /*printf("%d c in %lf %lfi out %lf %lfi\n", p, vecZ.arr[0], vecZ.arr[1], vecZ.arr[2], vecZ.arr[3]);
          getchar();*/

    if( sqrt(vecZ.arr[0] * vecZ.arr[0] + vecZ.arr[1] * vecZ.arr[1]) > 2.0){
      res = _mm256_set_epi64x(p, p, p, p);
      return res;
    }

  }

  return res;

}

long long scalar_mandel(double cre, double cim){

  double zre = 0.0;
  double zim = 0.0;

  for(int p = 0; p < NUM_ITERATIONS; p++){

    //z = (z * z) + c;
    double zre_temp = (zre * zre - zim * zim) + cre;
    zim = (2 * zre * zim) + cim;
    zre = zre_temp;

    /*printf("%d %.1f%+.1fi\n",p, zre, zim);
    getchar();*/

    if(sqrt(zre * zre + zim * zim) > 2.0) return p;

  }

  return NUM_ITERATIONS;

}

__m256i mandel(union four_doubles cre, union four_doubles cim, int is_avx2){

  union four_ints res;

  /*if(is_avx2){
    //AVX2 instrukce jsou podporovany, rychla cesta

    res.vec = vec_mandel(cre, cim);

    return res.vec;

  }
  else{*/

    //AVX2 instrukce nejsou podporovany, pomala cesta
    res.arr[0] = scalar_mandel(cre.arr[0], cim.arr[0]);
    res.arr[1] = scalar_mandel(cre.arr[1], cim.arr[1]);
    res.arr[2] = scalar_mandel(cre.arr[2], cim.arr[2]);
    res.arr[3] = scalar_mandel(cre.arr[3], cim.arr[3]);

    return res.vec;

  //}

}

void calculate_frame(pixel color[HEIGHT][WIDTH], double centerX, double centerY, double zoom, int is_avx2){

  /*pixel palette[NUM_ITERATIONS];

  for(int x = 0; x < NUM_ITERATIONS; x++) {
    palette[x].r = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 16.0));
    palette[x].g = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 32.0));
    palette[x].b = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 64.0));
  }*/

  double x_min = (RE_MIN + centerX) * zoom;
  double x_max = (RE_MAX + centerX) * zoom;
  double y_min = (IM_MIN + centerY) * zoom;
  double y_max = (IM_MAX + centerY) * zoom;

  static double arr_re[HEIGHT][WIDTH];
  static double arr_im[HEIGHT][WIDTH];

  //vypocet souradnic pixelu
  for (size_t i = 0; i < HEIGHT; i++){
    for (size_t j = 0; j < WIDTH; j++){

      double y =  zoom * (double)i / HEIGHT;
      double x =  zoom * (double)j / WIDTH;

      arr_re[i][j] = x_min + x * (x_max - x_min);
      arr_im[i][j] = y_min + y * (y_max - y_min);

    }
  }

  //vypocet iteraci pro jednotlive pixely
  for (size_t i = 0; i < HEIGHT; i++){
    for (size_t j = 0; j < WIDTH; j = j + 4){

      union four_ints res;
      union four_doubles cre, cim;
      cre.vec = _mm256_set_pd(arr_re[i][j + 3], arr_re[i][j + 2], arr_re[i][j + 1], arr_re[i][j]);
      cim.vec = _mm256_set_pd(arr_im[i][j + 3], arr_im[i][j + 2], arr_im[i][j + 1], arr_im[i][j]);

      res.vec = mandel(cre, cim, is_avx2);

      for(int ii = 0; ii < 4; ii++){
        color[i][j].r = (unsigned char)((double)res.arr[ii] * 255.0 / NUM_ITERATIONS);
        color[i][j].b = (unsigned char)((double)res.arr[ii] * 255.0 / NUM_ITERATIONS);
        color[i][j].g = (unsigned char)((double)res.arr[ii] * 255.0 / NUM_ITERATIONS);
      }
      //printf("%d \n",color[i][j].r);

    }
  }

}

int main() {

  int is_avx2 = is_avx2_supported();

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
  SDL_Event event;
  int quit = 0;
  int update = 1;

  double zoom = 1.0;
  double centerX = 0;
  double centerY = 0;

  SDL_Keycode commands[] = {SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_q};
  int i = 0;

  while (!quit)
  {

    if(update){
      static pixel color[HEIGHT][WIDTH];

      calculate_frame(color, centerX, centerY, zoom, is_avx2);

      SDL_ConvertPixels(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGB24, color, WIDTH * 3, SDL_PIXELFORMAT_RGBA32, image->pixels, image->pitch);
      texture = SDL_CreateTextureFromSurface(renderer, image);

      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      update = 0;
    }

    //SDL_WaitEvent(&event);

    event.key.keysym.sym = commands[i];
    i++;

    //switch (event.type){
    switch (SDL_KEYDOWN){

      case SDL_QUIT:
        quit = 1;
      break;

      case SDL_KEYDOWN:

        if(event.key.keysym.sym == SDLK_q){
          quit = 1;
        }
        else if(event.key.keysym.sym == SDLK_UP){
          centerY = centerY - 0.1 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_DOWN){
          centerY = centerY + 0.1 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_RIGHT){
          centerX = centerX + 0.1 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_LEFT){
          centerX = centerX - 0.1 * zoom;
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
