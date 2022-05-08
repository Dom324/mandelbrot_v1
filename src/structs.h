#pragma once

#include <emmintrin.h>
#include <immintrin.h>

typedef struct pixel{

  unsigned char r;
  unsigned char g;
  unsigned char b;

} pixel;

struct _4d_arr{
  double arr[4];
};

struct _4i_arr{
  int arr[4];
};

union four_doubles_avx{
  __m256d vec;
  double arr[4];
};

union four_ints_avx{
  __m256i vec;
  long long arr[4];
};
