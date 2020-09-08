
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

pid_t child_pid[4];
int fd;

int execute(const char* path,int type){

	child_pid[type] =fork();

	if(child_pid[type] ==0){
		close(fd);
		execl(path, path, NULL);
	}
	else if(child_pid[type]>0)
		printf("osd pid %d\n",child_pid[type]);
	else
		return -1;
}

static void child_handler(int sig){
	int status;
	pid_t pid = waitpid(-1,&status,WNOHANG);

	if(!pid)
		return ;
	fprintf(stderr,"CHILD EXIT %d\n",pid);

	fprintf(stderr,"restart\n");	
	if(pid == child_pid[PT_OSD])
		execute("../bin/osd",PT_OSD);
	else if(pid == child_pid[PT_IOMAN])
		execute("../bin/ioman",PT_IOMAN);
}

static void wait_pid(int sig)
{
	int status;
	pid_t pid = waitpid(-1,&status,WNOHANG);

	if(!pid)
		return ;
	fprintf(stderr,"CHILD EXIT %d\n",pid);
}

static void kill_handler(int sig){

	signal(SIGCHLD,(void *)wait_pid);
	kill(child_pid[PT_IOMAN],SIGKILL);
	sleep(1);
	kill(child_pid[PT_OSD],SIGKILL);
	sleep(1);
	kill(getpid(),SIGINT);
}

int main()
{
	int a = PT_MONITOR;
	char buffer[16] = {0,};	 
	int r;
	pid_t pid;
	int status;


	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("monitor\n");

	ioctl(fd, IOSET_TYPE, PT_MONITOR);
	
	if(execute("../bin/osd",PT_OSD)<0)
		return -1;
	if(execute("../bin/ioman",PT_IOMAN)<0)
		return -1;

	signal(SIGCHLD,(void *)child_handler);
	signal(SIGTERM,(void *)kill_handler);

	printf("start\n");

	

	while(1){
	
		sleep(0.5);
	}

	close(fd);	
	return 0;
}
