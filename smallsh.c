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

int foregroundOnly;

//custom function for CTRL+Z
void handleStop() {
	if (foregroundOnly) {  //in foreground only mode
		foregroundOnly = 0;  //change modes
		printf("\nExiting foreground-only mode\n" );
	} else {  //in background mode
		foregroundOnly = 1;  //change modes
		printf("\nEntering foreground-only mode (& is now ignored)\n");
	}
	fflush(stdout);
}


void runShell(char** cmdStr, int *num) {
	int status;
	pid_t spawnpid = -5;
	int fileStuff;
	int hasFile = 0;
	int backToggle = 0;

	struct sigaction SIGINT_action = {0};
	struct sigaction SIGTSTP_action = {0};

	//code for dealing with CTRL+C command
	SIGINT_action.sa_handler = SIG_IGN;  //ignore
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	//code for dealing with CTRL+Z command
	SIGTSTP_action.sa_handler = handleStop; //custom function above
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
  	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  	//check for background mode symbol
  	if(strcmp(cmdStr[*num-1], "&") == 0) {
  		//delete last argument index of command
  		(*num)--;
  		cmdStr[*num] = NULL;
  		if(!foregroundOnly) backToggle = 1;  //enter bacgound mode if foregroundOnly is off
  	}

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
		fflush(stdout);
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

					//if in background mode without a specified file, use /dev/null 
					if(backToggle) {
						hasFile = 1;
						fileStuff = open("/dev/null", O_RDONLY);
						if(dup2(fileStuff, STDIN_FILENO) == -1) {
							perror("source open()");
							exit(1);
						}
					}

					//input file specified
					if(strcmp(cmdStr[i], "<") == 0) {
						hasFile = 1;
						fileStuff = open(cmdStr[i+1], O_RDONLY);
						if(dup2(fileStuff, STDIN_FILENO) == -1) {
							perror("cannot open() in");
							exit(1);
						}
					}

					//output file specified
					if(strcmp(cmdStr[i], ">") == 0) {
						hasFile = 1;
						fileStuff = open(cmdStr[i + 1], O_CREAT | O_RDWR | O_TRUNC, 0644);
						if(dup2(fileStuff, STDOUT_FILENO) == -1) {
							perror("cannot open() out");
							exit(1);
						}
					}

				}
			
				//truncate command line if has file
				if(hasFile) {
					close(fileStuff);  //close file
					for (int i = 1; i < *num; i++) {
						cmdStr[i] = NULL;  //truncate
					}
				}

				//finally execute command
				int status_code = execvp(cmdStr[0], cmdStr);
				if(status_code == -1) {
					if(strcmp(cmdStr[0], "\n") != 0) {  //ignore empty lines
						printf("%s: no such file or directory\n", cmdStr[0]);
						fflush(stdout);
					}
					exit(1);
				}

				break;
			default:  //in parent process
				
				//print pid if a background process
				if(backToggle) {
					printf("background process pid is %d\n", spawnpid);
					fflush(stdout);
				}
				//otherwise, in foreground
				else {
					//wait for child to terminate
					waitpid(spawnpid, &status, 0);
					//deal with backgound process completion
					if(WIFSIGNALED(status)) { //check if child is dead
						printf("terminated by signal %d\n", WTERMSIG(status));
						fflush(stdout);
					}
					while (spawnpid != -1) { //keep fetching pid until process is killed
						spawnpid = waitpid(-1, &status, WNOHANG);	
						//print after results killed
						if (WIFEXITED(status) != 0 && spawnpid > 0){
							printf("background pid %d is done: exit value %d\n", spawnpid, WEXITSTATUS(status));
							fflush(stdout);
						} else if (WIFSIGNALED(status) != 0 && spawnpid > 0 && backToggle == 0){
							printf("background pid %d is done: terminated by signal %d\n", spawnpid, WTERMSIG(status));
							fflush(stdout);
						}
					}
				}
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

	fflush(stdin);

	if(inBuffer[0] == '#') return 0;  //is a comment

	//deal with $$ -- source code for this method referenced from Greg Noetzel on GitHub
	for (int i = 0; i < strlen(inBuffer); i ++) {
		// swapping $$ for %d
		if ((inBuffer[i] == '$')  && (inBuffer[i + 1] == '$')) {
			char * temp = strdup(inBuffer);
			temp[i] = '%';
			temp[i + 1] = 'd';
			sprintf(inBuffer, temp, getpid());  //sprintf the pid in the buffer string
			free(temp);
		}
	}

	//parse user input and update size of input command
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
	foregroundOnly = 0;  //mode set to not foreground-only by default

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
	}

	return 0;

}
