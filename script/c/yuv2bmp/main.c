#include "stdio.h"
#include "stdlib.h"

struct BITMAPFILEHEADER 
{   // bmfh
    char    bfType[2];
    unsigned int   bfSize;
    unsigned short    bfReserved1;
    unsigned short    bfReserved2;
    unsigned int   bfOffBits;
}__attribute((packed))  ;

struct BITMAPINFOHEADER
{    // bmih
    unsigned int   biSize;
    unsigned long    biWidth;
    unsigned long    biHeight;
    unsigned short    biPlanes;
    unsigned short    biBitCount;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    unsigned long    biXPelsPerMeter;
    unsigned long    biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
}__attribute((packed))  ;

struct RGBQUAD{   // rgbq
    char   rgbBlue;
    char   rgbGreen;
    char   rgbRed;
    char   rgbReserved;
} __attribute((packed))  ;

struct RGBTRIPLE{   // rgbt
    char   rgbBlue;
    char   rgbGreen;
    char   rgbRed;
} __attribute((packed))  ;

void write_bmp_header(FILE *file,int width , int height)
{
	struct BITMAPFILEHEADER header;
	unsigned char willwrite[sizeof(BITMAPFILEHEADER)];
	header.bfType[0] = 'B';
	header.bfType[1] = 'M';
	header.bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+width*height*sizeof(RGBTRIPLE);
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits = 54;
	fwrite(&header,1,sizeof(header),file);
}

void write_bmp_info_header(FILE *file,int width , int height)
{
	struct BITMAPINFOHEADER info_header;
	info_header.biSize = 40;
	info_header.biWidth = width;
	info_header.biHeight = height;
	info_header.biPlanes = 1;
	info_header.biBitCount = 24;
	info_header.biCompression = 0;
	info_header.biSizeImage = width*height*sizeof(RGBTRIPLE);
	info_header.biXPelsPerMeter = 0;
	info_header.biYPelsPerMeter = 0;
	info_header.biClrUsed = 0;
	info_header.biClrImportant = 0;
	fwrite(&info_header,1,sizeof(info_header),file);
}


void write_bmp_rgb(FILE *file,int width , int height)
{
	struct RGBTRIPLE rgb;
	for(int i = 0 ; i < height ; ++i)
	{
		for(int j = 0 ; j < width; ++j)
		{
			rgb.rgbBlue = i;
			rgb.rgbGreen = j;
			rgb.rgbRed = i+j;
			fwrite(&rgb,1,sizeof(rgb),file);
		}
	}
}



void yuvtorgb888(int width, int height, unsigned char *src_y, unsigned char *src_uv, unsigned char *dest_rgb8888) {
    unsigned int i, j;
    int r, g, b;
    unsigned int YPOS, UPOS, VPOS;
    unsigned int num = height * width - 1;

    for(i = 0; i < height; i++) {
        for(j = 0; j < width; j++) {
            YPOS = i * width + j;
            VPOS = (i / 2) * width + (j & 0xFFFE);
            UPOS = (i / 2) * width + (j | 0x0001);
            r = (int)(src_y[YPOS] + (1.370705 * (src_uv[VPOS] - 128)));
            g = (int)(src_y[YPOS] - (0.698001 * (src_uv[VPOS] - 128)) - (0.337633 * (src_uv[UPOS] - 128)));
            b = (int)(src_y[YPOS] + (1.732446 * (src_uv[UPOS] - 128)));

            if(r > 255)
                r = 255;
            if(r < 0)
                r = 0;

            if(g > 255)
                g = 255;
            if(g < 0)
                g = 0;

            if(b > 255)
                b = 255;
            if(b < 0)
                b = 0;

            dest_rgb8888[num * 3] = r;
            dest_rgb8888[num * 3 + 1] = g;
            dest_rgb8888[num * 3 + 2] = b;

            num--;
        }
    }
    num++;
}


void yuv2rgb(unsigned char *yuv_buffer, unsigned char *rgb_buffer,int width , int height)
{
	unsigned char *y_buffer=yuv_buffer;
	unsigned char *uv_buffer=yuv_buffer+width*height;
	unsigned char  *rgb_used = rgb_buffer;
	yuvtorgb888(width,height,y_buffer,uv_buffer,rgb_used);
	/*
	for(int i = 0 ; i < height ; ++i)
	{
		y_buffer=yuv_buffer + i*width*3/2;
		uv_buffer = y_buffer+width;
		rgb_used = rgb_buffer+i*width*3;
		yuvtorgb888(width,1,y_buffer,uv_buffer,rgb_used);
	}
	*/
}

int main(char argc , char **argv)
{
	FILE *file_bmp;
	FILE *file_yuv;
	int width = 3200;
	int height = 2400;
	file_bmp = fopen("./test.bmp","w+");
	if(file_bmp==0)
	{
		perror("file_bmp open ./test.bmp:");
	}
	file_yuv = fopen("./1.yuv","r");
	if(file_yuv==0)
	{
		perror("file_yuv open :");
	}
	int yuv_size=width*height*3/2;
	unsigned char *yuv_buffer = (unsigned char*)malloc(yuv_size);
	int rgb_size=width*height*3;
	unsigned char *rgb_buffer = (unsigned char*)malloc(rgb_size);
	fread(yuv_buffer,1,yuv_size,file_yuv);
	yuv2rgb(yuv_buffer,rgb_buffer,width,height);
	printf("wanggongzhen\n");
	write_bmp_header(file_bmp,width,height);
	write_bmp_info_header(file_bmp,width,height);
	fwrite(rgb_buffer,1,rgb_size,file_bmp);
	
	free(yuv_buffer);
	free(rgb_buffer);
	fclose(file_bmp);
	fclose(file_yuv);
	return 0;
}
