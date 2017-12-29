#include <stdio.h>

#define GENERATE_ENUM(ENUM) ENUM
#define GENERATE_STRING(STRING) #STRING


#define GENERATE_ENUM_STRING(ENUMNAME,STRINGNAME,ENUM_ENTRY)	\
enum ENUMNAME {													\
	ENUM_ENTRY(GENERATE_ENUM)									\
};																\
const char *STRINGNAME[] = {									\
	ENUM_ENTRY(GENERATE_STRING)									\
}																\



#define ENUM_COLOR(ENUMSTRING)									\
ENUMSTRING(RED),												\
ENUMSTRING(BLUE),												\
ENUMSTRING(GREEN)												\

/*
enum color {
	ENUM_COLOR(GENERATE_ENUM)
};

const char *color_strings[] = {
	ENUM_COLOR(GENERATE_STRING)
};

*/


GENERATE_ENUM_STRING(color,color_strings,ENUM_COLOR);

void printenum(color c)
{
	printf("color is %s\n" , color_strings[c]);
}

int main(int argc,char **argv)
{
	color c = RED;
	printenum(c);
	c = BLUE;
	printenum(c);
}
