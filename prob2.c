#include <string.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * ============================
 *  과제 #2: 10진 기반 커스텀 타입
 * ============================
 * 목표
 *  - 이진 부동소수(float/double)로 정확히 표현 불가능한 10진수를
 *    "정확히" 저장/출력하는 구조를 직접 설계/구현한다.
 *
 * 핵심 설계 아이디어
 *  - 값 V = (-1)^sign × D × 10^(-E)
 *    · sign  : 부호(0=+, 1=-)
 *    · E     : 소수점 이하 자리수 (exponent 필드에서 BIAS를 뺀 값)
 *    · D     : 소수점을 제거한 10진 정수(문자열로 보관, 절대 라운딩/절단 없음)
 *
 * 장점
 *  - 10진 입력 "그대로"를 문자열로 저장 → 이진 변환 오차가 없다.
 *  - 출력 시 E만큼 소수점을 왼쪽으로 이동하여 정확히 복원한다.
 *
 * 주의
 *  - 성능 최적화보다 “정확성”에 초점.
 *  - digits[] 버퍼 초과 시 즉시 에러(프로그램 종료)로 명확히 알린다.
 */

/* ===== 비트폭 정의(교수님 템플릿을 따름) ===== */
#define SIGN_FIELD_BITS 1
#define EXPONENT_FIELD_BITS 8
/* fraction은 우리가 정의한 구조체(frac_part)의 전체 비트수로 간주 */
#define FRACTION_FIELD_BITS (sizeof(frac_part)*8)

/* 전체 타입 비트수(출력용 정보) */
#define MY_TYPE_BITS (FRACTION_FIELD_BITS+EXPONENT_FIELD_BITS+SIGN_FIELD_BITS)

/* exponent 바이어스 (IEEE754와 같은 개념) */
#define BIAS (int)(pow(2, EXPONENT_FIELD_BITS-1) - 1)

/*
 * ===== fraction 저장 형식 =====
 *  - D(소수점 제거 10진 정수)를 "문자열"로 저장
 *  - digits 크기는 과제 예시(수십 자리) 대비 넉넉히 64바이트로 설정
 *    (마지막 '\0' 포함 최대 63자리)
 *  - 필요 시 크기만 키우면 확장 가능
 */
typedef struct {
    char digits[64];
} frac_part;

/*
 * ===== 최종 타입 my_type =====
 *  - sign : 1비트 비트필드
 *  - exp  : 8비트 비트필드 (E = exp - BIAS)
 *  - frac : 위에서 정의한 10진 정수 문자열
 */
typedef struct {
    unsigned char sign: SIGN_FIELD_BITS;
    unsigned char exp: EXPONENT_FIELD_BITS;
    frac_part frac;
} my_type;


/* -----------------------------------------------------------
 * 입력 문자열을 (+/-)(정수부).(소수부) 형태로 분리하는 헬퍼
 *  - sign, int_part, frac_part에 결과 저장
 *  - 예) "-1234.5678" → sign='-', int_part="1234", frac_part="5678"
 *  - 소수점이 없으면 frac_part는 빈 문자열("")이 된다.
 * ----------------------------------------------------------- */
void split(const char *number, char *sign, char *int_part, char *frac_part) {
    if (number[0] == '+') {
        *sign = '+';
        number++;            /* 부호 문자 건너뛰기(로컬 포인터만 이동) */
    } else if (number[0] == '-') {
        *sign = '-';
        number++;            /* 부호 문자 건너뛰기 */
    } else {
        *sign = '+';
    }

    /* 정수부 수집 ('.' 또는 문자열 끝 전까지) */
    int i = 0;
    while (*number != '\0' && *number != '.') {
        int_part[i++] = *number;
        number++;
    }
    int_part[i] = '\0';

    /* 소수부 수집 ('.' 뒤부터 끝까지) */
    int f = 0;
    if (*number == '.') {
        number++; /* '.' 건너뛰기 */
        while (*number != '\0') {
            frac_part[f++] = *number;
            number++;
        }
        frac_part[f] = '\0';
    }
}


/* -----------------------------------------------------------
 * 입력 문자열을 my_type으로 인코딩
 *  - sign  : '+' → 0, '-' → 1
 *  - exp   : (E + BIAS) 저장. 여기서 E = (소수부 길이)
 *  - frac  : D = int_part + frac_part (선행 '0' 제거, 단 전체 0은 유지)
 * ----------------------------------------------------------- */
