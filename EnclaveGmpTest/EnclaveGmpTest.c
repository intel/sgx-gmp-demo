/*

Copyright 2018 Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "EnclaveGmpTest_t.h"
#include <sgx_tgmp.h>
#include <sgx_trts.h>
#include <math.h>
#include <string.h>
#include "serialize.h"

void *(*gmp_realloc_func)(void *, size_t, size_t);
void (*gmp_free_func)(void *, size_t);

void *reallocate_function(void *, size_t, size_t);
void free_function(void *, size_t);

void e_calc_pi (mpf_t *pi, uint64_t digits);

/*
 * Use a global to store our results. A real program would need something
 * more sophisticated than this.
 */

char *result;
size_t len_result= 0;

void tgmp_init()
{
	result= NULL;
	len_result= 0;

	mp_get_memory_functions(NULL, &gmp_realloc_func, &gmp_free_func);
	mp_set_memory_functions(NULL, &reallocate_function, &free_function);
}

void free_function (void *ptr, size_t sz)
{
	if ( sgx_is_within_enclave(ptr, sz) ) gmp_free_func(ptr, sz);
	else abort();
}

void *reallocate_function (void *ptr, size_t osize, size_t nsize)
{
	if ( ! sgx_is_within_enclave(ptr, osize) ) abort();

	return gmp_realloc_func(ptr, osize, nsize);
}

int e_get_result(char *str, size_t len)
{
	/* Make sure the application doesn't ask for more bytes than 
	 * were allocated for the result. */

	if ( len > len_result ) return 0;

	/*
	 * Marshal our result out of the enclave. Make sure the destination
	 * buffer is completely outside the enclave, and that what we are
	 * copying is completely inside the enclave.
	 */

	if ( result == NULL || str == NULL || len == 0 ) return 0;

	if ( ! sgx_is_within_enclave(result, len) ) return 0;

	if ( sgx_is_outside_enclave(str, len+1) ) { /* Include terminating NULL */
		strncpy(str, result, len); 
		str[len]= '\0';

		gmp_free_func(result, NULL);
		result= NULL;
		len_result= 0;

		return 1;
	}

	return 0;
}

size_t e_mpz_add(char *str_a, char *str_b)
{
	mpz_t a, b, c;

	/*
	 * Marshal untrusted values into the enclave so we don't accidentally
	 * leak secrets to untrusted memory.
	 *
	 * This is overkill for the trivial example in this function, but
	 * it's best to develop good coding habits.
	 */

	if ( str_a == NULL || str_b == NULL ) return 0;

	/* Clear the last, serialized result */

	if ( result != NULL ) {
		gmp_free_func(result, NULL);
		result= NULL;
		len_result= 0;
	}

	mpz_inits(a, b, c, NULL);

	/* Deserialize */

	if ( mpz_deserialize(&a, str_a) == -1 ) return 0;
	if ( mpz_deserialize(&b, str_b) == -1 ) return 0;

	mpz_add(c, a, b);

	/* Serialize the result */

	result= mpz_serialize(c);
	if ( result == NULL ) return 0;

	len_result= strlen(result);
	return len_result;
}

size_t e_mpz_mul(char *str_a, char *str_b)
{
	mpz_t a, b, c;

	/* Marshal untrusted values into the enclave. */

	if ( str_a == NULL || str_b == NULL ) return 0;

	/* Clear the last, serialized result */

	if ( result != NULL ) {
		gmp_free_func(result, NULL);
		result= NULL;
		len_result= 0;
	}

	mpz_inits(a, b, c, NULL);

	/* Deserialize */

	if ( mpz_deserialize(&a, str_a) == -1 ) return 0;
	if ( mpz_deserialize(&b, str_b) == -1 ) return 0;

	mpz_mul(c, a, b);

	/* Serialize the result */

	result= mpz_serialize(c);
	if ( result == NULL ) return 0;

	len_result= strlen(result);
	return len_result;
}

size_t e_mpz_div(char *str_a, char *str_b)
{
	mpz_t a, b, c;

	/* Marshal untrusted values into the enclave */

	if ( str_a == NULL || str_b == NULL ) return 0;

	/* Clear the last, serialized result */

	if ( result != NULL ) {
		gmp_free_func(result, NULL);
		result= NULL;
		len_result= 0;
	}

	mpz_inits(a, b, c, NULL);

	/* Deserialize */

	if ( mpz_deserialize(&a, str_a) == -1 ) return 0;
	if ( mpz_deserialize(&b, str_b) == -1 ) return 0;

	mpz_div(c, a, b);

	/* Serialize the result */

	result= mpz_serialize(c);
	if ( result == NULL ) return 0;

	len_result= strlen(result);
	return len_result;
}

