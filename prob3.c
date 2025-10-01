void add(const my_type *a, const my_type *b, my_type *r){
// [TODO] Complete this function
}

void sub(const my_type *a, const my_type *b, my_type *r){
    /* a - b = a + (-b) */
    my_type nb = *b; 
    nb.sign ^= 1;
    add(a, &nb, r);
}

void main(int argc, char **argv) {
	my_type x, y, result;
	init_type(argv[1], &x);   
    init_type(argv[2], &y);  

    printf("Computed Outcome:\n");
    printf("%s + %s = ", argv[1], argv[2]);
    add(&x, &y, &result);
    print_value(&result);
    printf("\n");    
    
    printf("%s - %s = ", argv[1], argv[2]);
    sub(&x, &y, &result);
    print_value(&result);
    printf("\n"); 
}

/*
./prob3 123412341234123412341234.12341234 98989898989898989898989898.9898989898989898989898

Computed Outcome:
123412341234123412341234.12341234 + 98989898989898989898989898.9898989898989898989898 = 99113311331133113311331133.1133113298989898989898
123412341234123412341234.12341234 - 98989898989898989898989898.9898989898989898989898 = -98866486648664866486648664.8664866498989898989898
*/