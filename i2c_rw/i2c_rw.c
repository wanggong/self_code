#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/i2c.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>

//#include <linux/i2c-dev.h> 


#define I2C_DEV "/dev/i2c-0"
static int addr_width = 1;

static int iic_read(int fd, char buff[], int addr, int count)
{
    int res;
    char sendbuffer1[2];
	if(addr_width == 1)
	{
		sendbuffer1[0]=addr;
	}
	else
	{
		sendbuffer1[0]=addr>>8;
    	sendbuffer1[1]=addr;
	}
    
    res = write(fd,sendbuffer1,addr_width); 
	if(res < 0)
	{
		printf("write address error %s\n" , strerror(errno));
	}
//	printf("start read count = %d\n" , count);
    res=read(fd,buff,count);
	if(res < 0)
	{
		printf("read error %s\n" , strerror(errno));
	}    
    return res;
}
static int iic_write(int fd, char buff[], int addr, int count)
{
        int res;
        int i,n;
        char sendbuffer[4096];
        memcpy(sendbuffer+addr_width, buff, count);
		if(addr_width == 1)
		{
			sendbuffer[0]=addr;
		}
		else
		{
	        sendbuffer[0]=addr>>8;
	    	sendbuffer[1]=addr;
		}
        res=write(fd,sendbuffer,count+addr_width);
		if(res < 0)
		{
			printf("write error %s\n" , strerror(errno));
		}
		return res;
}



typedef struct{
    char rw;
    __u8 command;
    int size;
    union i2c_smbus_data *data;
}i2c_smbus_ioctl_data_t;

static inline __s32 i2c_smbus_access(int fd, char rw, __u8 command, int size, union i2c_smbus_data *data)
{
    i2c_smbus_ioctl_data_t args;

    args.rw = rw;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(fd,I2C_SMBUS,&args);
}

static inline __s32 i2c_smbus_read_byte(int file)
{
    union i2c_smbus_data data;
    if (i2c_smbus_access(file,I2C_SMBUS_READ,0,I2C_SMBUS_BYTE,&data))
        return -1;
    else
        return 0x0FF & data.byte;
}

static int i2c_scan_bus(int file)
{
    int i, j;
    int res;
	int usage = 0;
    printf("     0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f\n");

    for (i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for(j = 0; j < 16; j++) {
            fflush(stdout);
			usage = 0;
            if (ioctl(file, I2C_SLAVE, i+j) < 0) {
                if (errno == EBUSY) 
				{
                    printf("U");
					usage = 1;
                    if (ioctl(file, I2C_SLAVE_FORCE, i+j) < 0)
                	{
                		fprintf(stderr, "Could not force set address to 0x%02x: %s\n", i+j,strerror(errno));
						return -1;
                	}
                } 
				else 
                {
                    fprintf(stderr, "Could not set address to 0x%02x: %s\n", i+j,strerror(errno));
                    return -1;
                }
            }
            res = i2c_smbus_read_byte(file);
			if(usage == 0)
			{
	            if (res < 0){
	                printf("--- ");
	            }
	            else{
	                printf("-%02x ", i+j);
	            }
			}
			else
			{
				if (res < 0){
	                printf("-- ");
	            }
	            else{
	                printf("%02x ", i+j);
	            }
			}
        }
        printf("\n");
    }
    return 0;
}

int i2c_detect_main(char *dev_name)
{
    int fd, res;
    unsigned long funcs;

    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open file /dev/i2c- error: %s\n", strerror(errno));
        exit(1);
    }

    if (ioctl(fd, I2C_FUNCS, &funcs) < 0) {
        fprintf(stderr, "Could not get the adapter: %s\n", strerror(errno));
        exit(1);
    }
    res = i2c_scan_bus(fd);
    exit(res?1:0);
}

int todig(char *dig)
{
        int ret = 0;
        if(dig[0] == '0' && (dig[1] == 'x' || dig[1] == 'X'))
        {
                sscanf(dig+2, "%x" , &ret);
        }
        else
        {
                sscanf(dig, "%d" , &ret);
        }
        return ret;

}


int main(int argc , char** argv){
    int fd;
    int res;
    char ch;
    char buf[4096] = {0};
    int regaddr = 0;
	int i= 0 ;
	int slaveaddr = 0;
	int count = 1;
	char* i2c_dev_name = argv[1];

	if(argc != 6 &&argc != 7 && argc != 2)
	{
		printf("usage:i2c_rw /dev/i2c-n register_addr_width count slaveaddr regaddr [wirte_value] \n");
		printf("or i2c_rw /dev/i2c-n to detect\n");
		printf("eg:i2c_rw /dev/i2c-0 1 1 106 0 1 \n");
	}

	if(argc == 2)
	{
		i2c_detect_main(argv[1]);
		return 0; 
	}
	
    fd = open(i2c_dev_name, O_RDWR);// I2C_DEV /dev/i2c-0
    if(fd < 0)
	{
            printf("####i2c test device open failed:%s\n" , strerror(errno));
            goto ERROR;
    }
//	addr_width = atoi(argv[2]);
//	count = atoi(argv[3]);
//	slaveaddr = atoi(argv[4]);
//	regaddr = atoi(argv[5]);

	sscanf(argv[2] , "%d", &addr_width);
	addr_width = todig(argv[2]);
	count = todig(argv[3]);
	slaveaddr = todig(argv[4]);
	regaddr = todig(argv[5]);

//	sscanf(argv[3] , "%d",&count);
//	sscanf(argv[4] , "%d",&slaveaddr);
//	sscanf(argv[5] , "%d", &regaddr);
    
    res = ioctl(fd,I2C_TENBIT,0);   //not 10bit
    if(res < 0)
	{
		printf("ioctl(fd,I2C_TENBIT,0) error:%s\n" , strerror(errno));
		goto ERROR;
	}
    res = ioctl(fd,I2C_SLAVE_FORCE,slaveaddr);    //����I2C���豸��ַ[6:0]
    if(res < 0)
	{
		printf("ioctl(fd,I2C_SLAVE,%d) error:%s\n" , slaveaddr , strerror(errno));
		goto ERROR;
	}
    
   if(argc == 6)
	{
            res=iic_read(fd,buf,regaddr,count);
			if(res < 0)
			{
				goto ERROR;
			}
			else
			{
	            for(i=0;i<res;i++){
					
					printf("%02x ",buf[i]);
					
					if((i+1)%8==0)
					{
						printf("\t");
					}
					if((i+1)%16==0)
					{
						printf("\n");
					}
	            }
	            printf("\n");
			}
	}
    else if (argc == 7)
	{
            int writeto = 0;
			sscanf(argv[6] , "%d", &writeto);
            res=iic_write(fd,(char*)&writeto,regaddr,count);
			if(res < 0)
			{
				goto ERROR;
			}
            printf("%d bytes write success\n" , res);
	}
	return 0;
	ERROR:
	printf("dev name i2c_dev_name=%s\n" , i2c_dev_name);
	printf("slave addr is:0x%x\n",slaveaddr);
    printf("reg addr is:0x%x\n",regaddr);	
      
    return -1;
}