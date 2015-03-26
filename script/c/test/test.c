#include "stdio.h"
struct aaaa
{
	char a;
	char b;
};
int main(int argc , char **argv)
{
	char aa[20]="abcdefgh";
	char *p = aa;
	aaaa *p2 = (aaaa*)p++;
	printf("%c,%c",p2->a,p2->b);
	
}