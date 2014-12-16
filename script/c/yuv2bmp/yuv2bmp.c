#include "stdio.h"
#include "stdlib.h"
#include "string.h"

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
    unsigned int num = 0;

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

            dest_rgb8888[num * 3] = b;
            dest_rgb8888[num * 3 + 1] = g;
            dest_rgb8888[num * 3 + 2] = r;

            num++;
        }
    }
    num++;
}

void rgb_mirror_LR(unsigned char *rgb_buf , int width , int height)
{
	unsigned char *temp_buf = (unsigned char*)malloc(width*3);
	for(int i = 0 ; i < height ; i++)
	{
		memcpy(temp_buf , rgb_buf+i*width*3,width*3);
		for(int k=0;k<width;k++)
		{
			rgb_buf[((i*width+k)*3)+0]=temp_buf[(width-k-1)*3+0];
			rgb_buf[((i*width+k)*3)+1]=temp_buf[(width-k-1)*3+1];
			rgb_buf[((i*width+k)*3)+2]=temp_buf[(width-k-1)*3+2];
		}
	}
}

void rgb_mirror_UB(unsigned char *rgb_buf , int width , int height)
{
	unsigned char *temp_buf = (unsigned char*)malloc(width*3);
	for(int i = 0 ; i < height/2 ; i++)
	{
		memcpy(temp_buf , rgb_buf+i*width*3,width*3);
		memcpy(rgb_buf+i*width*3 , rgb_buf+(height-i-1)*width*3 , width*3);
		memcpy(rgb_buf+(height-i-1)*width*3 , temp_buf , width*3);
	}
}




void yuv2rgb(unsigned char *yuv_buffer, unsigned char *rgb_buffer,int width , int height)
{
	unsigned char *y_buffer=yuv_buffer;
	unsigned char *uv_buffer=yuv_buffer+width*height;
	unsigned char  *rgb_used = rgb_buffer;
	yuvtorgb888(width,height,y_buffer,uv_buffer,rgb_used);
}
//yuv2bmp yuv_name [width] [height] [mirror]
//mirror 1 for LR 2 for UB ,3 for LR and UB
int main(char argc , char **argv)
{
	FILE *file_bmp;
	FILE *file_yuv;
	int width = 3200;
	int height = 2400;
	char yuv_file_name[256]={0};
	char bmp_file_name[256]={0};
	int mirror = 0;
	if(argc < 2)
	{
		printf("Useage: yuv2bmp yuv_name [width] [height] [mirror]\n");
		printf("yuv_name : short file name , no .yuv\n");
		printf("mirror : 1 for left right mirror , 2 for up bottom mirror,3 for LR and UB\n");
		return 0;
	}
	else
	{
		sprintf(yuv_file_name,"./%s.yuv",argv[1]);
		sprintf(bmp_file_name,"./%s.bmp",argv[1]);
	}
	if(argc >= 4)
	{
		width = atoi(argv[2]);
		height = atoi(argv[3]);
	}

	if(argc >= 5)
	{
		mirror = atoi(argv[4]);
	}

	
	file_bmp = fopen(bmp_file_name,"wb+");
	if(file_bmp==0)
	{
		perror("file_bmp open:");
		return -1;
	}
	file_yuv = fopen(yuv_file_name,"rb");
	if(file_yuv==0)
	{
		perror("file_yuv open:");
		return -1;
	}
	int yuv_size=width*height*3/2;
	unsigned char *yuv_buffer = (unsigned char*)malloc(yuv_size);
	if(yuv_buffer == 0)
	{
		perror("alloc yuv buffer failed:");
		return -1;
	}
	int rgb_size=width*height*3;
	unsigned char *rgb_buffer = (unsigned char*)malloc(rgb_size);
	if(rgb_buffer == 0)
	{
		perror("alloc rgb buffer failed:");
		return -1;
	}
	int read_size = fread(yuv_buffer,1,yuv_size,file_yuv);
	if(read_size != yuv_size)
	{
		perror("read file error:");
		return -1;
	}

	yuv2rgb(yuv_buffer,rgb_buffer,width,height);
	if(mirror & 1)
	{
		rgb_mirror_LR(rgb_buffer,width,height);
	}
	if(mirror&2)
	{
		rgb_mirror_UB(rgb_buffer,width,height);
	}
	
	write_bmp_header(file_bmp,width,height);
	write_bmp_info_header(file_bmp,width,height);
	int write_size = fwrite(rgb_buffer,1,rgb_size,file_bmp);
	if(write_size != rgb_size)
	{
		perror("write file error:");
		return -1;
	}
	free(yuv_buffer);
	free(rgb_buffer);
	fclose(file_bmp);
	fclose(file_yuv);
	printf("wanggongzhen exit success\n");
	return 0;
}
