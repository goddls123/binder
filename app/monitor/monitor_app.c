
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include "binder_ioctl.h"

pid_t child_pid[2];

void ex_osd(){

	child_pid[0] =fork();

	if(child_pid[0] ==0){
		printf("child process\n");
		execl("../bin/osd","osd",NULL);
	}
}

void ex_ioman(){

	child_pid[1] =fork();

	if(child_pid[1] ==0){
		printf("child process\n");
		execl("../bin/ioman","ioman",NULL);
	}
}

static void child_handler(int sig){
	int status;
	pid_t pid = waitpid(-1,&status,WNOHANG);

	if(!pid)
		return ;
	fprintf(stderr,"CHILD EXIT %d\n",pid);

	fprintf(stderr,"restart\n");	
	if(pid == child_pid[0])
		ex_osd();
	else if(pid == child_pid[1])
		ex_ioman();
}

static void kill_handler(int sig){

	printf("process\n");
	printf("killed\n");
	kill(child_pid[0],SIGKILL);
	kill(child_pid[1],SIGKILL);
	kill(getpid(),SIGINT);

}

int main()
{
	int fd;
	int a = PT_MONITOR;
	char buffer[16] = {0,};	 
	int r;
	pid_t pid;


	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("monitor\n");

	ioctl(fd, IOSET_TYPE, PT_MONITOR);
	
	ex_osd();
	ex_ioman();

	signal(SIGCHLD,(void *)child_handler);
	signal(SIGTERM,(void *)kill_handler);

	printf("start\n");

	while(1){
	
		sleep(0.5);
	}

	close(fd);	
	return 0;
}
