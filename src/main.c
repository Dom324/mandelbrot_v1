
#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>      //Library to render a window

#include "config.h"           //Include header file with defines
#include "structs.h"          //Include header file with structs
#include "detect_avx2.h"      //Include header file with function used to detect avx2 support

#include "render_mandel.c"    //Include source file with render functions

struct _4i_arr mandel(struct _4d_arr cre, struct _4d_arr cim, unsigned int avx2);
void calculate_frame(pixel color[HEIGHT][WIDTH], double centerX, double centerY, double zoom, unsigned int avx2);

int main() {

  //check avx2 support
  const unsigned int is_avx2 = is_avx2_supported();
  unsigned int enable_avx2 = is_avx2;

  //inform user whether his system supports avx2
  if(is_avx2) printf("AVX2 instrukce jsou podporovany, pouziva se vektorova (rychla) cesta\n");
  else printf("AVX2 instrukce NEjsou podporovany, pouziva se skalarni (pomala) cesta\n");

  //Inicializovat SDL okno, renderer, texturu
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window * window = SDL_CreateWindow("SDL2 Displaying Image", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);

  if(window == NULL){
    printf("%s", SDL_GetError());
    return -1;
  }

  SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Surface * image = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
  SDL_Texture * texture;
  SDL_Event event;

  //promena pro detekovani zda byla zmacknuta klavesa
  int update = 1;

  //setup position
  double zoom = 1.0;
  double centerX = 0;
  double centerY = 0;
  //create screen buffer
  static pixel color[HEIGHT][WIDTH];

  //const SDL_Keycode commands[] = {SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_KP_PLUS, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_q};
  //int i = 0;

  while (1){

    int quit = 0;

    //update the screen when key is pressed
    if(update){

      //volani funkce na vypocet noveho snimku
      //predava se pole "color" (ktere je pouzito i jako vystup), souradnice a zoom obrazovky, a zda se ma pouzit vektorovy mod
      calculate_frame(color, centerX, centerY, zoom, is_avx2 & enable_avx2);

      SDL_ConvertPixels(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGB24, color, WIDTH * 3, SDL_PIXELFORMAT_RGBA32, image->pixels, image->pitch);
      texture = SDL_CreateTextureFromSurface(renderer, image);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      update = 0;
    }

    //SDL_WaitEvent(&event);
    SDL_PollEvent(&event);

    /*event.key.keysym.sym = commands[i];
    i++;*/

    switch(event.type){
    //switch (SDL_KEYDOWN){

      case SDL_QUIT:       //pokud uzivatel zavrel okno
        quit = 1;
      break;

      case SDL_KEYDOWN:   //pokud uzivatel zmacknul klavesu

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
        else if(event.key.keysym.sym == SDLK_v){
          if(!is_avx2) printf("Nelze prepnout renderovaci mod, AVX2 instrukce NEjsou podporovany, pouziva se stale skalarni (pomala) cesta\n");
          else{
            enable_avx2 = !enable_avx2;
            update = 1;
            if(enable_avx2) printf("Renderovaci mod prepnut na vektorovy (rychly)\n");
            else printf("Renderovaci mod prepnut na skalarni (pomaly)\n");
          }
        }

      break;

    }

    //pokud bylo zmacknuto q nebo uzivatel zavrel okno
    //dealokovat pamet, zavrit okno a vyskocit z nekonecneho cyklu
    if(quit == 1){

      SDL_DestroyTexture(texture);
      SDL_FreeSurface(image);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);

      SDL_Quit();

      break;
    }

  }

  return 0;
}

//funkce pro vypocet noveho snimku
//vstupi data je souradnice na realne ose "centerX", souradnice na imaginarni ose "centerY", priblizeni a volba renderovaciho modu
//vystup funkce je zapsan do pole "color" predaneho jako parametr
void calculate_frame(pixel color[HEIGHT][WIDTH], double centerX, double centerY, double zoom, unsigned int avx2){

  // code to generate color palette
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

  // code to generate coordinates which are passed as an input to the rendering functions
  // coordinates are stored in arrays arr_re (real part), arr_im (imaginary part)
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

  //printf("xmin %lf xmax %lf ymin %lf ymax %lf\n", x_min, x_max, y_min, y_max);

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

      struct _4i_arr res;
      struct _4d_arr cre, cim;

      cre.arr[0] = arr_re[i][j + 0];
      cre.arr[1] = arr_re[i][j + 1];
      cre.arr[2] = arr_re[i][j + 2];
      cre.arr[3] = arr_re[i][j + 3];

      cim.arr[0] = arr_im[i][j + 0];
      cim.arr[1] = arr_im[i][j + 1];
      cim.arr[2] = arr_im[i][j + 2];
      cim.arr[3] = arr_im[i][j + 3];

      res = mandel(cre, cim, avx2);

      for(size_t ii = 0; ii < 4; ii++){

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
