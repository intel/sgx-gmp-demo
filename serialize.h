#ifndef __SERIALIZE__H
#define __SERIALIZE__H

#include <gmp.h>

char *mpz_serialize (mpz_t val);
char *mpf_serialize (mpf_t val, int digits);

int mpz_deserialize(mpz_t *val, char *s);
int mpf_deserialize(mpf_t *val, char *s, int digits);

#endif

