#include <sys/types.h>
#include <stdio.h>
#include <x86intrin.h>

/*********************************************
# gcc nl_cnt_test.c
# ./a.out
*********************************************/

/********** byfoot ********/

size_t byfoot(const char* s,size_t l) {
  size_t i,r;
  for (i=r=0; i<l; ++i)
    if (s[i]==0x0a) ++r;
  return r;
}

/******** self ********/

static size_t nl_to_0x10(size_t v) {
  size_t v_high, v_low, v_nibbled, v_flagged;
  v_low = v ^ (size_t)0x0a0a0a0a0a0a0a0aull;
  v_high = v>>4;
  v_nibbled = v_low | v_high | (size_t)0xf0f0f0f0f0f0f0f0ull; // F0 vs F1..FF
  v_flagged = (v_nibbled - 0x1111111111111111ull) & 0x1010101010101010ull; // DF vs. E0..EE
  return v_flagged;
}

size_t newlines_self(const char* s,size_t l) {
  size_t i,n=0,r=0,max=l/sizeof(n);
  size_t* S=(size_t*)s;
  max &= ~7;
  for (i=0; i<max; ) {
    n += nl_to_0x10(S[i++]);
    n += nl_to_0x10(S[i++]);
    n += nl_to_0x10(S[i++]);
    n += nl_to_0x10(S[i++]);

    n += nl_to_0x10(S[i++]);
    n += nl_to_0x10(S[i++]);
    n += nl_to_0x10(S[i++]);
    n += nl_to_0x10(S[i++]);
    r += n&0xFF0;
    n >>= 8;
  }
  while(n){
    r += n&0xFF0;
    n >>= 8;
  }
  r>>=4;

  s=(char *)&(S[i]); l = l-i*sizeof(n);
  while(l--) {
      r += (*s++=='\n');
  }
  return r;
}

static size_t fold(size_t v) {
  return (v * (size_t)0x0101010101010101) >> (sizeof(v)*8-8);
}


/***** FEFE ********/
static size_t nlto1(size_t v) {
  v ^= (size_t)0x0a0a0a0a0a0a0a0a;
  v |= (v & (size_t)0x0101010101010101ull) << 1;
  v = ((v - (size_t)0x0101010101010101ull) & ~v & (size_t)0x8080808080808080ull);
  return v >> 7;
}

size_t newlines_fefe(const char* s,size_t l) {
  size_t i,n=0,r=0,max=l/sizeof(n);
  size_t* S=(size_t*)s;
  int o=255;
  for (i=0; i<max; ++i) {
    n += nlto1(S[i]);
    if (!--o) {
      r += fold(n);
      n=0;
      o=255;
    }
  }
  r += fold(n);
  if (l %= sizeof(i)) {
    s += i*sizeof(n);
    for (; l; --l)
      r += (*s++=='\n');
  }
  return r;
}

/**********************/

void teste_mal(char * buff, size_t l){
 size_t n_NLa, n_NLc, n_NLb, t0,t1,t2,t3;
 t0 = __rdtsc();
 n_NLa = newlines_fefe(buff, l);
 t1 = __rdtsc();
 n_NLb = newlines_self(buff, l);
 t2 = __rdtsc();
 n_NLc = byfoot(buff, l);
 t3 = __rdtsc();


 printf("newlines: %d (fefe) vs. %d (self) vs. %d (foot) \n", n_NLa, n_NLb, n_NLc);
 printf("cycles: %d (fefe) vs. %d (self) vs. %d (foot) \n", t1-t0, t2-t1, t3-t2);

}

int main(){
 char buff[64*1024];
 size_t n, n_NLa, n_NLb, pre,mid,post;
 printf("size=%d\n", sizeof(n));

 for(n=64*1024;n--;) buff[n] = 10;
 printf("== 5k: 0x0a ==\n");
 teste_mal(buff,64*1024);

 for(n=64*1024;n--;) buff[n] = n&0x7F;
 printf("== 5k: 0x00 01 02 .. 7F 00 .. ==\n");
 teste_mal(buff,64*1024);

 for(n=64*1024;n--;) buff[n] = 0x00;
 printf("== 5k: 0x00 ==\n");
 teste_mal(buff,64*1024);
}
