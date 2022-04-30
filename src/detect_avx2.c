
#include <cpuid.h>    //Identify AVX2 support

//Function to determine AVX2 support
static unsigned int is_avx2_supported(){

  //unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  //__get_cpuid(7, &eax, &ebx, &ecx, &edx);

  volatile uint32_t regs[4];

  asm volatile(
    "cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (7), "c" (0)
  );

  return ((regs[1] & bit_AVX2) ? 1 : 0);

}
