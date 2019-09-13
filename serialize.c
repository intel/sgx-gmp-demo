/*
Copyright 2019 Intel Corporation

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

#ifdef HAVE_SGX
# include <sgx_tgmp.h>
#else
# include <gmp.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "serialize.h"

#define S_BASE 62

static void *(*gmp_alloc_func)(size_t)= NULL;
static void (*gmp_free_func)(void *, size_t)= NULL;

char *mpz_serialize (mpz_t val)
{
	/* Create as compact a form as possible */
	return mpz_get_str(NULL, S_BASE, val);
}

char *mpf_serialize (mpf_t val, int digits)
{
	mp_exp_t mpe= 0;
	mpz_t e;
	char *smant, *se, *s;
	size_t len;

	/* Get our free function. */

	if ( gmp_free_func == NULL || gmp_alloc_func == NULL )
		mp_get_memory_functions(&gmp_alloc_func, NULL, &gmp_free_func);

	smant= mpf_get_str(NULL, &mpe, S_BASE, digits, val);
	if ( smant == NULL ) return NULL;

	mpz_init_set_si(e, mpe);

	se= mpz_get_str(NULL, S_BASE, e);
	if ( se == NULL ) {
		gmp_free_func(smant, 0);
		return NULL;
	}

	len= strlen(smant)+strlen(se)+3;
	s= gmp_alloc_func(len); /* .M@N + NULL */
	if ( s == NULL ) {
		gmp_free_func(smant, 0);
		gmp_free_func(se, 0);
		return NULL;
	}
	
	/*
	 * mpf_get_str produces strings that can't be directly consumed by
	 * mpf_set_str, so deal with that.
	 */

	if ( smant[0] == '-' ) {
		strncpy(s, "-.", 2);
		strncat(s, &smant[1], strlen(smant)-1);
	} else {
		strncpy(s, ".", 2);
		strncat(s, smant, strlen(smant));
	}
	strncat(s, "@", 1);
	strncat(s, se, strlen(se));
	s[len]= '\0';
	gmp_free_func(smant, 0);
	gmp_free_func(se, 0);

	return s;
}

int mpz_deserialize(mpz_t *val, char *s)
{
	return mpz_set_str(*val, s, S_BASE);
}

int mpf_deserialize(mpf_t *val, char *s, int digits)
{
	static double bits= log2(10);

	mpf_set_prec(*val, (digits*bits)+1);

	return mpf_set_str(*val, s, S_BASE);
}
