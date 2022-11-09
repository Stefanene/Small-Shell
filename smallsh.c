#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT_LENGTH 2048


void handleStop() {

}


void runShell(char** cmdStr, int *num) {
	int status;
	pid_t spawnpid = -5;
	int fileStuff;

	struct sigaction SIGINT_action = {0};
	struct sigaction SIGTSTP_action = {0};

	//code for dealing with CTRL+C command
	SIGINT_action.sa_handler = SIG_IGN;  //ignore
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);
	//code for dealing with CTRL+Z command
	SIGTSTP_action.sa_handler = handleStop; //custom function
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
  	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  	//exit command
	if (strcmp(cmdStr[0], "exit") == 0) {
		fflush(stdout);
		while (1){
			exit(0);
		}
	} 

	//change directory command
	else if (strcmp(cmdStr[0], "cd") == 0) {
		if (*num == 1) {  //no arguments means returning to home dir
			chdir(getenv("HOME"));
		} else {  //cd to inputed directory
			chdir(cmdStr[1]);
		}
	}

	//status command
	else if (strcmp(cmdStr[0], "status") == 0){
		if (WIFEXITED(status)){
			printf("exit value %d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)){
			printf("terminated by signal %d\n", WTERMSIG(status));
		}
	}

	//other commands
	else {
		
		spawnpid = fork(); //fork initial process

		switch (spawnpid) {
			case -1:  //something went wrong with forking
				perror("hull breach!");
				exit(1);
				break;
			case 0:  // in child process
				//process command line
				for (int i = 1; i < *num; i++) {

					//input file specified
					if(strcmp(cmdStr[i], ">") == 0) {
						fileStuff = open(cmdStr[i+1], O_RDONLY);
						if(dup2(fileStuff, STDIN_FILENO) == -1) {
							perror("cannot open() in");
							exit(1);
						}
					}

					//output file specified
					if(strcmp(cmdStr[i], "<") == 0) {
						fileStuff = open(cmdStr[i + 1], O_CREAT | O_RDWR | O_TRUNC, 0644);
						if(dup2(fileStuff, STDOUT_FILENO) == -1) {
							perror("cannot open() out");
							exit(1);
						}
					}

				}
				close(fileStuff);

				char* args[512];
				args[0] = cmdStr[0]; //prepare exec() arguments

				//finally execute command
				int status_code = execvp(cmdStr[0], args);
				if(status_code == -1) {
					printf("%s: no such file or directory\n", cmdStr[0]);
					exit(1);
				}

				break;
			default:  //in parent process

				//wait for child to terminate
				waitpid(spawnpid, &status, 0);

				break;
		}

	}
		
}


int askForCmd(char** cmdStr, int *inSize) {
	char inBuffer[MAX_INPUT_LENGTH];
	char* stuff;

	printf(": ");
	fgets(inBuffer, MAX_INPUT_LENGTH, stdin);
	strtok(inBuffer, "\n");  //get rid of newline from fgets

	if(inBuffer[0] == '#') return 0;  //is a comment

	//parse user input
	stuff = strtok(inBuffer, " ");	
	while(stuff != NULL) {
		cmdStr[*inSize] = strdup(stuff);
		(*inSize)++;
		stuff = strtok(NULL, " ");
	}

	return 1;
}


int main() {

	int inSize;
	char* cmdStr[512];

	int notComm = 1;

	while(1) {
		inSize = 0;
		memset(cmdStr, '\0', 512);
		fflush(stdout);
		fflush(stdin);

		//ask user for command
		notComm = askForCmd(cmdStr, &inSize);
		//execut command in shell
		if(notComm) runShell(cmdStr, &inSize);
		else printf("comments are not executed\n");
	}

	return 0;

}
