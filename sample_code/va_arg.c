#include <stdio.h> 
#include <stdarg.h>

void fun_va_arg(int a, ...) 
{ 
    int p = 0;
    va_list argp; 
    va_start(argp,a); 
    while((p=va_arg(argp,int))!= 0)
    {
        printf("%d\n",p);;
    }
    va_end(argp);
}

int main() 
{ 
    int a = 1; 
    int b = 2; 
    int c = 3; 
    int d = 4; 
    int e = 0;
    fun_va_arg(4, a, b, c, d,e); 
    return 0; 
}
 
/*
the Output:: 
1 
2 
3 
4
*/
