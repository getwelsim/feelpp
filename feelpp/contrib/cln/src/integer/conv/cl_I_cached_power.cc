// cached_power().

// Specification.
#include "integer/conv/cl_I_cached_power.h"


// Implementation.

namespace cln {

const power_table_entry power_table [36-2+1] = {
#if (intDsize==8)
	{ 7, 2*2*2*2*2*2*2 },
	{ 5, 3*3*3*3*3 },
	{ 3, 4*4*4 },
	{ 3, 5*5*5 },
	{ 3, 6*6*6 },
	{ 2, 7*7 },
	{ 2, 8*8 },
	{ 2, 9*9 },
	{ 2, 10*10 },
	{ 2, 11*11 },
	{ 2, 12*12 },
	{ 2, 13*13 },
	{ 2, 14*14 },
	{ 2, 15*15 },
	{ 1, 16 },
	{ 1, 17 },
	{ 1, 18 },
	{ 1, 19 },
	{ 1, 20 },
	{ 1, 21 },
	{ 1, 22 },
	{ 1, 23 },
	{ 1, 24 },
	{ 1, 25 },
	{ 1, 26 },
	{ 1, 27 },
	{ 1, 28 },
	{ 1, 29 },
	{ 1, 30 },
	{ 1, 31 },
	{ 1, 32 },
	{ 1, 33 },
	{ 1, 34 },
	{ 1, 35 },
	{ 1, 36 },
#endif
#if (intDsize==16)
	{ 15, 2*2*2*2*2*2*2*2*2*2*2*2*2*2*2 },
	{ 10, 3*3*3*3*3*3*3*3*3*3 },
	{  7, 4*4*4*4*4*4*4 },
	{  6, 5*5*5*5*5*5 },
	{  6, 6*6*6*6*6*6 },
	{  5, 7*7*7*7*7 },
	{  5, 8*8*8*8*8 },
	{  5, 9*9*9*9*9 },
	{  4, 10*10*10*10 },
	{  4, 11*11*11*11 },
	{  4, 12*12*12*12 },
	{  4, 13*13*13*13 },
	{  4, 14*14*14*14 },
	{  4, 15*15*15*15 },
	{  3, 16*16*16 },
	{  3, 17*17*17 },
	{  3, 18*18*18 },
	{  3, 19*19*19 },
	{  3, 20*20*20 },
	{  3, 21*21*21 },
	{  3, 22*22*22 },
	{  3, 23*23*23 },
	{  3, 24*24*24 },
	{  3, 25*25*25 },
	{  3, 26*26*26 },
	{  3, 27*27*27 },
	{  3, 28*28*28 },
	{  3, 29*29*29 },
	{  3, 30*30*30 },
	{  3, 31*31*31 },
	{  3, 32*32*32 },
	{  3, 33*33*33 },
	{  3, 34*34*34 },
	{  3, 35*35*35 },
	{  3, 36*36*36 },
#endif
#if (intDsize==32)
	{ 31, 2UL*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2 },
	{ 20, 3UL*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3 },
	{ 15, 4UL*4*4*4*4*4*4*4*4*4*4*4*4*4*4 },
	{ 13, 5UL*5*5*5*5*5*5*5*5*5*5*5*5 },
	{ 12, 6UL*6*6*6*6*6*6*6*6*6*6*6 },
	{ 11, 7UL*7*7*7*7*7*7*7*7*7*7 },
	{ 10, 8UL*8*8*8*8*8*8*8*8*8 },
	{ 10, 9UL*9*9*9*9*9*9*9*9*9 },
	{  9, 10UL*10*10*10*10*10*10*10*10 },
	{  9, 11UL*11*11*11*11*11*11*11*11 },
	{  8, 12UL*12*12*12*12*12*12*12 },
	{  8, 13UL*13*13*13*13*13*13*13 },
	{  8, 14UL*14*14*14*14*14*14*14 },
	{  8, 15UL*15*15*15*15*15*15*15 },
	{  7, 16UL*16*16*16*16*16*16 },
	{  7, 17UL*17*17*17*17*17*17 },
	{  7, 18UL*18*18*18*18*18*18 },
	{  7, 19UL*19*19*19*19*19*19 },
	{  7, 20UL*20*20*20*20*20*20 },
	{  7, 21UL*21*21*21*21*21*21 },
	{  7, 22UL*22*22*22*22*22*22 },
	{  7, 23UL*23*23*23*23*23*23 },
	{  6, 24UL*24*24*24*24*24 },
	{  6, 25UL*25*25*25*25*25 },
	{  6, 26UL*26*26*26*26*26 },
	{  6, 27UL*27*27*27*27*27 },
	{  6, 28UL*28*28*28*28*28 },
	{  6, 29UL*29*29*29*29*29 },
	{  6, 30UL*30*30*30*30*30 },
	{  6, 31UL*31*31*31*31*31 },
	{  6, 32UL*32*32*32*32*32 },
	{  6, 33UL*33*33*33*33*33 },
	{  6, 34UL*34*34*34*34*34 },
	{  6, 35UL*35*35*35*35*35 },
	{  6, 36UL*36*36*36*36*36 },
#endif
#if (intDsize==64)
	{ 63, 2ULL*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2*2 },
	{ 40, 3ULL*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3*3 },
	{ 31, 4ULL*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4*4 },
	{ 27, 5ULL*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5*5 },
	{ 24, 6ULL*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6*6 },
	{ 22, 7ULL*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7*7 },
	{ 21, 8ULL*8*8*8*8*8*8*8*8*8*8*8*8*8*8*8*8*8*8*8*8 },
	{ 20, 9ULL*9*9*9*9*9*9*9*9*9*9*9*9*9*9*9*9*9*9*9 },
	{ 19, 10ULL*10*10*10*10*10*10*10*10*10*10*10*10*10*10*10*10*10*10 },
	{ 18, 11ULL*11*11*11*11*11*11*11*11*11*11*11*11*11*11*11*11*11 },
	{ 17, 12ULL*12*12*12*12*12*12*12*12*12*12*12*12*12*12*12*12 },
	{ 17, 13ULL*13*13*13*13*13*13*13*13*13*13*13*13*13*13*13*13 },
	{ 16, 14ULL*14*14*14*14*14*14*14*14*14*14*14*14*14*14*14 },
	{ 16, 15ULL*15*15*15*15*15*15*15*15*15*15*15*15*15*15*15 },
	{ 15, 16ULL*16*16*16*16*16*16*16*16*16*16*16*16*16*16 },
	{ 15, 17ULL*17*17*17*17*17*17*17*17*17*17*17*17*17*17 },
	{ 15, 18ULL*18*18*18*18*18*18*18*18*18*18*18*18*18*18 },
	{ 15, 19ULL*19*19*19*19*19*19*19*19*19*19*19*19*19*19 },
	{ 14, 20ULL*20*20*20*20*20*20*20*20*20*20*20*20*20 },
	{ 14, 21ULL*21*21*21*21*21*21*21*21*21*21*21*21*21 },
	{ 14, 22ULL*22*22*22*22*22*22*22*22*22*22*22*22*22 },
	{ 14, 23ULL*23*23*23*23*23*23*23*23*23*23*23*23*23 },
	{ 13, 24ULL*24*24*24*24*24*24*24*24*24*24*24*24 },
	{ 13, 25ULL*25*25*25*25*25*25*25*25*25*25*25*25 },
	{ 13, 26ULL*26*26*26*26*26*26*26*26*26*26*26*26 },
	{ 13, 27ULL*27*27*27*27*27*27*27*27*27*27*27*27 },
	{ 13, 28ULL*28*28*28*28*28*28*28*28*28*28*28*28 },
	{ 13, 29ULL*29*29*29*29*29*29*29*29*29*29*29*29 },
	{ 13, 30ULL*30*30*30*30*30*30*30*30*30*30*30*30 },
	{ 12, 31ULL*31*31*31*31*31*31*31*31*31*31*31 },
	{ 12, 32ULL*32*32*32*32*32*32*32*32*32*32*32 },
	{ 12, 33ULL*33*33*33*33*33*33*33*33*33*33*33 },
	{ 12, 34ULL*34*34*34*34*34*34*34*34*34*34*34 },
	{ 12, 35ULL*35*35*35*35*35*35*35*35*35*35*35 },
	{ 12, 36ULL*36*36*36*36*36*36*36*36*36*36*36 },
#endif
};

cached_power_table* ctable [36-2+1] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL
};

const cached_power_table_entry * cached_power (uintD base, uintL i)
{
	var cached_power_table* ptr;
	if (!(ptr = ctable[base-2]))
        { ctable[base-2] = ptr = new cached_power_table (); }
	var uintL j;
	for (j = 0; j <= i; j++) {
		if (zerop(ptr->element[j].base_pow)) {
			// Compute b^(k*2^j) and its inverse.
			cl_I x =
			    (j==0 ? cl_I(power_table[base-2].b_to_the_k)
			          : ptr->element[j-1].base_pow * ptr->element[j-1].base_pow
			     );
			ptr->element[j].base_pow = x;
#ifdef MUL_REPLACES_DIV
			ptr->element[j].inv_base_pow = floor1(ash(1,2*integer_length(x)),x);
#endif
		}
	}
	return &ptr->element[i];
}

AT_DESTRUCTION(cached_power)
{
	for (var uintD base = 2; base <= 36; base++) {
        	var cached_power_table* ptr = ctable[base-2];
		if (ptr) {
			delete ptr;
			ctable[base-2] = NULL;
		}
        }
}

}  // namespace cln