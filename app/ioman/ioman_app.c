#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "binder_ioctl.h"
#include <termios.h>
#include <string.h>



void set_rawMode(){
  struct termios  newt;
  int oldf;

  cfmakeraw(&newt);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
}

void get_service(int fd){

	while(ioctl(fd,IOC_REGISTER_SERVICE,PT_OSD)<0){
		printf("no service\r");
	}
	printf("\n");
}

int main(){
	struct termios oldt;
	int fd;
	int a = PT_MONITOR;
	char buffer[16];	 
	char  ch;

	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("ioman\n");

	ioctl(fd, IOSET_TYPE, PT_IOMAN);

	get_service(fd);

	printf("send: ping\n");
	ioctl(fd, IO_PING, buffer);

	tcgetattr(STDIN_FILENO, &oldt);
	set_rawMode();

	while(1){

		ch = getchar();
		if (ch>=0) {
			a = write(fd,&ch,1);
		}

		if(a < 0){
			get_service(fd);
		}
		if(ch == 'q')
			break;
	}
	
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	close(fd);	

	return 0;

}
