/* $begin shellmain */
#include "csapp.h"
#include <errno.h>

#define MAXARGS 128
#define MAX_HISTORY_COUNT 15 /* The maximum number of commands stored in history */

char history[MAX_HISTORY_COUNT][MAXARGS]; /* Array to store history commands */
int history_count = 0; /* Number of commands currently stored in history */
sigset_t mask_one, prev_one, mask_all;
int fg_end_flag = 0; // foreground job 종료 check
pid_t pid;           /* Process id */

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void add_to_history(char* cmdline);
void print_history();
void ncmdline(char* cmdline);
void init_sig();
void sig_int_handler(int sig);
void sig_stop_handler(int sig);
void sig_child_handler(int sig);


void init_sig() {
	// set mask
	Sigfillset(&mask_all);
	Sigemptyset(&mask_one);
	Sigaddset(&mask_one, SIGCHLD);

	// signal handler
	Signal(SIGCHLD, sig_child_handler);
	Signal(SIGINT, sig_int_handler);
	Signal(SIGTSTP, sig_stop_handler);

	// SIGTTOU, SIGTIN
	Signal(SIGTTOU, SIG_IGN);
	Signal(SIGTTIN, SIG_IGN);
}

void sig_int_handler(int sig)
{
	if (pid != 0)
	{
		kill(-pid, SIGINT);//foreground -> sigint
	}
	return;
}
void sig_stop_handler(int sig)
{
	if (pid != 0)
	{
		kill(-pid, SIGINT);//foreground -> sigint
	}
	return;
}

void sig_child_handler(int sig) {
	int olderrno = errno; // current errno
	sigset_t mask_all, prev_all;
	pid_t pid;
	int status;
	Sigfillset(&mask_all);
	// waitpid 이용해 종료된 child process 확인
	// 'WNOHANG' child process가 종료되지 않았을 경우 return
	// 'WUNTRACED' 중단된 process도 받아옴
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) { 

		Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

		if (WIFSTOPPED(status)) { // 해당 child porcess가 중단
			printf("Job [%d] stopped by signal %d\n", pid, WSTOPSIG(status)); // 중단된 signal num printf
			fg_end_flag = 1; //parent process가 처리할 수 있도록
		}
		else if (WIFSIGNALED(status)) { // 해당 child process가 signal에 의해 중단
			printf("Job [%d] terminated by signal %d\n", pid, WTERMSIG(status));
			fg_end_flag = 1; 
		}
		else if (WIFEXITED(status)) { // 해당 child process 정상적 종료
			fg_end_flag = 1; 
		}

		Sigprocmask(SIG_SETMASK, &prev_all, NULL);
	}
	errno = olderrno; // errno 다시 설정
}

void ncmdline(char* cmdline)
{
	int i;
	int length = strlen(cmdline);
	char buf[MAXLINE];
	int buf_idx = 0;
	cmdline[strlen(cmdline) - 1] = ' '; /* Replace trailing '\n' with space */
	for (i = 0; i < length; i++)
	{
		if (cmdline[i] == '\t')
			cmdline[i] = ' '; //tab -> space로 바꾸어줌
	}

	for (i = 0; i < length; i++)
	{
		if (cmdline[i] == '|' || cmdline[i] == '&') // pipe'|'나 bg'&' -> space 
		{
			buf[buf_idx++] = ' ';
			buf[buf_idx++] = cmdline[i];
			buf[buf_idx++] = ' ';
		}
		else
		{
			buf[buf_idx++] = cmdline[i];
		}
		buf[buf_idx] = '\0';
	}
	strcpy(cmdline, buf);
}

void add_to_history(char* cmdline) {
	// Make a copy of cmdline
	char* cmdline_copy = strdup(cmdline);

	// Check if history log already contains the command
	if (history_count > 0 && strcmp(history[history_count - 1], cmdline_copy) == 0) {
		free(cmdline_copy); // Free the memory used by cmdline_copy
		return;
	}

	// Add the command to history log
	if (history_count >= MAX_HISTORY_COUNT) {
		for (int i = 0; i < MAX_HISTORY_COUNT - 1; i++) {
			strcpy(history[i], history[i + 1]);
		}
		history_count--;
	}
	strcpy(history[history_count], cmdline_copy);
	history_count++;

	// Free the memory used by cmdline_copy
	free(cmdline_copy);
}

