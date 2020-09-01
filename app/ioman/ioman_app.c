#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "binder_ioctl.h"
#include <termios.h>
#include <string.h>

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}


int main(){
	int fd;
	int a = PT_MONITOR;
	//char *buffer;
	//char buffer[16] = {0,};	// 
	char buffer[16];	// 

	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("binder detect\n");

	ioctl(fd, IOSET_TYPE, PT_IOMAN);
	if(ioctl(fd,IOC_REGISTER_SERVICE,PT_OSD)<0){
		printf("no service\n");
		return -1;
	}

	printf("send: ping\n");
	ioctl(fd, IO_PING, buffer);


	while(1){
		if(kbhit()){
			memset(buffer,0,16);
			read(0,buffer,15);
			write(fd,buffer,15);
		}
	}

	close(fd);	

	return 0;

}