size_t e_mpf_div(char *str_a, char *str_b, int digits)
{
	mpz_t a, b;
	mpf_t fa, fb, fc;

	/* Marshal untrusted values into the enclave */

	if ( str_a == NULL || str_b == NULL ) return 0;

	/* Clear the last, serialized result */

	if ( result != NULL ) {
		gmp_free_func(result, NULL);
		result= NULL;
		len_result= 0;
	}

	mpz_inits(a, b, NULL);
	mpf_inits(fa, fb, fc, NULL);

	/* Deserialize */

	if ( mpz_deserialize(&a, str_a) == -1 ) return 0;
	if ( mpz_deserialize(&b, str_b) == -1 ) return 0;

	mpf_set_z(fa, a);
	mpf_set_z(fb, b);

	mpf_div(fc, fa, fb);


	/* Serialize the result */

	result= mpf_serialize(fc, digits);
	if ( result == NULL ) return 0;

	len_result= strlen(result);
	return len_result;
}

/* Use the Chudnovsky equation to rapidly estimate pi */

#define DIGITS_PER_ITERATION 14.1816 /* Roughly */

mpz_t c3, c4, c5;
int pi_init= 0;

size_t e_pi (uint64_t digits)
{
	mpf_t pi;

	/* Clear the last, serialized result */

	if ( result != NULL ) {
		gmp_free_func(result, NULL);
		result= NULL;
		len_result= 0;
	}

	/*
	 * Perform our operations on a variable that's located in the enclave,
	 * then marshal the final value out of the enclave.
	 */

	mpf_init(pi);

	e_calc_pi(&pi, digits+1);

	/* Marshal our result to untrusted memory */

	mpf_set_prec(pi, mpf_get_prec(pi));

	result= mpf_serialize(pi, digits+1);
	if ( result == NULL ) return 0;

	len_result= strlen(result);
	return len_result;
}

void e_calc_pi (mpf_t *pi, uint64_t digits)
{
	uint64_t k, n;
	mp_bitcnt_t precision;
	static double bits= log2(10);
	mpz_t kf, kf3, threekf, sixkf, z1, z2, c4k, c5_3k;
	mpf_t C, sum, div, f2;

	n= (digits/DIGITS_PER_ITERATION)+1;
	precision= (digits * bits)+1;

	mpf_set_default_prec(precision);

	/* Re-initialize the pi variable to use our new precision */

	mpf_set_prec(*pi, precision);

	/*

		426880 sqrt(10005)    inf (6k)! (13591409+545140134k)
		------------------- = SUM ---------------------------
		         pi           k=0   (3k)!(k!)^3(-640320)^3k

		C / pi = SUM (6k)! * (c3 + c4*k) / (3k)!(k!)^3(c5)^3k

		C / pi = SUM f1 / f2

		pi = C / sum

	*/

	mpz_inits(sixkf, z1, z2, kf, kf3, threekf, c4k, c5_3k, NULL);
	mpf_inits(C, sum, div, f2, NULL);

	/* Calculate 'C' */

	mpf_sqrt_ui(C, 10005);
	mpf_mul_ui(C, C, 426880);

	if ( ! pi_init ) {
		/* Constants needed in 'sum'. */

		mpz_inits(c3, c4, c5, NULL);

		mpz_set_ui(c3, 13591409);
		mpz_set_ui(c4, 545140134);
		mpz_set_si(c5, -640320);

		pi_init= 1;
	}


	mpf_set_ui(sum, 0);

	for (k= 0; k< n; ++k) {
		/* Numerator */
		mpz_fac_ui(sixkf, 6*k);
		mpz_mul_ui(c4k, c4, k);
		mpz_add(c4k, c4k, c3);
		mpz_mul(z1, c4k, sixkf);
		mpf_set_z(div, z1);

		/* Denominator */
		mpz_fac_ui(threekf, 3*k);
		mpz_fac_ui(kf, k);
		mpz_pow_ui(kf3, kf, 3);
		mpz_mul(z2, threekf, kf3);
		mpz_pow_ui(c5_3k, c5, 3*k);
		mpz_mul(z2, z2, c5_3k);

		/* Divison */

		mpf_set_z(f2, z2);
		mpf_div(div, div, f2);

		/* Sum */

		mpf_add(sum, sum, div);
	}

	mpf_div(*pi, C, sum);

	mpf_clears(div, sum, f2, NULL);
}