void print_history() {
	for (int i = 0; i < history_count; i++) {
		printf("%d   %s\n", i + 1, history[i]);
	}
}

/* eval - Evaluate a command line */
void eval(char *cmdline)
{
	char* argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE];   /* Holds modified command line */
	int bg;              /* Should the job run in bg or fg? */

	ncmdline(cmdline);

	strcpy(buf, cmdline); // Copy cmdline to buf
	bg = parseline(buf, argv); // Parse the command using parseline function
	// parseline function returns whether the job should run in background or foreground

	if (argv[0] == NULL) 
		return;   /* Ignore empty lines */
	
	// check for builtin commands
	if (strcmp(argv[0], "history") == 0) {
		// Add command to history log
		add_to_history(cmdline);
		print_history();
		return;
	}
	else if (argv[0][0] == '!') {
		int command_num;
		if (argv[0][1] == '!') {
			if (history_count == 0) {
				printf("No commands in history\n");
				return;
			}
			// Execute the most recent command in history
			command_num = history_count;
		}
		else {
			command_num = atoi(argv[0] + 1);
		}
		if (command_num < 1 || command_num > history_count) {
			printf("No such command in history\n");
			return;
		}
		// Execute the specified command in history
		strcpy(buf, history[command_num - 1]);
		// Add command to history log
		printf("%s\n", history[command_num - 1]);

		// Parse the command line again
		bg = parseline(buf, argv);
		if (argv[0] == NULL)
			return;
		add_to_history(history[command_num - 1]);
	}
	else {
		// Add command to history log
		add_to_history(cmdline);
	}

	// Pass the parsed command to the function
	if (!builtin_command(argv)) 
	{ 
		char argv0[MAXARGS] = "/bin/";
		strcat(argv0, argv[0]); // "/bin/command"
		Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

		if ((pid = Fork()) == 0) // child process
		{
			setpgid(0, 0);
			if (execve(argv0, argv, environ) < 0)
			{
				// user/bin
				strcpy(argv0, "/usr/bin/");
				strcat(argv0, argv[0]);
				if (execve(argv0, argv, environ) < 0)
				{
					printf("%s: Command not found.\n", argv[0]);
					exit(0);
				}
			}
		}

		/* Parent waits for foreground job to terminate */
		if (!bg) // child가 foregorund process인 경우
		{
			setpgid(pid, 0);
			tcsetpgrp(STDERR_FILENO, pid);
			int status;
			Sigprocmask(SIG_BLOCK, &mask_all, NULL);
			while (!fg_end_flag) // foreground job을 명시적으로 기다리기 위해서
				Sigsuspend(&prev_one);
			fg_end_flag = 0;
			Sigprocmask(SIG_SETMASK, &prev_one, NULL); // unblock sigchild
			tcsetpgrp(STDERR_FILENO, getpgrp());
		}
		else //when there is backgrount process!
			printf("%d %s", pid, cmdline);
	}
	return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
	if (!strcmp(argv[0], "exit")) /* exit command */
		exit(0);
	if (!strcmp(argv[0], "quit")) /* quit command */
		exit(0);
	if (!strcmp(argv[0], "&")) /* Ignore singleton & */
		return 1;
	if (!strcmp(argv[0], "cd")) // command cd
	{
		if (argv[1] == NULL || !strcmp(argv[1], "~")) {
			// Change to home directory
			chdir(getenv("HOME"));
		}
		else if (chdir(argv[1]) < 0) {
			// If chdir fails, print an error message
			fprintf(stderr, "cd: %s: No such file or directory\n", argv[1]);
		}
		return 1;
	}
	return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
int parseline(char* buf, char** argv)
{
	char* delim;     /* Points to first space delimiter */
	int argc;        /* Number of args */
	int bg;          /* Background job? */
	
	buf[strlen(buf) - 1] = ' ';  /* Replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;

	/* Build the argv list */
	argc = 0;
	while ((delim = strchr(buf, ' '))) {
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* Ignore spaces */
			buf++;
	}
	argv[argc] = NULL;

	if (argc == 0) /* Ignore blank line */
		return 1;

	/* Should the job run in the background? */
	if ((bg = (*argv[argc - 1] == '&')) != 0)
		argv[--argc] = NULL;

	return bg;
}
/* $end parseline */

