/* $begin shellmain */
#include "csapp.h"
#include <errno.h>

#define MAXARGS 128
#define MAX_HISTORY_COUNT 15 /* The maximum number of commands stored in history */

char history[MAX_HISTORY_COUNT][MAXARGS]; /* Array to store history commands */
int history_count = 0; /* Number of commands currently stored in history */
sigset_t mask_one, prev_one, mask_all;
int fg_end_flag = 0; // foreground job 종료 check
int pipe_end_flag = 0; // pipe -> command 종료 check;
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

int pipe_execution(char **p_command, int p_command_cnt); // execution command line which has pipe


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


void sig_child_in_pipe_handler(int sig)
{
	int olderrno=errno;
	sigset_t mask_all, prev_all;
	pid_t pid;
	int status;
	Sigfillset(&mask_all);
	
	// WNOHANG, WUNTRACED를 지정해, child process가 종료되지 않았을 때도 return, 멈춘 프로세스에 대해서도 상태정보 얻을 수 있음
	// while문은 종료된 child process의 pid를 return, 반환값이 0인 경우는 아직 종료되지 않은 경우 -> while문 빠져나감
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) // child process가 종료될때까지 대기하는 system call
	{
		Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // block
		// child process가 정상적으로 종료되거나 signal에 의해 종료되는 경우
		if(WIFEXITED(status) || WIFSIGNALED(status))
		{
			pipe_end_flag=1; 
		}
		Sigprocmask(SIG_SETMASK, &prev_all, NULL); // unblock
	}
	errno=olderrno; // 이전의 errno값 복원
}

