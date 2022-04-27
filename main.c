
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL.h>      //Library to render a window
#include <cpuid.h>    //Identify AVX2 support

#define HEIGHT 1080
#define WIDTH 1920
#define NUM_ITERATIONS_MAX 180

#define RE_MIN -2
#define RE_MAX 1.0

#define IM_MIN -1
#define IM_MAX 1.0

//Function to determine AVX2 support
static unsigned int is_avx2_supported(){

  //unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  //__get_cpuid(7, &eax, &ebx, &ecx, &edx);

  uint32_t regs[4];

  asm volatile(
    "cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (7), "c" (0)
     );

  return ((regs[1] & bit_AVX2) ? 1 : 0);

}

typedef struct pixel{

  unsigned char r;
  unsigned char g;
  unsigned char b;

} pixel;

union four_doubles{
  __m256d vec;
  double arr[4];
};

union four_ints{
  __m256i vec;
  long long arr[4];
};

union eight_ints{
  __m256i vec;
  int arr[8];
};

__m256i vec_mandel(__m256d cre1, __m256d cim1, __m256d cre2, __m256d cim2){

  union four_doubles Zre1, Zim1;
  Zre1.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  Zim1.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);

  union four_doubles Zre2, Zim2;
  Zre2.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  Zim2.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  //printf("c in %lf %lfi out %lf %lfi\n", vecC.arr[0], vecC.arr[1], vecC.arr[2], vecC.arr[3]);

  union four_ints res1, res2;
  res1.vec = _mm256_set_epi64x(0, 0, 0, 0);
  res2.vec = _mm256_set_epi64x(0, 0, 0, 0);

  const __m256i zero = _mm256_set_epi64x(0, 0, 0, 0);
  const __m256i one = _mm256_set_epi64x(1, 1, 1, 1);
  const __m256d two = _mm256_set_pd(2.0, 2.0, 2.0, 2.0);
  __m256i iterator = _mm256_set_epi64x(0, 0, 0, 0);

  __m256d Zre_square_1 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  __m256d Zim_square_1 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);

  __m256d Zre_square_2 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  __m256d Zim_square_2 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);

  for(int p = 1; p <= NUM_ITERATIONS_MAX; p++){

    iterator = _mm256_add_epi64(iterator, one);

    //first 4 pixels
    __m256d twoRe_1 = _mm256_add_pd(Zre1.vec, Zre1.vec);
    Zim1.vec = _mm256_mul_pd(Zim1.vec, twoRe_1);
    Zim1.vec = _mm256_add_pd(Zim1.vec, cim1);

    Zre1.vec = _mm256_sub_pd(Zre_square_1, Zim_square_1);
    Zre1.vec = _mm256_add_pd(Zre1.vec, cre1);

    Zre_square_1 = _mm256_mul_pd(Zre1.vec, Zre1.vec);
    Zim_square_1 = _mm256_mul_pd(Zim1.vec, Zim1.vec);

    __m256d abs1 = _mm256_add_pd(Zre_square_1, Zim_square_1);
    abs1 = _mm256_sqrt_pd(abs1);

    //second two pixels
    __m256d twoRe_2 = _mm256_add_pd(Zre2.vec, Zre2.vec);
    Zim2.vec = _mm256_mul_pd(Zim2.vec, twoRe_2);
    Zim2.vec = _mm256_add_pd(Zim2.vec, cim2);

    Zre2.vec = _mm256_sub_pd(Zre_square_2, Zim_square_2);
    Zre2.vec = _mm256_add_pd(Zre2.vec, cre2);

    Zre_square_2 = _mm256_mul_pd(Zre2.vec, Zre2.vec);
    Zim_square_2 = _mm256_mul_pd(Zim2.vec, Zim2.vec);

    __m256d abs2 = _mm256_add_pd(Zre_square_2, Zim_square_2);
    abs2 = _mm256_sqrt_pd(abs2);

    //conditional move and break
    union four_ints mask1, mask2;
    mask1.vec = (__m256i)_mm256_cmp_pd(abs1, two, _CMP_LT_OQ);
    mask2.vec = (__m256i)_mm256_cmp_pd(abs2, two, _CMP_LT_OQ);

    //if( ( mask.arr[0] | mask.arr[1] | mask.arr[2] | mask.arr[3] ) == 0 ) break;
    if(_mm256_testc_si256(zero, mask1.vec) & _mm256_testc_si256(zero, mask2.vec)) break;

    //printf("%d\n", mask.arr[0]);

    /*res.arr[0] = mask.arr[0] ? p : res.arr[0];
    res.arr[1] = mask.arr[1] ? p : res.arr[1];
    res.arr[2] = mask.arr[2] ? p : res.arr[2];
    res.arr[3] = mask.arr[3] ? p : res.arr[3];*/

    res1.vec = _mm256_blendv_epi8(res1.vec, iterator, mask1.vec);
    res2.vec = _mm256_blendv_epi8(res2.vec, iterator, mask2.vec);

  }

  union eight_ints res;

  res.arr[0] = (int)res1.arr[0];
  res.arr[1] = (int)res1.arr[1];
  res.arr[2] = (int)res1.arr[2];
  res.arr[3] = (int)res1.arr[3];
  res.arr[4] = (int)res2.arr[0];
  res.arr[5] = (int)res2.arr[1];
  res.arr[6] = (int)res2.arr[2];
  res.arr[7] = (int)res2.arr[3];

  return res.vec;

}

