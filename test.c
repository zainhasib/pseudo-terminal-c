/*
 *	Test program to check the usage of pseudo terminals
 *  @author Zain
 */

#define _XOPEN_SOURCE 800
#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <error.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

#define MAX_BUF 1024

int run_pty_cmd(char **argv, char * const *env);

void set_env(char **env, char *value);

void listen_for_read_and_write(int fd);

void stop_signal_handler(int sig_num);

int
run_pty_cmd(char **args, char * const *env)
{
	int master_fd, slave_fd, n;
	char *slave_device;
	char buf[MAX_BUF];
	int fp;
	struct winsize winsz = {50, 80, 0, 0};
	if ((master_fd = posix_openpt(O_RDWR|O_NOCTTY)) == -1) {
		perror("Error getting fd for master pty with error number");
		exit(-1);
	}
	if (grantpt(master_fd) == -1) {
		printf("Unable to grant ownership\n");
		exit(-1);
	}
	if (unlockpt(master_fd) == -1) {
		printf("Unable to unlock master fd\n");
		exit(-1);
	}
	if ((slave_device = ptsname(master_fd)) == NULL) {
		perror("Unable to get pts name");
		exit(-1);
	}
	if ((slave_fd = open(slave_device, O_RDWR|O_NOCTTY)) < 0) {
		perror("Unable to open slave device");
	}
	switch (fork()) {
		case -1:
			perror("Unable to fork!");
			exit(-1);
		case 0:
			close(master_fd);
			dup2(slave_fd, STDIN_FILENO);
			dup2(slave_fd, STDOUT_FILENO);
			dup2(slave_fd, STDERR_FILENO);
			if (ioctl(slave_fd, TIOCSWINSZ, &winsz) == -1) {
				perror("Unable to set window size");
			}
			if (isatty(1) != 1) {
				perror("It is not a terminal");
			}
			if (env) {
				execvpe(args[0], args, (char * const *)env);
			} else {
				execvp(args[0], (char * const *)args);
			}
			break;
		default:
			//freopen("file3", "a+", fdopen(master_fd, "rw"));
			close(slave_fd);
			listen_for_read_and_write(master_fd);
			break;
	}
	return 0;
}

void
set_env(char **env, char *value)
{
	int i=0;
	for (i=0; env[i]!=NULL; i++) {}
	env[i] = value;
	env[i+1] = NULL;
}

void
listen_for_read_and_write(int fd)
{
	FILE *fp = fopen("file", "w+");
	int n;
	char c[1];
	char buffer[MAX_BUF];
	int store_stdin = STDIN_FILENO;
	//signal(SIGINT, stop_signal_handler);
	while((n = read(fd, buffer, MAX_BUF)) > 0) {
		buffer[n] = '\0';
		printf("%s", buffer);
		//char c = fgetc(stdin);
		char *bu = "^C\n";
		//buffer[0] = c;
		write(fd, bu, strlen(bu));
		//dup2(STDOUT_FILENO, fd);
		fprintf(fp, "%s", buffer);
		fflush(stdout);
	}
	//close(STDOUT_FILENO);
	//dup(fp);
	write(fd, "ls", 2);
}

void
stop_signal_handler(int sig_num)
{
	
}

int
main(int argc, char **argv)
{
	int i=0;
	char buffer[MAX_BUF];
	char **env = (char **)malloc(sizeof(char **)*MAX_BUF);
	env[0] = NULL;
	char **args = (char **)malloc(sizeof(char **)*MAX_BUF);
	for (i=1; i<argc; i++) {
		args[i-1] = argv[i];
	}
	args[i] = NULL;
	set_env(env, "PWD=/");
	set_env(env, "HOME=/root");
	set_env(env, "TERM=xterm-256color");
	set_env(env, "USER=root");
	set_env(env, "SHELL=/bin/bash");
	run_pty_cmd(args, env);
	return 0;
}
