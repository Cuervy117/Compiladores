// Factorial y promedio: usa funciones, ciclos, if/else y printf/scanf
int factorial(int n) {
    int result = 1;
    while (n > 1) {
        result = result * n;
        n -= 1;
    }
    return result;
}

float average(int a, int b, int c) {
    return (a + b + c) / 3.0f;
}

int main() {
    int i, total = 0;
    float avg;
    char grade = 'A';

    for (i = 1; i <= 5; i += 1) {
        if (i % 2 == 0)
            continue;
        total += factorial(i);
    }

    avg = average(total, 10, 20);
    if (avg >= 90.0) {
        grade = 'A';
    } else if (avg >= 80.0) {
        grade = 'B';
    } else
        grade = 'C';

    printf("total = %d, promedio = %.2f, grado %c\n", total, avg, grade);
    scanf("%d", &i);
    print(total);
    return 0;
}
