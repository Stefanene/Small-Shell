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
		
}


void askForCmd(char** cmdStr, int *inSize) {
	char inBuffer[MAX_INPUT_LENGTH];
	char* stuff;

	printf(": ");
	fgets(inBuffer, MAX_INPUT_LENGTH, stdin);
	strtok(inBuffer, "\n");  //get rid of newline from fgets

	//parse user input
	stuff = strtok(inBuffer, " ");	
	while(stuff != NULL) {
		cmdStr[*inSize] = strdup(stuff);
		(*inSize)++;
		stuff = strtok(NULL, " ");
	}

}


int main() {

	int inSize;
	char* cmdStr[512];

	while(1) {
		inSize = 0;
		memset(cmdStr, '\0', 512);
		fflush(stdout);
		fflush(stdin);

		//ask user for command
		askForCmd(cmdStr, &inSize);
		//execut command in shell
		runShell(cmdStr, &inSize);
	}

	return 0;

}
