#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BACKTRACE_COUNT 1000
static inline void print_backtrace(void)
{
  void* array[BACKTRACE_COUNT] = {0};
  int size = 0;
  char **strframe = 0;
  int i = 0, j = 0;
  
  size = backtrace(array, BACKTRACE_COUNT);
  strframe = backtrace_symbols(array, size);
  
  printf("print call frame now:\n");
  for(i = 0; i < size; i++){
    printf("frame %d -- %s\n", i, strframe[i]);
  }
  
  if(strframe)
  {
    free(strframe);
    strframe = 0;
  }
}

#define BACKTRACE_TEST_ONLY
#ifdef BACKTRACE_TEST_ONLY
void fun2(void)
{
  print_backtrace();
}

void fun1(void)
{
  fun2();
}

int main(void)
{
  fun1();
  return 0;
}
#endif




/*

linux-xms:/data/test # gcc  test.c

linux-xms:/data/test # ./a.out

print call frame now:
frame 0 -- ./a.out [0x80484fe]
frame 1 -- ./a.out [0x8048582]
frame 2 -- ./a.out [0x804858f]
frame 3 -- ./a.out [0x80485a7]
frame 4 -- /lib/libc.so.6(__libc_start_main+0xdc) [0xb7e188ac]
frame 5 -- ./a.out [0x8048431]

只能看到地址

 

修改编译参数

linux-xms:/data/test # gcc -rdynamic  test.c 
linux-xms:/data/test # ./a.out 
print call frame now:
frame 0 -- ./a.out(fun3+0x4a) [0x80486de]
frame 1 -- ./a.out(fun2+0xb) [0x8048762]
frame 2 -- ./a.out(fun1+0xb) [0x804876f]
frame 3 -- ./a.out(main+0x16) [0x8048787]
frame 4 -- /lib/libc.so.6(__libc_start_main+0xdc) [0xb7e588ac]
frame 5 -- ./a.out [0x8048611]

现在可以看到函数名了，但没有行号，不过没关系addr2line提供了这个功能

 

然后我们试图用addr2line来看地址对应的函数和行号

linux-xms:/data/test # addr2line 0x80486de -e ./a.out -f
fun3
??:0

失败了，别急，我们再次修改编译参数

linux-xms:/data/test # gcc -g -rdynamic test.c 
linux-xms:/data/test # ./a.out 
print call frame now:
frame 0 -- ./a.out(fun3+0x4a) [0x80486de]
frame 1 -- ./a.out(fun2+0xb) [0x8048762]
frame 2 -- ./a.out(fun1+0xb) [0x804876f]
frame 3 -- ./a.out(main+0x16) [0x8048787]
frame 4 -- /lib/libc.so.6(__libc_start_main+0xdc) [0xb7dcb8ac]
frame 5 -- ./a.out [0x8048611]
linux-xms:/data/test # addr2line 0x80486de -e ./a.out -f
fun3
/data/test/test.c:14

这次我们成功了，多加了-g参数。


*/