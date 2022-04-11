
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <complex.h>

#include <png.h>

#define HEIGHT 1080
#define WIDTH 1920
#define NUM_ITERATIONS 80

#define RE_MIN -2
#define RE_MAX 1.0

#define IM_MIN -1
#define IM_MAX 1.0

typedef struct pixel{

  unsigned char r;
  unsigned char g;
  unsigned char b;

} pixel;

void write_png_file(char* file_name, pixel color[WIDTH][HEIGHT]);

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

int main() {

  pixel palette[NUM_ITERATIONS];

  for(int x = 0; x < NUM_ITERATIONS; x++) {
    palette[x].r = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 16.0));
    palette[x].g = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 32.0));
    palette[x].b = (unsigned char)(128.0 + 128 * sin(3.1415 * x / 64.0));
  }

  static double complex arr[WIDTH][HEIGHT];
  static pixel color[WIDTH][HEIGHT];

  for (size_t i = 0; i < WIDTH; i++) {
    for (size_t j = 0; j < HEIGHT; j++) {

      double x =  (double)i / WIDTH;
      double y =  (double)j / HEIGHT;

      arr[i][j] = RE_MIN + x * (RE_MAX - RE_MIN) + IM_MIN * I + y * (IM_MAX - IM_MIN) * I;

      /*printf("%.1f%+.1fi cartesian is rho=%f theta=%f polar\n",
           creal(arr[i][j]), cimag(arr[i][j]), cabs(arr[i][j]), carg(arr[i][j]));*/

      arr[i][j] = mandel(arr[i][j]);

      /*printf("%.1f%+.1fi cartesian is rho=%f theta=%f polar\n",
           creal(arr[i][j]), cimag(arr[i][j]), cabs(arr[i][j]), carg(arr[i][j]));*/

      color[i][j].r = (unsigned char)255 - (unsigned char)(cabs(arr[i][j]) * 255.0 / NUM_ITERATIONS);
      //printf("%d \n",color[i][j].r);
      color[i][j].b = palette[(int)cabs(arr[i][j])].b;
      color[i][j].g = palette[(int)(arr[i][j])].g;

    }
  }

  write_png_file("output2.png", color);

  return 0;
}


void write_png_file(char* file_name, pixel color[WIDTH][HEIGHT])
{

        FILE *fp = fopen(file_name, "wb");
        if (!fp) exit(-1);

        png_structp png_ptr;
        png_infop info_ptr;
        png_bytep * row_pointers;

        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr) exit(-1);

        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == NULL) exit(-1);

        if (setjmp(png_jmpbuf(png_ptr))) exit(-1);

        png_init_io(png_ptr, fp);

        if (setjmp(png_jmpbuf(png_ptr))) exit(-1);

        png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT,
                     8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * HEIGHT);

        if(row_pointers == NULL) exit(-1);

        for (int y=0; y < HEIGHT; y++){
            row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
        }

        for (int y=0; y < HEIGHT; y++) {

          png_byte* row = row_pointers[y];

          for (int x=0; x < WIDTH; x++) {

                  png_byte* ptr = &(row[x*4]);
                  /*printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
                        x, y, ptr[0], ptr[1], ptr[2], ptr[3]);*/

                  /* set red value to 0 and green value to the blue one */
                  ptr[0] = color[x][y].r;
                  ptr[1] = color[x][y].g;
                  ptr[2] = color[x][y].b;
                  ptr[3] = 255;
          }
        }

        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        for (int y=0; y < HEIGHT; y++) free(row_pointers[y]);

        free(row_pointers);
        fclose(fp);
}
