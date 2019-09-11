#include "serialize.h"
#include <stdio.h>
#include <gmp.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
	mpz_t a;
	mpf_t f;
	char *s;

	mpz_init_set_str(a, "-3495834905870984801203923984598723", 10);
	gmp_printf("GMP: %Zd\n", a);
	s= mpz_serialize(a);

	printf("serialized: %s\n", s);

	mpz_set_ui(a, 0);
	gmp_printf("cleared: %Zd\n", a);

	if ( mpz_deserialize(&a, s) == -1 ) {
		fprintf(stderr, "mpz_deserialize: bad serial format\n");
		return 1;
	}
	free(s);

	gmp_printf("deserialized GMP: %Zd\n", a);

	if ( mpf_init_set_str(f, "-314159.2653546", 10) == -1 ) {
		fprintf(stderr, "mpf_init_set_str: bad format\n");
		return 1;
	}
	gmp_printf("GMP: %.12Ff\n", f);
	s= mpf_serialize(f, 6);

	printf("serialize (6 digits): %s\n", s);
	free(s);

	s= mpf_serialize(f, 9);

	printf("serialize (9 digits): %s\n", s);

	mpf_set_ui(f, 0);
	gmp_printf("cleared: %.12Ff\n", f);

	if ( mpf_deserialize(&f, s, 9) == -1 ) {
		fprintf(stderr, "mpz_deserialize: bad serial format\n");
		return 1;
	}
	free(s);

	gmp_printf("deserialized GMP: %.12Ff\n", f);
}

