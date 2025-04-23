
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * ./button_test /dev/100ask_gpio_flame
 *
 */
int main(int argc, char **argv)
{
	int fd;
	int val;
	
	/* 1. 判断参数 */
	if (argc != 2) 
	{
		printf("Usage: %s <dev>\n", argv[0]);
		return -1;
	}

	/* 2. 打开文件 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	while (1)
	{
		/* 3. 读文件 */
		read(fd, &val, 4);//无数据会阻塞，相当于阻塞休眠模式？
		printf("get flame : 0x%x\n", val);
		if(val & 0x1)
		{
			printf("Fired,beep ring!\n");
		}
		else
		{
			printf("Safe,beep mute!\n");
		}
		
	}
	
	close(fd);
	
	return 0;
}


