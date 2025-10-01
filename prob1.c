#include <stdio.h>
#include <string.h>
#include <math.h>

#define SIGN_FIELD_BITS 1
#define EXPONENT_FIELD_BITS 8
#define FRACTION_FIELD_BITS 23
#define TYPE_BITS (SIGN_FIELD_BITS+EXPONENT_FIELD_BITS+FRACTION_FIELD_BITS)

typedef struct {
   unsigned int frac : FRACTION_FIELD_BITS;
   unsigned int exp : EXPONENT_FIELD_BITS;
   unsigned int sign : SIGN_FIELD_BITS;
} float_like_type;


float to_float(float_like_type *flt) {
	float f;
	memcpy(&f, flt, sizeof(f));
	return f;
}

float get_largest_float() {
	float_like_type flt;
	
	flt.sign = /* put the correct value here */;
	flt.exp = /* put the correct value here */;
	flt.frac = /* put the correct value here */;

	return to_float(&flt);
}

void main()
{
	float largest = get_largest_float();
	
	printf("The largest number: %f", largest);
}