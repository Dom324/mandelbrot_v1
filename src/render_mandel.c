
// include headers with vector instructions
#include <emmintrin.h>
#include <immintrin.h>

// standard scalar version
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

// vector version
__attribute__ ((target ("avx2")))
struct _4i_arr vec_mandel(struct _4d_arr cre, struct _4d_arr cim){

  union four_doubles_avx Zre, Zim, Cre, Cim;
  Cre.vec = _mm256_set_pd(cre.arr[0], cre.arr[1], cre.arr[2], cre.arr[3]);
  Cim.vec = _mm256_set_pd(cim.arr[0], cim.arr[1], cim.arr[2], cim.arr[3]);
  Zre.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  Zim.vec = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  //printf("c in %lf %lfi out %lf %lfi\n", vecC.arr[0], vecC.arr[1], vecC.arr[2], vecC.arr[3]);

  union four_ints_avx res;
  res.vec = _mm256_set_epi64x(0, 0, 0, 0);
  __m256i zero = _mm256_set_epi64x(0, 0, 0, 0);
  __m256i iterator = _mm256_set_epi64x(0, 0, 0, 0);
  __m256i one = _mm256_set_epi64x(1, 1, 1, 1);

  __m256d Zre2 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
  __m256d Zim2 = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);

  __m256d two = _mm256_set_pd(2.0, 2.0, 2.0, 2.0);

  for(int p = 1; p <= NUM_ITERATIONS_MAX; p++){

    iterator = _mm256_add_epi64(iterator, one);

    __m256d twoX = _mm256_add_pd(Zre.vec, Zre.vec);
    Zim.vec = _mm256_mul_pd(Zim.vec, twoX);
    Zim.vec = _mm256_add_pd(Zim.vec, Cim.vec);

    Zre.vec = _mm256_sub_pd(Zre2, Zim2);
    Zre.vec = _mm256_add_pd(Zre.vec, Cre.vec);

    Zre2 = _mm256_mul_pd(Zre.vec, Zre.vec);
    Zim2 = _mm256_mul_pd(Zim.vec, Zim.vec);

    __m256d abs = _mm256_add_pd(Zre2, Zim2);
    abs = _mm256_sqrt_pd(abs);

    union four_ints_avx mask;
    mask.vec = (__m256i)_mm256_cmp_pd(abs, two, _CMP_LT_OQ);

    //if( ( mask.arr[0] | mask.arr[1] | mask.arr[2] | mask.arr[3] ) == 0 ) break;
    if(_mm256_testc_si256(zero, mask.vec)) break;

    //printf("%d\n", mask.arr[0]);

    /*res.arr[0] = mask.arr[0] ? p : res.arr[0];
    res.arr[1] = mask.arr[1] ? p : res.arr[1];
    res.arr[2] = mask.arr[2] ? p : res.arr[2];
    res.arr[3] = mask.arr[3] ? p : res.arr[3];*/

    res.vec = _mm256_blendv_epi8(res.vec, iterator, mask.vec);

  }

  //convert result from array of long long to array of int
  struct _4i_arr result;
  result.arr[0] = (int)res.arr[0];
  result.arr[1] = (int)res.arr[1];
  result.arr[2] = (int)res.arr[2];
  result.arr[3] = (int)res.arr[3];

  return result;

}
