#include <string.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// [TODO] You need to decide the bit widths of the fields.
#define SIGN_FIELD_BITS 1
#define EXPONENT_FIELD_BITS 8
#define FRACTION_FIELD_BITS (sizeof(frac_part)*8)

#define MY_TYPE_BITS (FRACTION_FIELD_BITS+EXPONENT_FIELD_BITS+SIGN_FIELD_BITS)

#define BIAS (int)(pow(2, EXPONENT_FIELD_BITS-1) - 1)



// D(소수점 제거된 10진 정수)를 그대로 문자열로 저장하는 가장 단순한 방식
// 과제 예시를 고려해 64바이트 버퍼(=최대 63자리 + null)
typedef struct {
    char digits[64];
} frac_part;

typedef struct {
    unsigned char exp : EXPONENT_FIELD_BITS;
    unsigned char sign : SIGN_FIELD_BITS;
    frac_part frac;
} my_type;



void split(const char *number, char *sign, char *int_part, char *frac_part) {

	if (number[0] == '+') {
		*sign = '+';
		number++;
	} else if (number[0] == '-') {
		*sign = '-';
		number++;
	} else {
		*sign = '+';
	}

	// integral part
	int i = 0;
	while (*number != '\0' && *number != '.') {
		int_part[i++] = *number;
		number++;
	}
	int_part[i] = '\0';

	// fraction part
	int f = 0;
	if (*number == '.') {
		number++;
		while (*number != '\0') {
			frac_part[f++] = *number;
			number++;
		}

		frac_part[f] = '\0';
	}
}


void init_type(const char *number, my_type *t) {
    char sign;
    char int_part[256];
    char frac_part[256];

    split(number, &sign, int_part, frac_part);

    //printf("int_part: %s\n", int_part);
    //printf("frac_part: %s\n", frac_part);

    // sign field
    if (sign == '+') {
	    t->sign = 0;
	    number++;
	}
	else if (sign == '-') {
		t->sign = 1;
		number++;
	}

	// TODO: Implement the encoding logic for the exponent field and fraction field of my_type.

	// exponent field
    // (B) exponent 필드 채우기: E = 소수점 이하 자리수, exp = E + BIAS
    {
        int E = (int)strlen(frac_part);                 // 소수부 길이
        int biased = E + BIAS;                          // 바이어스 더해서 저장
        int maxexp = (1 << EXPONENT_FIELD_BITS) - 1;    // exp 필드의 최대값
        if (biased < 0) biased = 0;                     // 하한 클램프(안전)
        if (biased > maxexp) biased = maxexp;           // 상한 클램프(안전)
        t->exp = (unsigned char)biased;
    }

    // (C) fraction(D) 채우기: D = int_part + frac_part (소수점 제거)
    {
        char buf[256];
        buf[0] = '\0';
        // int_part가 비어 있으면 0부터 시작
        if (int_part[0] != '\0') strcat(buf, int_part); else strcat(buf, "0");
        strcat(buf, frac_part);
        // 전체 선행 0 제거(단, 전체가 0이면 한 자리 0은 남긴다)
        int k = 0; while (buf[k]=='0' && buf[k+1] != '\0') k++;
        if (k) memmove(buf, buf + k, strlen(buf) - k + 1);
        strncpy(t->frac.digits, buf, sizeof(t->frac.digits)-1);
        t->frac.digits[sizeof(t->frac.digits)-1] = '\0';
    }

}


void decode_fields(my_type *t) {
	printf("Decoded fields\n");
	unsigned char *b = (unsigned char*)t;
    for (size_t i = 0; i < MY_TYPE_BITS; ++i) {
        size_t byte = i / 8;
        int    bit  = 7 - (int)(i % 8);

        if (i == 0) {
	        printf("  - sign: ");
	    } else if (i == 1) {
			printf("  - exponent: ");
	    } else if (i == SIGN_FIELD_BITS + EXPONENT_FIELD_BITS) {
	        printf("  - fraction: ");
        }
        putchar(((b[byte] >> bit) & 1) ? '1' : '0');

        if (i == 0) {
	        printf("\n");
        } else if (i == SIGN_FIELD_BITS + EXPONENT_FIELD_BITS -1) {
            printf(" (%d) => E = %d - %d = %d\n", t->exp, t->exp, BIAS, t->exp - BIAS);
        }
    }
    printf("\n");
}


void print_value(my_type *t) {
    // sign output (단, 값이 0이면 '-' 출력하지 않음)
    const char *D = t->frac.digits;
    int allzero = 1; for (int i=0; D[i] != '\0'; ++i) { if (D[i] != '0') { allzero = 0; break; } }
    if (t->sign == 1 && !allzero) printf("-");

    int E = (int)t->exp - BIAS; // 실제 소수자리수
    int nd = (int)strlen(D);
    int dot = nd - E;           // 소수점 위치

    if (E == 0) {
        printf("%s\n", D);
    } else if (dot > 0) {
        printf("%.*s.%s\n", dot, D, D + dot);
    } else {
        printf("0.");
        for (int i = 0; i < -dot; ++i) putchar('0');
        printf("%s\n", D);
    }
}

void main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <number>\n", argv[0]);
        return;
    }
	printf("my_type (%d-bit)\n", MY_TYPE_BITS);
	printf("  Layout  : [sign(%ld) | exponent(%ld) | fraction(%ld)]\n",
	    SIGN_FIELD_BITS, EXPONENT_FIELD_BITS, FRACTION_FIELD_BITS);

    printf("\n");
    printf("Value definition :\n");
    printf("  V = (-1)^sign x D x 10^(-E)\n");
    printf("  E = exponent - BIAS\n");
    printf("  BIAS = 2^(%ld-1) - 1 = %ld\n", EXPONENT_FIELD_BITS, BIAS);
    printf("  M = D (decimal integer with decimal point removed)\n");

	my_type t;
	init_type(argv[1], &t);   
	
	printf("\n");
	decode_fields(&t);
	printf("\n");
	printf("Therefore, by my design:\n");
	printf("V = ");
	print_value(&t);
}