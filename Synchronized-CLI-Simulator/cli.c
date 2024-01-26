#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 1000

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_t listenerThreads[MAX_LINE_LENGTH];
pid_t childPids[MAX_LINE_LENGTH];

typedef struct {
    char command[MAX_LINE_LENGTH];
    char inputs[MAX_LINE_LENGTH];
    char options[MAX_LINE_LENGTH];
    char redirection[MAX_LINE_LENGTH];
    char redirected[MAX_LINE_LENGTH];
    char backgroundJob;
} CommandInfo;

typedef struct {
    int fd[2];
} ppipe;

void parseCommand(char *line, CommandInfo *commandInfo, FILE *outputFile) {
    // Initialize variables
    strcpy(commandInfo->inputs, "");
    strcpy(commandInfo->options, "");
    strcpy(commandInfo->redirection, "");
    strcpy(commandInfo->redirected, "");
    char rest[MAX_LINE_LENGTH];
    // Parse the line
    sscanf(line, "%s %[^\n]", commandInfo->command, rest);

    // Check for options and redirection
    bool redirect = false;
    char *token = strtok(rest, " ");
    while (token != NULL) {
        if (token[0] == '-' && strlen(token) > 1) {
            strcat(commandInfo->options, token);
        } 
        else if (token[0] == '<' || token[0] == '>') {
	    redirect = true;
            strcat(commandInfo->redirection, token);
        }
        else if (!redirect && token[0] != '&'){
            strcat(commandInfo->inputs, token);
        }
        else if (redirect && token[0] != '&'){
            strcat(commandInfo->redirected, token);
        }
        token = strtok(NULL, " ");
    }

    // Check for background job
    if (strchr(line, '&') != NULL) { 
        commandInfo->backgroundJob = 'y';
    } else {
        commandInfo->backgroundJob = 'n';
    }
    
	// Use the commandInfo struct as needed
	fprintf(outputFile, "---------\n");
	fprintf(outputFile, "Command: %s\n", commandInfo->command);
	fprintf(outputFile, "Inputs: %s\n", (strlen(commandInfo->inputs) > 0) ? commandInfo->inputs : " ");
	fprintf(outputFile, "Options: %s\n", (strlen(commandInfo->options) > 0) ? commandInfo->options : " ");
	fprintf(outputFile, "Redirection: %s\n", (strlen(commandInfo->redirection) > 0) ? commandInfo->redirection : "-");
	fprintf(outputFile, "Background Job: %c\n", commandInfo->backgroundJob);
	fprintf(outputFile, "---------\n");
	// Flush the output buffer to make sure the content is immediately visible
	fflush(outputFile);
}
void *mythread(void *arg) {
    pthread_mutex_lock(&lock);
    pthread_t tid = pthread_self();
    printf("---- %lu\n", (unsigned long)tid);
    
    int *fd = (int *)arg;
    close(fd[1]);
    
    // Open a FILE stream for reading from the read end of the pipe
    FILE *ReadStream = fdopen(fd[0], "r");
    if (ReadStream == NULL) {
        perror("fdopen failed");
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    char buffer[MAX_LINE_LENGTH];
    // Read from the pipe and print to the console
    while (fgets(buffer, MAX_LINE_LENGTH, ReadStream) != NULL) {
        printf("%s", buffer);
    }

    // Close the FILE stream
    fclose(ReadStream);
    
    printf("---- %lu\n", (unsigned long)tid);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    FILE *inputFile, *outputFile;
    char line[MAX_LINE_LENGTH];

    // Open files
    inputFile = fopen("commands.txt", "r");
    if (inputFile == NULL) {
        perror("Error opening commands.txt");
        return EXIT_FAILURE;
    }

    outputFile = fopen("parse.txt", "w");
    if (outputFile == NULL) {
        perror("Error opening parse.txt");
        fclose(inputFile);
        return EXIT_FAILURE;
    }
    
    
    CommandInfo commandArray[MAX_LINE_LENGTH];
    int i = 0; 
    // Read lines from commands.txt and parse
    while (fgets(line, MAX_LINE_LENGTH, inputFile) != NULL) {
        CommandInfo commandInfo;
        parseCommand(line, &commandInfo, outputFile);
        commandArray[i] = commandInfo;
        i++;
    }  
    
    int threadCount = 0;
    int childCount = 0;
    ppipe pipes[i];
     
    for(int j=0; j<i;j++){
    CommandInfo commandInfo = commandArray[j];
    
    if (strcmp(commandInfo.command, "wait") == 0) {
    	    // Wait for all child processes to complete (if any)
	    for (int k = 0; k < childCount; ++k) {
                waitpid(childPids[k], NULL, 0);
            }
            childCount = 0;
	    // Wait for all listener threads to complete (if any)
	    for (int a = 0; a < threadCount; ++a) {
	        pthread_join(listenerThreads[a], NULL);
	    }
	    threadCount = 0;
    }
    else{
	    
	    //int fd[2];
	    
	    if (commandInfo.redirection[0] != '>') {
	        if (pipe(pipes[j].fd) == -1) { //fd
	            perror("pipe failed");
	            return EXIT_FAILURE;
	        }
	    }
	    
	    pid_t pid = fork();
	    if (pid < 0){
	        perror("fork failed");
	        return EXIT_FAILURE;
	    }
	    else if (pid == 0){ //command-child process
	        bool noin = strcmp(commandInfo.inputs, "") == 0;
	        bool noopt = strcmp(commandInfo.options, "") == 0;
	        if(commandInfo.redirection[0] != '>'){
	            
	            close(pipes[j].fd[0]);
	            if(commandInfo.redirection[0] == '<'){
    	 	 	int input1 = open("input1.txt", O_RDONLY);
        		if (input1 == -1) {
            		    perror("open input1 fail");
            		    return 1;
        		}
        		dup2(input1, STDIN_FILENO);
        		close(input1);
        	    }
	            // Redirect standard output to the write end of the pipe
	            dup2(pipes[j].fd[1], STDOUT_FILENO);
	    		
	            close(pipes[j].fd[1]);
	                     
	        }
	        else{
		        // close(STDOUT_FILENO);
		        int out = open(commandInfo.redirected, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
		        if (out == -1) {
            		    perror("open out1 fail");
            		    return 1;
        		}
		        dup2(out, STDOUT_FILENO);
		}		        
			
		        if (commandInfo.backgroundJob == 'y'){
		            if(setpgid(0, 0) == -1){
		                perror("setpgid error");
		                exit(EXIT_FAILURE);
		            }
		        }
		        if (noin){
		            if (noopt){execlp(commandInfo.command,commandInfo.command, (char *)NULL);}
		            else{execlp(commandInfo.command,commandInfo.command, commandInfo.options, (char *) NULL);}
		        }
		        else{
		    	    if(noopt){execlp(commandInfo.command,commandInfo.command, commandInfo.inputs, (char *) NULL);}
		    	    else{execlp(commandInfo.command,commandInfo.command, commandInfo.inputs, commandInfo.options, (char *)NULL);}
		        }
		        perror("execlp failed");
		        exit(EXIT_FAILURE);

	    }
	    else{ 	//Shell-parent process
	        if (commandInfo.backgroundJob == 'n'){
	            waitpid(pid, NULL, 0);
	        }
	        
	        if (commandInfo.redirection[0] != '>'){
	            
	            pthread_t my_thread;
    		    
	            // Create and start a new thread
	            if (pthread_create(&my_thread, NULL, mythread, (void*)pipes[j].fd) != 0) {
	                fprintf(stderr, "Error creating thread\n");
	                return 1;
	            }
	            
	            listenerThreads[threadCount] = my_thread;
	            threadCount++;
	        }
	        
	        childPids[childCount] = pid;
	        childCount++;
	    }
	}
	}
    // Wait for all child processes to complete (if any)
    for (int k = 0; k < childCount; ++k) {
        waitpid(childPids[k], NULL, 0);
    }
    
    // Wait for all listener threads to complete (if any)
    for (int a = 0; a < threadCount; ++a) {
	pthread_join(listenerThreads[a], NULL);
    }
    
    // Close files
    fclose(inputFile);
    fclose(outputFile);

    return EXIT_SUCCESS;
}
