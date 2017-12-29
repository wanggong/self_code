#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

int fork_test_for_file_and_vm()
{
	//echo 123456 > Data.txt
	int data_fd = open("Data.txt", O_RDONLY);
	char buffer[256];
	int bufsize = 3;
	int pid = 0;;
	int data_processed;
	int vmtest = 999;
	if(data_fd > 0)
	{
		pid = fork();
		if(pid == -1)
		{
			fprintf(stderr, "Fork failure");
			exit(EXIT_FAILURE);
		}
		if(pid == 0)
		{
			//子进程中
			//读取数据，这里读取到的数据是123
			data_processed = read(data_fd , buffer, bufsize);
			vmtest = 200;
			//这里打印的vmtest是200
			printf("Read %d bytes: %s ,vmtest:%d,vmtest addr:%p\n", data_processed, buffer , vmtest , &vmtest);
			//等待父进程执行完
			sleep(4);
			exit(EXIT_SUCCESS);
		}
		else
		{
			//先让子进程运行
			sleep(2);
			//父进程中
			//写数据，这里读取到的是456，因为子进程刚才读取了数据，更新了文件的pos，所以这里从第4个字符开始读取，
			//同时也说明父进程和子进程是共享struct file的
			data_processed = read(data_fd , buffer, bufsize);
			//这里打印的vmtest是999，说明内存空间他们是分开的了。
			printf("Read %d bytes: %s ,vmtest:%d,vmtest addr:%p\n", data_processed, buffer , vmtest , &vmtest);
			//休眠2秒，主要是为了等子进程先结束，这样做也只是纯粹为了输出好看而已
			//父进程其实没有必要等等子进程结束
			sleep(6);
			exit(EXIT_SUCCESS);
		}
	}
	exit(EXIT_FAILURE);
}

int main(int argc , char **argv)
{
	fork_test_for_file_and_vm();
}