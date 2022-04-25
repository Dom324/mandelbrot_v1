
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL.h>      //Library to render a window
#include <cpuid.h>    //Identify AVX2 support

#define HEIGHT 1080
#define WIDTH 1920
#define NUM_ITERATIONS 100

#define RE_MIN -2
#define RE_MAX 1.0

#define IM_MIN -1
#define IM_MAX 1.0

//Function to determine AVX2 support
static unsigned int is_avx2_supported(){

  unsigned int eax, ebx, ecx, edx;

  __get_cpuid(7, &eax, &ebx, &ecx, &edx);

  //printf("ebx %d %x\n", ebx, ebx);

  return ((ebx & bit_AVX2) ? 1 : 0);

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

__m256i vec_mandel(__m256d cre, __m256d cim){

  union four_doubles Zre, Zim;
  Zre.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  Zim.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  //printf("c in %lf %lfi out %lf %lfi\n", vecC.arr[0], vecC.arr[1], vecC.arr[2], vecC.arr[3]);

  union four_ints res;
  res.vec = _mm256_set_epi64x(0.0, 0.0, 0.0, 0.0);
  __m256i iterator = _mm256_set_epi64x(0, 0, 0, 0);

  __m256d Zre2 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  __m256d Zim2 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);

  __m256d two = _mm256_set_pd(2.0, 2.0, 2.0, 2.0);

  for(int p = 1; p <= NUM_ITERATIONS; p++){

    __m256d twoX = _mm256_add_pd(Zre.vec, Zre.vec);
    Zim.vec = _mm256_mul_pd(Zim.vec, twoX);
    Zim.vec = _mm256_add_pd(Zim.vec, cim);

    Zre.vec = _mm256_sub_pd(Zre2, Zim2);
    Zre.vec = _mm256_add_pd(Zre.vec, cre);

    Zre2 = _mm256_mul_pd(Zre.vec, Zre.vec);
    Zim2 = _mm256_mul_pd(Zim.vec, Zim.vec);

    __m256d abs = _mm256_add_pd(Zre2, Zim2);
    abs = _mm256_sqrt_pd(abs);

    union four_ints mask;
    mask.vec = (__m256i)_mm256_cmp_pd(abs, two, _CMP_LT_OQ);

    res.arr[0] = mask.arr[0] ? p : res.arr[0];
    res.arr[1] = mask.arr[1] ? p : res.arr[1];
    res.arr[2] = mask.arr[2] ? p : res.arr[2];
    res.arr[3] = mask.arr[3] ? p : res.arr[3];

    if( ( mask.arr[0] | mask.arr[1] | mask.arr[2] | mask.arr[3] ) == 0 ) break;

  }

  return res.vec;

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

  if(1){
    //AVX2 instrukce jsou podporovany, rychla cesta

    res.vec = vec_mandel(cre.vec, cim.vec);

    return res.vec;

  }
  else{

    //AVX2 instrukce nejsou podporovany, pomala cesta
    res.arr[0] = scalar_mandel(cre.arr[0], cim.arr[0]);
    res.arr[1] = scalar_mandel(cre.arr[1], cim.arr[1]);
    res.arr[2] = scalar_mandel(cre.arr[2], cim.arr[2]);
    res.arr[3] = scalar_mandel(cre.arr[3], cim.arr[3]);

    return res.vec;

  }

}

void calculate_frame(pixel color[HEIGHT][WIDTH], double centerX, double centerY, double zoom, int is_avx2){

  pixel palette[NUM_ITERATIONS];

  for(int x = 0; x < NUM_ITERATIONS; x++) {
    palette[x].r = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 16.0));
    palette[x].g = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 32.0));
    palette[x].b = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 64.0));
  }

  double x_min = (RE_MIN + centerX);
  double x_max = (RE_MAX + centerX);
  double y_min = (IM_MIN + centerY);
  double y_max = (IM_MAX + centerY);

  double x_zoom = (x_max - x_min) * (zoom - 1) * 0.5;
  double y_zoom = (y_max - y_min) * (zoom - 1) * 0.5;

  x_min = x_min - x_zoom;
  x_max = x_max + x_zoom;
  y_min = y_min - y_zoom;
  y_max = y_max + y_zoom;

  printf("xmin %lf xmax %lf ymin %lf ymax %lf\n", x_min, x_max, y_min, y_max);

  static double arr_re[HEIGHT][WIDTH];
  static double arr_im[HEIGHT][WIDTH];

  //vypocet souradnic pixelu
  for (size_t i = 0; i < HEIGHT; i++){
    for (size_t j = 0; j < WIDTH; j++){

      double y =  (double)i / HEIGHT;
      double x =  (double)j / WIDTH;

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

        double range = (double)res.arr[ii] / NUM_ITERATIONS;
        int index = 255 - (unsigned char)(range * 255.0);

        color[i][j + ii].r = index;
        //color[i][j + ii].r = palette[res.arr[ii]].r;
        color[i][j + ii].b = index;
        color[i][j + ii].g = index;
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

  /*SDL_Keycode commands[] = {SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_q};
  int i = 0;*/

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

    SDL_WaitEvent(&event);

    //event.key.keysym.sym = commands[i];
    //i++;

    switch (event.type){
    //switch (SDL_KEYDOWN){

      case SDL_QUIT:
        quit = 1;
      break;

      case SDL_KEYDOWN:

        if(event.key.keysym.sym == SDLK_q){
          quit = 1;
        }
        else if(event.key.keysym.sym == SDLK_UP){
          centerY = centerY - 0.2 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_DOWN){
          centerY = centerY + 0.2 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_RIGHT){
          centerX = centerX + 0.2 * zoom;
          update = 1;
        }
        else if(event.key.keysym.sym == SDLK_LEFT){
          centerX = centerX - 0.2 * zoom;
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