int scalar_mandel(double cre, double cim){

  double zre = 0.0;
  double zim = 0.0;

  for(int p = 0; p < NUM_ITERATIONS_MAX; p++){

    //z = (z * z) + c;
    double zre_temp = (zre * zre - zim * zim) + cre;
    zim = (2 * zre * zim) + cim;
    zre = zre_temp;

    if(sqrt(zre * zre + zim * zim) > 2.0) return p;

  }

  return NUM_ITERATIONS_MAX;

}

__m256i mandel(union four_doubles cre1, union four_doubles cim1, union four_doubles cre2, union four_doubles cim2, unsigned int is_avx2){

  union eight_ints res;

  if(is_avx2){
    //AVX2 instrukce jsou podporovany, rychla cesta
    res.vec = vec_mandel(cre1.vec, cim1.vec, cre2.vec, cim2.vec);

  }
  else{
    //AVX2 instrukce nejsou podporovany, pomala cesta
    res.arr[0] = scalar_mandel(cre1.arr[0], cim1.arr[0]);
    res.arr[1] = scalar_mandel(cre1.arr[1], cim1.arr[1]);
    res.arr[2] = scalar_mandel(cre1.arr[2], cim1.arr[2]);
    res.arr[3] = scalar_mandel(cre1.arr[3], cim1.arr[3]);
    res.arr[4] = scalar_mandel(cre2.arr[0], cim2.arr[0]);
    res.arr[5] = scalar_mandel(cre2.arr[1], cim2.arr[1]);
    res.arr[6] = scalar_mandel(cre2.arr[2], cim2.arr[2]);
    res.arr[7] = scalar_mandel(cre2.arr[3], cim2.arr[3]);

  }

  return res.vec;

}

void calculate_frame(pixel color[HEIGHT][WIDTH], double centerX, double centerY, double zoom, unsigned int is_avx2){

  pixel palette[NUM_ITERATIONS_MAX];

  pixel start_color, end_color, diff;

  /*start_color.r = 193;
  start_color.g = 30;
  start_color.b = 56;
  end_color.r = 34;
  end_color.g = 11;
  end_color.b = 52;*/

  start_color.r = 0;
  start_color.g = 0;
  start_color.b = 0;
  end_color.r = 255;
  end_color.g = 255;
  end_color.b = 255;

  diff.r = end_color.r - start_color.r;
  diff.g = end_color.g - start_color.g;
  diff.b = end_color.b - start_color.b;

  for(int x = 0; x < NUM_ITERATIONS_MAX; x++) {
    double range = (double) x / (NUM_ITERATIONS_MAX - 1);
    palette[x].r = start_color.r + (unsigned char)(diff.r * range);
    palette[x].g = start_color.g + (unsigned char)(diff.g * range);
    palette[x].b = start_color.b + (unsigned char)(diff.b * range);
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
    for (size_t j = 0; j < WIDTH; j = j + 8){

      union eight_ints res;
      union four_doubles cre1, cim1, cre2, cim2;
      cre1.vec = _mm256_set_pd(arr_re[i][j + 3], arr_re[i][j + 2], arr_re[i][j + 1], arr_re[i][j]);
      cim1.vec = _mm256_set_pd(arr_im[i][j + 3], arr_im[i][j + 2], arr_im[i][j + 1], arr_im[i][j]);

      cre2.vec = _mm256_set_pd(arr_re[i][j + 7], arr_re[i][j + 6], arr_re[i][j + 5], arr_re[i][j + 4]);
      cim2.vec = _mm256_set_pd(arr_im[i][j + 7], arr_im[i][j + 6], arr_im[i][j + 5], arr_im[i][j + 4]);

      res.vec = mandel(cre1, cim1, cre2, cim2, is_avx2);

      for(size_t ii = 0; ii < 8; ii++){

        int index = (int)res.arr[ii];

        if(index == 0){
          color[i][j + ii].r = 0;
          color[i][j + ii].b = 0;
          color[i][j + ii].g = 0;
        }
        else{
          color[i][j + ii].r = palette[index].r;
          color[i][j + ii].b = palette[index].g;
          color[i][j + ii].g = palette[index].b;
        }
      }
      //printf("%d \n",color[i][j].r);

    }
  }

}

int main() {

  unsigned int is_avx2 = is_avx2_supported();

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
  static pixel color[HEIGHT][WIDTH];

  SDL_Keycode commands[] = {SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_q};
  int i = 0;

  while (!quit)
  {

    if(update){

      calculate_frame(color, centerX, centerY, zoom, is_avx2);

      SDL_ConvertPixels(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGB24, color, WIDTH * 3, SDL_PIXELFORMAT_RGBA32, image->pixels, image->pitch);
      texture = SDL_CreateTextureFromSurface(renderer, image);

      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      update = 0;
    }

    //SDL_WaitEvent(&event);
    SDL_PollEvent(&event);

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