void init_type(const char *number, my_type *t) {
    char sign;
    char int_part[256];   /* 파싱용 임시 버퍼(넉넉히) */
    char frac_part[256];

    split(number, &sign, int_part, frac_part);

    /* sign 설정
     * 주의: 아래 number++는 현재 함수 내에서는 의미 없음(로컬 포인터).
     *       남겨둔 이유는 원본 템플릿 흐름을 유지하기 위함. 제거해도 동작 동일.
     */
    if (sign == '+') {
        t->sign = 0;
        number++;
    } else if (sign == '-') {
        t->sign = 1;
        number++;
    }

    /* exponent 설정
     * E = 소수부 자리수 = strlen(frac_part)
     * exp = E + BIAS (범위 보호를 위해 [0, (1<<EXPONENT_FIELD_BITS)-1]로 클램프)
     */
    {
        int E = (int) strlen(frac_part);
        int biased = E + BIAS;
        int maxexp = (1 << EXPONENT_FIELD_BITS) - 1;
        if (biased < 0) biased = 0;
        if (biased > maxexp) biased = maxexp;
        t->exp = (unsigned char) biased;
    }

    /* fraction(D) 설정
     * D = int_part + frac_part (소수점 제거)
     *  - 전체 선행 '0' 제거 (단, 전체가 0이면 '0' 한 자리 유지)
     *  - digits[] 용량을 "절대" 넘지 않도록 체크(넘으면 즉시 에러)
     */
    {
        const size_t cap = sizeof(t->frac.digits); /* NULL 포함 총 용량 */
        char tmp[512];
        tmp[0] = '\0';

        /* 정수부가 비어 있으면 '0'부터 시작 */
        if (int_part[0] != '\0')
            strncat(tmp, int_part, sizeof(tmp)-1);
        else
            strncat(tmp, "0", sizeof(tmp)-1);

        /* 소수부 이어붙이기 */
        strncat(tmp, frac_part, sizeof(tmp)-1);

        /* 선행 '0' 제거(단, 전체가 '0'이면 한 자리 남김) */
        size_t k = 0;
        while (tmp[k] == '0' && tmp[k + 1] != '\0') k++;
        if (k)
            memmove(tmp, tmp + k, strlen(tmp) - k + 1);

        /* digits[] 용량 검사: 라운딩/절단 금지 → 넘치면 즉시 종료 */
        size_t need = strlen(tmp) + 1; /* NULL 포함 */
        if (need > cap) {
            fprintf(stderr, "[fatal] fraction exceeds buffer: need=%zu, cap=%zu (increase digits[])\n", need, cap);
            exit(1);
        }

        /* 안전 복사 */
        memcpy(t->frac.digits, tmp, need);
    }
}

/* -----------------------------------------------------------
 * 내부 저장 필드(sign, exp, frac)를 디코딩해서 보기 좋게 출력
 *  - exp는 비트열(상위비트부터)도 함께 보여줘서 과제 요구 형식 충족
 * ----------------------------------------------------------- */
void decode_fields(my_type *t) {
    printf("Decoded fields\n");

    /* sign 출력 */
    printf("  - sign: %u\n", t->sign);

    /* exponent 비트열 + 실제 E값(E = exp - BIAS) */
    printf("  - exponent: ");
    for (int k = EXPONENT_FIELD_BITS - 1; k >= 0; --k)
        putchar(((t->exp >> k) & 1) ? '1' : '0');
    printf(" (%u) => E = %u - %d = %d\n", t->exp, t->exp, BIAS, t->exp - BIAS);

    /* fraction 문자열 그대로 */
    printf("  - fraction: %s\n", t->frac.digits);

    printf("\n");
}

/* -----------------------------------------------------------
 * 저장된 my_type을 사람이 읽는 10진수 문자열로 "정확히" 출력
 *  - D 문자열에 소수점을 왼쪽으로 E칸 이동시키는 방식
 *  - 예) D="12341234", E=8 → "0.12341234"
 * ----------------------------------------------------------- */
void print_value(my_type *t) {
    const char *D = t->frac.digits;

    /* 전부 '0'인지 검사하여 -0 출력 방지 */
    int allzero = 1;
    for (int i = 0; D[i] != '\0'; ++i) {
        if (D[i] != '0') { allzero = 0; break; }
    }
    if (t->sign == 1 && !allzero) printf("-");

    int E = (int) t->exp - BIAS; /* 실제 소수자리수 */
    int nd = (int) strlen(D);
    int dot = nd - E;             /* 소수점 위치(왼쪽에서 몇 자리 뒤인지) */

    if (E == 0) {
        /* 소수부가 없으면 그대로 출력 (예: D="1234") */
        printf("%s\n", D);
    } else if (dot > 0) {
        /* 정수부가 존재 (예: D="12341234", E=2 → "123412.34") */
        printf("%.*s.%s\n", dot, D, D + dot);
    } else {
        /* dot <= 0 : "0.00...D" 형태 (예: D="1234", E=8 → "0.00001234") */
        printf("0.");
        for (int i = 0; i < -dot; ++i) putchar('0');
        printf("%s\n", D);
    }
}

/*
 * 단독 실행 모드 (PROB2_AS_LIB 미정의 시)
 *  - prob3.c에서 재사용할 때는 PROB2_AS_LIB를 정의하여
 *    여기 main()이 컴파일되지 않도록 한다.
 */
#ifndef PROB2_AS_LIB //
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <number>\n", argv[0]);
        return 0;
    }
    printf("my_type (%d-bit)\n", (int) MY_TYPE_BITS);
    printf("  Layout  : [sign(%ld) | exponent(%ld) | fraction(%ld)]\n",
           (long) SIGN_FIELD_BITS, (long) EXPONENT_FIELD_BITS, (long) FRACTION_FIELD_BITS);

    printf("\n");
    printf("Value definition :\n");
    printf("  V = (-1)^sign x D x 10^(-E)\n");
    printf("  E = exponent - BIAS\n");
    printf("  BIAS = 2^(%ld-1) - 1 = %ld\n", (long) EXPONENT_FIELD_BITS, (long) BIAS);
    printf("  M = D (decimal integer with decimal point removed)\n");

    my_type t;
    init_type(argv[1], &t);

    printf("\n");
    decode_fields(&t);
    printf("\n");
    printf("Therefore, by my design:\n");
    printf("V = ");
    print_value(&t);
    return 0;
}
#endif