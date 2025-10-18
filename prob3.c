/*
 * ==========================================================
 *  과제 #3: Arbitrary-precision Addition/Subtraction
 * ==========================================================
 *  목표
 *    - prob2.c에서 설계한 "정확한 10진 표현 my_type"을 이용해
 *      덧셈(add)과 뺄셈(sub)을 구현한다.
 *    - 즉, float/double이 아닌 문자열 기반 덧셈/뺄셈 연산기 만들기.
 *
 *  접근 개요
 *    - my_type의 구조:
 *        V = (-1)^sign × D × 10^(-E)
 *        E = exp - BIAS
 *        D = 문자열로 저장된 정수부 (소수점 제거된 형태)
 *    - E(소수점 자리수)가 다르면, 자릿수를 맞추기 위해
 *      문자열의 끝에 0을 추가하여 소수점 자리를 정렬(alignment)한다.
 *    - 이후 문자열끼리의 덧셈 또는 뺄셈을 수행.
 *
 *  구성
 *    - strip0() : 선행 '0' 제거
 *    - cmp_mag(): 두 숫자 문자열의 크기 비교
 *    - align_E(): exponent를 맞추며 문자열 뒤에 0을 덧붙임
 *    - add()    : 부호에 따라 덧셈 또는 뺄셈 수행
 *    - sub()    : a - b = a + (-b) 형태로 처리
 */

#define PROB2_AS_LIB 1   // prob2.c의 main()을 비활성화
#include "prob2.c"       // 타입 정의 및 유틸 함수 전체 포함


/* ==========================================================
 *  [HELPER FUNCTIONS]
 * ========================================================== */

/* 문자열 선행 0 제거
 *  ex) "000123" → "123"
 *  단, 전체가 '0'이면 한 자리만 남긴다. */
static void strip0(char *s) {
    size_t k = 0;
    while (s[k] == '0' && s[k+1] != '\0') k++;
    if (k) memmove(s, s + k, strlen(s) - k + 1);
}

/* 문자열 크기 비교 (부호 무시)
 *  길이 > → 더 큰 수
 *  길이 같으면 strcmp 비교
 *  반환값: 1 (X>Y), -1 (X<Y), 0 (같음) */
static int cmp_mag(const char *X, const char *Y) {
    size_t nx = strlen(X), ny = strlen(Y);
    if (nx != ny) return (nx > ny) ? 1 : -1;
    int c = strcmp(X, Y);
    return (c > 0) - (c < 0);
}

/* 지수 정렬: 소수부 자리수(E)가 다를 경우,
 * 작은 쪽 문자열 뒤에 0을 붙여 맞춘다.
 *
 * 예) E_a=2, E_b=4 이면
 *     A="1234", B="5678" → A="123400"
 * */
static void align_E(char *A, int *Ea, char *B, int *Eb) {
    int Emax = (*Ea > *Eb) ? *Ea : *Eb;

    /* A 확장 */
    while (*Ea < Emax) {
        size_t la = strlen(A);
        if (la + 1 >= 256) { fprintf(stderr, "[fatal] add: A overflow\n"); exit(1); }
        A[la] = '0'; A[la+1] = '\0'; (*Ea)++;
    }

    /* B 확장 */
    while (*Eb < Emax) {
        size_t lb = strlen(B);
        if (lb + 1 >= 256) { fprintf(stderr, "[fatal] add: B overflow\n"); exit(1); }
        B[lb] = '0'; B[lb+1] = '\0'; (*Eb)++;
    }
}


/* ==========================================================
 *  [덧셈 함수 구현]
 * ========================================================== */
