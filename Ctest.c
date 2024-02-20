#include <stdio.h>
#include <stdlib.h>

int factorial(int a){
    int ans = 1;
    for(a; a > 1 ;a--){
        ans = ans * a;
    }
    return ans;
}

int main() {
    int num = 6;
    int result = factorial(num);
    printf("%d\n", result);
    return 0;
}