// This function performs execution of commands using pipes
int pipe_execution(char **p_command, int p_command_cnt)
{
	int i;
	char *argv[MAXARGS]; // An array of pointers to the arguments
	char buf[MAXLINE]; // Buffer to store command line input
	int pipefd[MAXARGS][2]; // 2D array to hold file descriptors of pipes
	pid_t pid;
	int status;
	sigset_t prev_all;

	Sigemptyset(&prev_all); // Initialize a signal set and empty it
	Signal(SIGCHLD, sig_child_in_pipe_handler); // Register a signal handler for child process termination
	Signal(SIGTSTP, SIG_DFL); // Set the default signal handling for SIGTSTP in the child process
	Signal(SIGINT, SIG_DFL); // Set the default signal handling for SIGINT in the child process

	// For each command separated by pipes
	for(i=0; i<p_command_cnt; i++)
	{
		char argv0[MAXLINE]="/bin/"; // Initialize argv[0] with "/bin/"
		int temp;
		strcpy(buf, p_command[i]); // Copy the current command to the buffer
		temp=parseline(buf, argv); // Parse the command line input and store arguments in the array argv[]

		strcat(argv0, argv[0]); // Concatenate "/bin/" with the first argument in argv[]

		pipe(pipefd[i]); // Create a pipe

		
		if((pid=Fork())==0)
		{
			// If it is the first command
			if (i == 0) {
				close(pipefd[i][0]); // Close the read end of the pipe
				dup2(pipefd[i][1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
				close(pipefd[i][1]); // Close the write end of the pipe
			}
			// If it is the last command
			else if (i == p_command_cnt - 1) {
				close(pipefd[i - 1][1]); // Close the write end of the previous pipe
				dup2(pipefd[i - 1][0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
				close(pipefd[i - 1][0]); // Close the read end of the previous pipe
			}
			// For all other commands
			else {
				dup2(pipefd[i - 1][0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
				dup2(pipefd[i][1], STDOUT_FILENO); // Redirect stdout to the write end of the current pipe
				close(pipefd[i - 1][0]); // Close the read end of the previous pipe
				close(pipefd[i][1]); // Close the write end of the current pipe
			}

			// Try to execute the command in the child process
			if (execve(argv0, argv, environ) < 0) {
				strcpy(argv0, "/usr/bin/"); // Try executing the command in /usr/bin/
				strcat(argv0, argv[0]);
				if (execve(argv0, argv, environ) < 0) { // If the command is still not found
					printf("%s: Command not found.\n", argv[0]); // Print an error message
					exit(0); // Exit the child process
				}
			}
		} 
		close(pipefd[i][1]); // Close the write end of the pipe
		if (i > 0) {
			close(pipefd[i - 1][0]); // Close the read end of the previous pipe
		}

		// Wait for the signal handler to receive
		while (!pipe_end_flag) {
			Sigsuspend(&prev_all);
		}
		pipe_end_flag = 0;
	}
	exit(0);
}

void ncmdline(char * cmdline)
{
	int i;
	int length = strlen(cmdline);
	char buf[MAXLINE];
	int buf_idx=0;
	cmdline[strlen(cmdline) - 1] = ' '; /* Replace trailing '\n' with space */
	for (i = 0; i < length; i++)
	{
		if (cmdline[i] == '\t')
			cmdline[i] = ' '; //tab -> space로 바꾸어줌
	}

	for(i=0; i<length; i++)
	{
		if(cmdline[i]=='|' || cmdline[i]=='&')  // pipe'|'나 bg'&' -> space
		{
			buf[buf_idx++]=' ';
			buf[buf_idx++]=cmdline[i];
			buf[buf_idx++]=' ';
		}
		else
		{
			buf[buf_idx++]=cmdline[i];
		}
		buf[buf_idx]='\0';
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

void eval(char *cmdline)
{
	char *argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE];   /* Holds modified command line(for argv) */
	char buf2[MAXLINE];  /* Holds modified command line (for piped command)*/
	int bg;              /* Should the job run in bg or fg? */

	int pipe_flag=0; // Stores whether there is a pipe or not
	int i;

	ncmdline(cmdline); 

	strcpy(buf, cmdline); // Copy cmdline to buf
	bg = parseline(buf, argv); // Parse the command using parseline function
	// parseline function returns whether the job should run in background or foreground

	if (argv[0] == NULL)
		return; /* Ignore empty lines */

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
	{ //quit -> exit(0), & -> ignore, other -> run

		char argv0[MAXLINE] = "/bin/";
		strcat(argv0, argv[0]); // Create "/bin/ls" format.
		char *p_command[MAXARGS]; // Stores each command after splitting using pipe(|)
		int p_command_cnt=0; // Stores the number of commands after splitting using pipe(|)
		int p_start=0; // Stores the starting point of each command after splitting using pipe(|)
		strcpy(buf2, cmdline); // Store cmdline in temporary variable buf2
		int i;
		Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

		if((pid=Fork())==0)
		{
			setpgid(0, 0);
			// Check for pipe;
			for(i=0; i<strlen(cmdline); i++) // Using strlen(buf2) here can lead to changes in the middle of strlen
			{
				if(buf2[i]=='|') // If pipe(|) exists
				{
					pipe_flag=1;
					p_command[p_command_cnt++]= &buf2[p_start];
					buf2[i]='\0';
					p_start=i+1;
				}
			}
			if(pipe_flag==1) //pipe가 있는 경우
			{
				//마지막 pipe 뒤쪽의 command도 p_command에 추가해줌.
				p_command[p_command_cnt++]= &buf2[p_start];
				pipe_execution(p_command, p_command_cnt);				
			}
			else//pipe가 없는 경우
			{
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
		}
		/* Parent waits for foreground job to terminate */
		else
		{
			setpgid(pid, 0);
			if (!bg)
			{
				tcsetpgrp(STDERR_FILENO, pid);
				int status;
				Sigprocmask(SIG_BLOCK, &mask_all, NULL);
				while(!fg_end_flag)//foreground job을 명시적으로 기다리기 위해서
					Sigsuspend(&prev_one);
				fg_end_flag=0;
				Sigprocmask(SIG_SETMASK, &prev_one, NULL);//unblock sigchild
				tcsetpgrp(STDERR_FILENO, getpgrp());
			}
			else //when there is backgrount process!
				printf("%d %s", pid, cmdline);
		}
	}
	return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char** argv)
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
int parseline(char *buf, char **argv)
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
