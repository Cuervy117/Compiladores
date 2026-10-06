int decimal = 42;
    int octal = 0755;
    int hexadecimal = 0x1A2B3C;
    ull_int big_number = 9223372036854775807ULL;
    
    float pi_f = 3.14159f;
    double sci_num = 2.5e-4;
    double sci_num2 = 1.0E+10;
    long double big_float = .5678L;

    // 2. CARACTERES Y CADENAS (Secuencias de escape)
    char simple_char = 'A';
    char newline = '\n';
    char hex_char = '\x1F';
    char tab = '\t';
    char quote = '\'';

    char *string1 = "Cadena normal con un \"salto\" y tab\t.";
    char *string2 = "Cadena que " "se concatena " "automaticamente."; // C string concatenation
    char *empty_string = "";

    // 3. OPERADORES ARITMÉTICOS Y DE ASIGNACIÓN
    int a = 10, b = 3, c = 0;
    c = a + b - (a * b) / MAX_CAPACITY % 2;
    a += 5; b -= 2; c *= a; a /= 2; b %= 3;
    a++; ++b; c--; --a;

    // 4. OPERADORES RELACIONALES Y LÓGICOS
    _Bool flag = (a == b) && (c != 0) || !(a <= b) && (c >= 10);
    int ternary = flag ? MAX(a, b) : -1;

    // 5. OPERADORES DE BITS (Bitwise)
    unsigned int bits1 = 0b101010; // GCC extension para binarios, útil para probar
    unsigned int bits2 = 0x0F;
    unsigned int bit_result = (bits1 & bits2) | (bits1 ^ ~bits2);
    bit_result <<= 2;
    bit_result >>= 1;

    // 6. PALABRAS CLAVE Y CONTROL DE FLUJO
    if (a > 0) {
        goto error_label; // Goto y etiquetas
    } else if (a == 0) {
        return 1;
    } else {
        // Do nothing
    }

    switch (hexadecimal) {
        case 0x1A2B3C:
            break;
        default:
            a = 0;
    }

    for (int i = 0; i < 100; ++i) {
        while (b > 0) {
            b--;
            if (b == 5) continue;
        }
    }

    do {
        a++;
    } while (a < 10);

    // 7. PUNTEROS, ESTRUCTURAS Y OPERADORES DE TAMAÑO
    struct Node n1 = {1, "Test", NULL};
    struct Node *ptr = &n1;
    ptr->_id_numero_1 = sizeof(struct Node); // Acceso con -> y sizeof
    (*ptr).name = "Cambiado";                // Acceso con . y *
