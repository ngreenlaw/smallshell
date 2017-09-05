/*Nathan Greenlaw
 ONID: greenlan
 CS 344
 Program 3: Small shell in C*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
// Start shell
// display prompt with :
// get input
// process input
// if command is exit, cd or status send them to function to handle that
// if command begins with a comment(#) or is blank then ignore the line and display prompt again
// else fork(), wait(waitpid() for foreground process and waitpid(nohang) for background process), handle redirection, exec() and then go back to main program
// within programs have to error check
// signals will be ctrl-c and ctrl-z of SIGINT and SIGTSTP
// exit: exits the shell and takes no arguments. Must kill any other processes or jobs started before terminating
// cd: changes directory. No Argument means it changes to the HOME environment variable it can also take one argument which is the path of the directory to change to
// status: prints out the exit status or the terminating signal of the last foreground process

int fgMode = 0; // 0 means foreground mode is off and 1 means foreground mode is on
int currentFgProcess = -5; // the pid of the current fg process
int shellPid = -5; // process id of the shell
int exitValue = 0; // exit value for status to print	
int signalValue = -5; // signal value for status to print

int bgTime = 0; // check if & was on end

int input = 0; //check if < was entered
int output = 0; // check if > was entered
char* inputFile; // store input file name
char* outputFile; // store output file name

void catchSIGINT(int signo)
{
	if( (currentFgProcess != shellPid))
		{
			char* message = "Terminated by signal 2\n";
			write(STDOUT_FILENO, message,24);
			signalValue = 2;
			exitValue = -5;
			kill(currentFgProcess,SIGTERM); //this kills thecurrent process without killing shell
		}
	else
	{
		char* nL = "\n";
		write(STDOUT_FILENO, nL,2);
	}
}

void catchSIGSTP(int signo)
{
	char* f1 = "Foreground only mode on (& is ignored)";
	char* f2 = "Foreground only mode off";
	char* nL = "\n";
	write(STDOUT_FILENO, nL,2);
	
	if (fgMode == 0)
	{
		fgMode = 1;
		write(STDOUT_FILENO, f1,38);
	}
	else
	{
		fgMode = 0;
		write(STDOUT_FILENO, f2,24);
	}
	write(STDOUT_FILENO, nL,2);

}

// main function
int main()
{
	//initialize needed variables
	int finish = 0; //exits the loop when exit is called
	int numCharsEntered = -5; // how many characters where entered
	int currChar = -5; // tracks where we are when we print each character
	size_t bufferSize = 2048; // holds how large the allocated buffer is
	char* lineEntered;
	lineEntered = calloc(2048,sizeof(char)); // Points to a buffer allocated by getline() that holds our entered string + \n and \0
	int lineLength = 0; // length of line entered
	int *bgProcessId = NULL; //array of bgprocess ids
	bgProcessId = calloc(2048, sizeof(int)); //allocate memory
	int bgNumber =0; //number of background processes run	

	inputFile = calloc(2048,sizeof(char));
	outputFile = calloc(2048,sizeof(char));


	currentFgProcess = getpid(); // holds the currently running foreground process to kill off if ctrl c is given
	shellPid = getpid(); // holds the pid of the shell

	// get starting directory and set HOME to it
	char cwd[2048];
	getcwd(cwd,sizeof(cwd));
	setenv("HOME",cwd,1);

	// Signals set up
	struct sigaction SIGINT_action = {0}, SIGSTP_action = {0}, ignore_action = {0};

	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;

	SIGSTP_action.sa_handler = catchSIGSTP;
	sigfillset(&SIGSTP_action.sa_mask);
	SIGSTP_action.sa_flags = 0;

	sigaction(SIGINT, &SIGINT_action,NULL); 
	sigaction(SIGTSTP, &SIGSTP_action,NULL);
	
	// Loop until exit
	while(finish == 0)
	{
		//Check the background processes to see if they have completed
		int i=0;
		for( i=0;i<bgNumber;i++ )
		{
			pid_t checkPid = -5;
			int cExitStat = -5;
			int eVal = -5;
			int sVal = -5;
			
			// Check that the process has not been completed
			if( !(bgProcessId[i] == -10))
			{
				checkPid = waitpid(bgProcessId[i],&cExitStat, WNOHANG);
				if( checkPid == -1)
				{
					perror("Wait Failed");
					exit(1);
				}
			
				//If the process is finished then display exit value
				if( !(checkPid == 0))
				{					
					if(WIFEXITED(cExitStat))
					{
						eVal = WEXITSTATUS(cExitStat);
						printf("background process %d is done: exit value %d\n",bgProcessId[i], eVal); fflush(stdout);
					}
		
					else
					{
						if(WIFSIGNALED(cExitStat))
						{
							sVal = WTERMSIG(cExitStat);
							printf("background process %d is done: terminated by signal %d\n", bgProcessId[i], sVal); fflush(stdout);
						}
					}
					bgProcessId[i] = -10; // process has been completed
				}
			}
		}

		while(1) //continue until input is given
		{
			// display prompt
			printf(": "); fflush(stdout);
		
			// get line from user
			numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
			if(numCharsEntered == -1)
			{
				clearerr(stdin);
			}
			else
			{
				break;
			}
		}
		// process input
		lineEntered[strcspn(lineEntered, "\n")] = '\0';
	
		// convert any $$ to process id of shell	
		char replace[2048];// new line
		char process[2048];// get processid
		char* sign; //pointer to location in string
		if(sign = strstr(lineEntered,"$$"))
		{
			sprintf(process,"%d",getpid()); fflush(stdout);//make pid a string
			strncpy(replace, lineEntered, sign-lineEntered);//copy the characters except for $$
			replace[sign-lineEntered] = '\0';//add null terminating character, assist from internet
			sprintf(replace+(sign-lineEntered), "%s%s",process,sign+strlen("$$")); //reprint the string with the process id
			strcpy(lineEntered,replace); //replace line entered with the new line
			sign = NULL;
		}
		//printf("line: %s\n", lineEntered);

		//get the length of the line entered
		lineLength = strlen(lineEntered);
		lineLength--;
	
		// Use strtok to separate the input into arguments and ignore the & if foreground mode is active
		char *args[512];
		char **next = args;
		char *temp = strtok(lineEntered, " ");
		int numArgs = 0; // number of things input

		//if ctrl z has been given
		if(fgMode == 1)
		{
			//reset all the variables for checking if input and output
			output = 0;
			input = 0;
			bgTime = 0;
			strcpy(inputFile, "");
			strcpy(outputFile, "");
		
			//loop until there are no more arguments
			while (temp != NULL)
			{
				//if there is & then move on to next arg
				if( (strcmp(temp, "&") == 0))
				{
					temp = strtok(NULL, " ");
				}
				else
				{
					//if there is a < set the input file and then store the input file in args and move on
					if( strcmp(temp, "<") == 0)
					{
						input = 1; // indicate that there was an input given
						temp = strtok(NULL, " ");
						*next++ = temp;
						strcpy(inputFile, temp);
						numArgs++;
						temp = strtok(NULL, " ");
					}
					else
					{
						// if there is a > set the output file and then move past the < and output file without adding to args
						if(strcmp(temp, ">") == 0)
						{
							output = 1; //indicate there is an output given
							temp = strtok(NULL, " ");
							strcpy(outputFile, temp);
							temp = strtok(NULL, " ");
							
						}
						// otherwise add the command
						else
						{
							*next++ = temp;
							numArgs++;
							temp = strtok(NULL, " ");
						}
					}
				}
			}
			*next = NULL;

		}

		// if foreground mode isnt on
		// loop is same as above except set the variable to indicate process should run in background
		else
		{
			output = 0;
			input = 0;
			bgTime = 0;
			strcpy(inputFile, "");
			strcpy(outputFile, "");
			while (temp != NULL)
			{
				if( (strcmp(temp, "&") == 0))
				{
					bgTime = 1;
					temp = strtok(NULL, " ");
				}
				else
				{
					if( strcmp(temp, "<") == 0)
					{
						input = 1;
						temp = strtok(NULL, " ");
						*next++ = temp;
						strcpy(inputFile, temp);
						numArgs++;
						temp = strtok(NULL, " ");
					}
					else
					{
						if(strcmp(temp, ">") == 0)
						{
							output = 1;
							temp = strtok(NULL, " ");
							strcpy(outputFile, temp);
							temp = strtok(NULL, " ");
							
						}
						else
						{
							*next++ = temp;
							numArgs++;
							temp = strtok(NULL, " ");
						}
					}
				}
			}
			*next = NULL;

		}
	
		// if ther first line is not a comment then check everything else
		if(!(strncmp(lineEntered, "#",1) == 0 || lineLength == -1))
		{
			// if the exit command was entered
			if(strcmp(lineEntered, "exit") == 0)
			{
				//kill all currently running processes
				int j=0;
				for(j = 0; j< bgNumber; j++)
				{
					// -10 means the process is complete. Value assigned when process is completed
					if( !(bgProcessId[j] == -10))
					{
						kill(bgProcessId[j], SIGTERM);
					}
				}
				
				//free allocated memory for all things
				free(lineEntered);
				lineEntered = NULL;
				free(bgProcessId);
				bgProcessId = NULL;
				free(inputFile);
				inputFile = NULL;
				free(outputFile);
				outputFile = NULL;

				//kill(0,SIGTERM); // remove when kill is fixed		

				// terminate itself
				finish = 1;
				exit(0);
			}
			else
			{
				// if the cd command was entered
				if(strcmp(args[0],"cd") == 0)
				{
					// move to the path pointed to by the HOME variable
					if(numArgs == 1)//strcmp(args[1],".") == 0)
					{
						chdir(getenv("HOME"));
						printf("moved to %s\n", getcwd(cwd,sizeof(cwd))); fflush(stdout);
						exitValue = 0;
					}	
					else
					{
						// attempt to move to directory and print that you cannot otherwise
						if(chdir(args[1]) == 0)
						{
							printf("moved to %s\n", getcwd(cwd,sizeof(cwd))); fflush(stdout);
							exitValue = 0;
						}
						else
						{
							printf("Cannot move to directory\n"); fflush(stdout);
							exitValue = 1;
						}
					}
				}
				else
				{
					// if the status command was entered
					if(strcmp(lineEntered, "status") == 0 || strcmp(lineEntered, "status &") == 0)
					{
						// if there was an exit value given otherwise print the signal value
						if(exitValue != -5)
						{
							printf("Exit value %d\n",exitValue); fflush(stdout);
						}
						else
						{
							if(signalValue != -5)
							{
								printf("Signal Terminated %d\n",signalValue); fflush(stdout);
							}
						}						
						exitValue = 0;
					}
					
					else
					{
						//create the fork to do other command
						pid_t spawnPid = -5;
						int childExitStatus = -5;
						
						spawnPid = fork();
						if(spawnPid != -1) { currentFgProcess = spawnPid;}
						switch(spawnPid)
						{
							// error in spawn. Too many processes
							case -1: {perror("Hull Breach\n");exit(1);break;}
							
							// child commands
							case 0:
							{
								//handle redirection here
								int k = 0;
								int sourceFD;
								int targetFD;
								int result;
								int outputDone = 0;
								int inputDone = 0;								

								for(k =0; k < (numArgs) ; k++)
								{
									//if there is supposed to be an output file then open it and redirect the stdout to it
									if(output == 1)
									{
										output = 0;
										outputDone = 1;
										targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0755);
										if(targetFD == -1) {perror(outputFile); exit(1);}
										result = dup2(targetFD, 1);
										if(result == -1) {perror("dup2"); exit(1);}
										
										fcntl(targetFD, F_SETFD, FD_CLOEXEC); // close the file on exec
									}
									else
									{
										// if no output was given and its supposed to run in the background then open dev/null and send stdout to it
										if(output == 0 && bgTime == 1 && outputDone == 0)
										{
											output = 0;
											outputDone = 1;
											targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0755);
											if(targetFD == -1) {perror(outputFile); exit(1);}
											result = dup2(targetFD, 1);
											if(result == -1) {perror("dup2"); exit(1);}

											fcntl(targetFD, F_SETFD, FD_CLOEXEC);
										}
									}

									//if there is supposed to be an input file then open it and redirect the stdin to it
									if(input == 1)
									{
										input = 0;
										inputDone = 1;
										sourceFD = open(inputFile, O_RDONLY);
										if(sourceFD == -1) {perror(inputFile); exit(1);}
										result = dup2(sourceFD,0);
										if(result == -1) {perror("source dup2()"); exit(1);}
										fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
										
									}
									else
									{
										// if no input was given and process is supposed to be in the background then pull from dev/null
										if(input == 0 && bgTime == 1 && inputDone == 0)
										{
											input = 0;
											inputDone = 1;
											sourceFD = open("/dev/null", O_RDONLY);
											if(sourceFD == -1) {perror(inputFile); exit(1);}
											result = dup2(sourceFD,0);
											if(result == -1) {perror("source dup2()"); exit(1);}
											fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
										}
									}
								}
								//execute the function
								execvp(args[0],args);
								perror("exec failure\n");
								exit(1); 
								break;
							}
							
							// parent commands
							default:
							{
								pid_t actualPid;
								
								// if the & is the last argument and fgMode is not on then allow for background process
								if(bgTime == 1 && fgMode == 0) 
								{
									printf("background process id is %d\n",spawnPid); fflush(stdout);
									bgProcessId[bgNumber] = spawnPid;
									bgNumber++;
									bgTime = 0;
								}
								else
								{
										//wait for the child process to complete
										actualPid = waitpid(spawnPid, &childExitStatus, 0);
										currentFgProcess = shellPid;	
																		
										if(WIFEXITED(childExitStatus))
										{
											exitValue = WEXITSTATUS(childExitStatus);
											signalValue = -5; // signal did not terminate for status check
										}
		
										else
										{
											if(WIFSIGNALED(childExitStatus) )
											{
												exitValue = -5; // signal terminated so exit does not display on status check
											}
										}
									
								}
								break;
							}
						}						
					}
				}
			}
		}
		//free allocated memory for lineEntered
		free(lineEntered);
		lineEntered = NULL;		
	}
	return 0;
}