void add(const my_type *a, const my_type *b, my_type *r) {
    /* Step 1. 문자열 복사 (파괴 방지용 로컬 버퍼 사용) */
    char A[256], B[256];
    strcpy(A, a->frac.digits);
    strcpy(B, b->frac.digits);

    /* Step 2. 소수 자리수 정렬 */
    int Ea = (int)a->exp - BIAS;
    int Eb = (int)b->exp - BIAS;
    align_E(A, &Ea, B, &Eb);
    int Emax = Ea; // 두 개가 같아졌으므로 하나만 저장

    /* Step 3. 부호가 같을 경우 → 단순 덧셈 */
    if (a->sign == b->sign) {
        int i = (int)strlen(A) - 1;
        int j = (int)strlen(B) - 1;
        int carry = 0, t = 0;
        char tmp[512];

        /* 오른쪽(1의 자리)부터 자리올림 덧셈 */
        while (i >= 0 || j >= 0 || carry) {
            int da = (i >= 0) ? (A[i--] - '0') : 0;
            int db = (j >= 0) ? (B[j--] - '0') : 0;
            int s  = da + db + carry;
            tmp[t++] = (char)('0' + (s % 10));
            carry = s / 10;
        }

        /* 결과 뒤집기 (역순 저장되어 있으므로) */
        for (int m = 0; m < t; ++m)
            A[m] = tmp[t - 1 - m];
        A[t] = '\0';

        /* 선행 0 제거 */
        strip0(A);

        // [PATCH] 결과가 0일 때 –0 방지
        if (A[0] == '\0' || (A[0]=='0' && A[1]=='\0')) {
            strcpy(A, "0");
            r->sign = 0;
            r->exp  = (unsigned char)BIAS; // E=0
            strcpy(r->frac.digits, A);
            return;
        }

        /* 결과 저장 */
        if (strlen(A) >= sizeof(r->frac.digits)) {
            fprintf(stderr,"[fatal] add: result overflow\n");
            exit(1);
        }
        strcpy(r->frac.digits, A);
        r->sign = a->sign;
        r->exp  = (unsigned char)(Emax + BIAS);
        return;
    }

    /* Step 4. 부호가 다를 경우 → 큰 수에서 작은 수를 뺀다. */
    int cmp = cmp_mag(A, B);
    if (cmp == 0) {
        /* 크기가 같으면 결과는 0 */
        strcpy(r->frac.digits, "0");
        r->sign = 0;
        r->exp  = (unsigned char)BIAS; // E=0
        return;
    }

    /* 큰 쪽(L) - 작은 쪽(S) */
    const char *srcL = (cmp > 0) ? A : B;
    const char *srcS = (cmp > 0) ? B : A;
    int signL = (cmp > 0) ? a->sign : b->sign;

    char L[256], S[256], tmp[512];
    strcpy(L, srcL);
    strcpy(S, srcS);

    int i = (int)strlen(L) - 1;
    int j = (int)strlen(S) - 1;
    int borrow = 0, t = 0;

    /* 오른쪽(1의 자리)부터 자리내림 뺄셈 */
    while (i >= 0 || j >= 0) {
        int dl = (i >= 0) ? (L[i--] - '0') : 0;
        int ds = (j >= 0) ? (S[j--] - '0') : 0;
        int d  = dl - borrow - ds;
        if (d < 0) { d += 10; borrow = 1; } else borrow = 0;
        tmp[t++] = (char)('0' + d);
    }

    /* 결과 뒤집기 */
    for (int m = 0; m < t; ++m)
        L[m] = tmp[t - 1 - m];
    L[t] = '\0';

    /* 선행 0 제거 */
    strip0(L);

    // [PATCH] 결과가 0일 때 –0 방지
    if (L[0] == '\0' || (L[0]=='0' && L[1]=='\0')) {
        strcpy(L, "0");
        signL = 0;
        Emax  = 0;
    }

    /* 결과 저장 */
    if (strlen(L) >= sizeof(r->frac.digits)) {
        fprintf(stderr,"[fatal] add: result overflow\n");
        exit(1);
    }
    strcpy(r->frac.digits, L);
    r->sign = (unsigned char)signL;
    r->exp  = (unsigned char)(Emax + BIAS);
}


/* ==========================================================
 *  [뺄셈 함수 구현]
 * ==========================================================
 *  - a - b = a + (-b) 로 처리
 *  - 즉, b의 부호를 반전시켜 add() 호출
 */
void sub(const my_type *a, const my_type *b, my_type *r) {
    my_type nb = *b;
    nb.sign ^= 1;        // 부호 반전
    add(a, &nb, r);
}


/* ==========================================================
 *  [main 함수]
 * ==========================================================
 *  - 입력 두 개를 받아 add/sub 수행 결과를 출력
 *  - prob2의 print_value()를 그대로 재사용
 */
int main(int argc, char **argv) {
    // [PATCH] 인자 부족 체크
    if (argc < 3) {
        printf("Usage: %s <A> <B>\n", argv[0]);
        return 0;
    }

    my_type x, y, result;
    init_type(argv[1], &x);
    init_type(argv[2], &y);

    printf("Computed Outcome:\n");

    printf("%s + %s = ", argv[1], argv[2]);
    add(&x, &y, &result);
    print_value(&result);

    printf("%s - %s = ", argv[1], argv[2]);
    sub(&x, &y, &result);
    print_value(&result);

    return 0;
}