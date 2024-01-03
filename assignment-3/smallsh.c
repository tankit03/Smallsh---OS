#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

// Define global variables
int foreground = 1;
int last_info;


// structure to represent a command
struct command {

    char* command;
    char** argument;
    int background;
    int count; 

};

// structure to represent background processes
struct backgroundPro{
    pid_t pid;
    int status;
};


// Declare an array to store background processes and a counter for the number of background processes
struct backgroundPro backgroundPros[512];
int countBackgroundPro = 0;

void user_prompt();

// Function to check the status of background processes
void checkBackgroundPro(){

    int i = 0;

    // Iterate through all background processes
    while (i < countBackgroundPro) {

        // Check the status of the background process without blocking
        pid_t pid = waitpid(backgroundPros[i].pid, &backgroundPros[i].status, WNOHANG);

        // If the process is still running, move to the next one
        if (pid == 0) {
            ++i;
        } 

        // If the process has exited
        else if (pid > 0) {

            // Check if the process exited normally
            if (WIFEXITED(backgroundPros[i].status)) {
                printf("Background process %d has completed: exited with status %d\n", backgroundPros[i].pid, WEXITSTATUS(backgroundPros[i].status));
            } 
            // If the process was terminated by a signal
            else {
                
                printf("Background process %d has finished: terminated by signal %d\n", backgroundPros[i].pid, WTERMSIG(backgroundPros[i].status));
            }
            fflush(stdout);

            // Shift the remaining background processes to fill the gap
            for (int j = i; j < countBackgroundPro - 1; ++j) {
                backgroundPros[j] = backgroundPros[j + 1];
            }

           // Decrease the count of background processes
            --countBackgroundPro;
        } else {
            
            perror("waitpid");


            // Move to the next process
            ++i;
        }
    }
}

// Function to occurrences of "$$" in a string with the current process ID
char* subDollars(char* UserInput){

    // Allocate memory for the substituted input string, assuming the length could double due to replacements
    char* subInput = malloc(strlen(UserInput) * 2);
    int found = 0;
    int newIndex = 0;

    // Iterate through each character in the input string
    for (int oldIndex = 0; oldIndex < strlen(UserInput); oldIndex++) {
        // Check for the occurrence of "$$"
        if (UserInput[oldIndex] == '$' && UserInput[oldIndex + 1] == '$') {
            
            // Get the current process ID
            int processID = getpid();
            // Convert the process ID to a string
            char processIDString[20];
            sprintf(processIDString, "%d", processID);

            // Copy the process ID string to the substituted input string
            strcpy(&subInput[newIndex], processIDString);
            // Update the new index to the end of the inserted process ID
            newIndex += strlen(processIDString);

            // Move the old index forward to skip the second '$'
            oldIndex += 1;
            // Set the found flag to true
            found = 1;
        } else {
            
            // Copy the character from the original input to the substituted input
            subInput[newIndex] = UserInput[oldIndex];
            // Move the new index forward
            newIndex++;
        }
    }

    // Null-terminate the substituted input string
    subInput[newIndex] = '\0';
    // Copy the substituted input back to the original input string
    strcpy(UserInput, subInput);

    // Check if "$$" was found in the input
    if (found == 1) {
        // Return the modified input string
        return UserInput;
    } else {
        // If "$$" was not found, return NULL
        return NULL;
    }
}

// Signal handler for SIGINT (Ctrl+C)
void sigint(int signo){
    // Check if the process receiving the signal is the foreground process group
    if (getpid() == getpgrp()) {
        
        // Display message indicating that SIGINT is ignored, and prompt the user
        printf("\n Ignoring Ctrl+C (SIGINT)...\n");
        fflush(stdout);
        // Call the user_prompt function to display the prompt
        user_prompt();
        fflush(stdout);
    } else {
        
        // If not in the foreground process group, terminate the process
        printf(" terminating the process...\n");
        fflush(stdout);
        exit(1);
    }
}

// Signal handler for SIGTSTP (Ctrl+Z)
void sigstp(int signo){

    // Toggle the foreground mode
    foreground = !foreground;

    // Display a message based on the current state of foreground mode
    if(foreground == 1){
        printf("\n Disabling foreground-only mode\n");
    }
    else if(foreground == 0){
        printf("\n Enabling exclusive foreground mode\n");
    }

    // Flush the output buffer and prompt the user
    fflush(stdout);
    user_prompt();
}


// Function to set up signal handlers
void setup_signal_handlers(int signum, void (*handler)(int), const char* signal_name){
    // Set up the specified signal handler
    if(signal(signum, handler) == SIG_ERR){
        perror("Unable to establish signal handler");
        exit(EXIT_FAILURE);
    }
    
}
// Function to set up signal handlers for SIGINT and SIGTSTP
void setup_handlers(){
    // Call the setup_signal_handlers function for SIGINT and SIGTSTP
    setup_signal_handlers(SIGINT, sigint, "SIGINT");
    setup_signal_handlers(SIGTSTP, sigstp, "SIGTSTP");
}

// Function to display the user prompt
void user_prompt(){

    // Check the status of background processes
    checkBackgroundPro();

    // Get the process ID of the current process
    pid_t process_id = getpid();

    // Display the shell prompt
    fflush(stdout);
    printf(":");
    fflush(stdout);
}

// Function to remove newline characters from user input
void removeNewline(char* UserInput){
    // Get the length of the input string
    size_t inputLength = strlen(UserInput);
    // Remove newline character if it exists at the end
    if(inputLength > 0 && UserInput[inputLength - 1] == '\n'){
        UserInput[inputLength - 1] = '\0';
    }
}

// Function to parse user input into a command structure
struct command* pareUserInput(char* UserInput){

    // Allocate memory for the parsed input structure
    struct command* parsedInput = malloc(sizeof(struct command));
    int argcount = 0;

    // Use strtok_r to tokenize the user input
    char* savePointer;
    char* token = strtok_r(UserInput, " ", &savePointer);

    // Allocate memory for the command and copy the token
    parsedInput->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(parsedInput->command, token);

    // Initialize the argument array to NULL
    parsedInput->argument = NULL;

    // Loop to tokenize the remaining arguments
    while(1){

        token = strtok_r(NULL, " ", &savePointer);

        // Check if there are more tokens
        if(token != NULL){

            // Allocate memory for the argument and copy the token
            char* arg = calloc(strlen(token) + 1, sizeof(char));

            // Skip "&" if in background mode
            if((foreground == 0) && (strcmp(token, "&") == 0)){
                continue;
            }

            strcpy(arg, token);

            // Reallocate memory for the argument array and add the argument
            parsedInput->argument = realloc(parsedInput->argument, (argcount + 1) * sizeof(char*));
            parsedInput->argument[argcount] = arg;

            // Increment the argument count
            argcount++;

        }else{
            // Break the loop if there are no more tokens
            break;
        }
    }

    // Check for background mode and adjust the parsed structure accordingly
    if (foreground != 0 && argcount != 0 && strcmp(parsedInput->argument[argcount - 1], "&") == 0 && strcmp(parsedInput->command, "cd") != 0 && strcmp(parsedInput->command, "exit") != 0 && strcmp(parsedInput->command, "status") != 0) {
    parsedInput->background = 1;
    free(parsedInput->argument[argcount - 1]);
    argcount--;
    } else {
    parsedInput->background = 0;
    }

    // Set the argument count in the parsed structure
    parsedInput->count = argcount;

    // Return the parsed structure
    return parsedInput;

}

// Function to check if a string consists of only whitespace characters or is empty
int WhiteSpaceOrEmpty(char* str) {
    // Iterate through each character in the string
    for(int i = 0; str[i] != '\0'; i++){
        // Check if the character is not a whitespace character
        if(!isspace((unsigned char)str[i])){
            return 0;  // Return 0 if a non-whitespace character is found
        }
    }
    return 1; // Return 1 if the string is empty or consists of only whitespace characters
}

// Function to check if a line is a blank line or a comment
int BlankLineOrComment(char* InputLine){
    // Check if the input line is empty or starts with a '#' character
    if(WhiteSpaceOrEmpty(InputLine) || InputLine[0] == '#'){
        return 1; // Return 1 if the line is a blank line or a comment
    }

    return 0; // Return 0 if the line is not a blank line or a comment

}

// Function to process blank lines and comments in user input
int processBlankandComment(char* UserInput){
    // Check if the user input is a blank line or a comment
    if(BlankLineOrComment(UserInput)){
        return 1;
    }

    return 0;

}

// Function to display the current working directory
void DisplayCurrDir(){
    char CurrDir[100000];
    // Get the current working directory
    getcwd(CurrDir, sizeof(CurrDir));
    // Display the current directory
    printf("Change Directory: %s\n", CurrDir);
    fflush(stdout);
}

// Function to display an error when changing the directory
void DisplayChangeDirError(){
    perror("cd");
}

// Function to navigate to the specified directory
void nav_dir(struct command* command){

    char* TargetDir;
    // Set the target directory to HOME if no argument is provided
    if(command->argument == NULL || command->argument[0] == NULL){
        TargetDir = getenv("HOME");
    }else{
        TargetDir = command->argument[0];
    }
    // Attempt to change the directory
    if(chdir(TargetDir) != 0){
        // Display an error if changing the directory fails
        DisplayChangeDirError();
        return;
    }

    // Display the current directory after successful navigation
    DisplayCurrDir();

}

// Function to execute a command
void executeCom(struct command* UserCom);

// Function to process a parsed command
void processCom(struct command* ParsedInput){

    // Check if the command is "cd"
    if (strcmp(ParsedInput->command, "cd") == 0) {
        nav_dir(ParsedInput);
    }
    // Check if the command is "exit"
    else if(strcmp(ParsedInput->command, "exit") == 0){

        killpg(0,15);
        exit(0);
        
    }
    // Check if the command is "status"
    else if(strcmp(ParsedInput->command, "status") == 0){
        // Display the exit status of the last foreground process
        if(ParsedInput->background){
            if(countBackgroundPro > 0){
                last_info = backgroundPros[countBackgroundPro - 1].status;
            }
        }

        printf("Exit Status: %d\n", last_info);
        fflush(stdout);
    }
     // If the command is not a built-in command, execute it
    else{

        executeCom(ParsedInput);
        //implement all the executeCommand in this else statment in function

    }

}

// Function to set up the default signal handler for SIGINT
void setupSignalHandle(){
    signal(SIGINT, SIG_DFL);
}
// Function to set up command arguments for execution
void SetupComArguments(struct command* UserCom, char* args[]){

    args[0] = UserCom->command;

    for (int i = 0; i < UserCom->count; ++i) {
        args[i + 1] = UserCom->argument[i];
    }

    args[UserCom->count + 1] = NULL;

}
// Function to find the index of a redirection symbol in the command
int FindRedirectIndex(struct command* UserCom, const char* RedirectSymbol){

    int ReDirectIn = - 1;

    for(int i = 0; i < UserCom->count + 1; i++){
        if (strcmp(UserCom->argument[i], RedirectSymbol) == 0) {
            ReDirectIn = i;
            break;
        }
    }

    return ReDirectIn;
}

// Function to handle output redirection
void handleOutputRedirection(char* args[], int outRedirect){

    // Open or create a file for writing with the specified permissions
    int fileDescriptor = open(args[outRedirect + 1], O_WRONLY | O_CREAT, 0660);

     // Check for errors in opening the file
    if (fileDescriptor == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int successResult = dup2(fileDescriptor, 1);

    // Check for errors in duplicating the file descriptor
    if (successResult == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
    }

    // Close the file descriptor as it is no longer needed
    close(fileDescriptor);

    // Set the arguments corresponding to the output redirection to NULL
    args[outRedirect] = NULL;
    args[outRedirect + 1] = NULL;

}

// Function to execute a command in a child process
void executeCommandInChildProcess(char* args[]) {
    execvp(args[0], args);
}

// Function to handle the execution of a child process
void HandleChildPro(struct command* UserCom){

    char* args[512];
    SetupComArguments(UserCom, args);

    int inRedirect = FindRedirectIndex(UserCom, "<");
    int outRedirect = FindRedirectIndex(UserCom, ">");

    if (outRedirect != -1) {
        handleOutputRedirection(args, outRedirect);
    }
     // Set up the default signal handler for SIGINT in the child process

    setupSignalHandle();
    // Execute the command in the child process
    executeCommandInChildProcess(args);
    // If execvp fails, display an error message

    perror("execvp");
}

// Function to execute a command
void executeCom(struct command* UserCom){

    // Fork a new process
    pid_t child_PID = fork();

    // Check for errors in forking
    if(child_PID == -1){
        perror("fork");
        exit(EXIT_FAILURE);

    }else if(child_PID == 0){ // Child process
        // Execute command in the child process
        HandleChildPro(UserCom);


    }else {
        // Parent process
        // Check if the command is running in the background
        if (UserCom->background) {
            
            int status;
            pid_t child_pid = waitpid(child_PID, &status, WNOHANG);
            // Check the status of the background process

            if (child_pid == 0) {
                // Display a message if the background process started successfully
                printf("Background process started with PID: %d\n", child_PID);
                fflush(stdout);
            } else if (child_pid > 0) {
                // Do nothing if the background process has already finished
                
            } else {
                // Display an error message if waitpid fails
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        } else { // Foreground process

            // Fork a new process to handle command execution
            pid_t pid = fork();

            // Create an array to hold command arguments
            char* user_args[512];
            user_args[0] = UserCom->command;

            // Copy command arguments to the array
            for(int i = 0; i < UserCom->count; i++){
                user_args[i+1] = UserCom->argument[i];
            }

            user_args[UserCom->count+1] = NULL;
            
            // Check for errors in forking
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { 
                // Set the default signal handler for SIGINT in the child process
                signal(SIGINT, SIG_DFL);  

                 // Check for input and output redirection in the command
                int redirect_in = -1;
                int redirect_out = -1;

                
                for(int i = 0; i < UserCom->count+1; i++){
                    if(strcmp(user_args[i], ">") == 0){
                        redirect_out = i;
                    }
                    else if(strcmp(user_args[i], "<") == 0){
                        redirect_in = i;
                    }
                }

                // Handle output redirection
                if(redirect_out != -1){
    
                     
                    int fd = open(user_args[redirect_out + 1], O_WRONLY | O_CREAT, 0660);

                    
                    if (fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    
                    int success_result = dup2(fd, 1);

                    
                    if(success_result == -1){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }

                    
                    close(fd);
                    
                    
                    user_args[redirect_out] = NULL;
                    user_args[redirect_out +1] = NULL;

                }

                // Handle input redirection                
                if(redirect_in != -1){
                    int fd = open(user_args[redirect_in + 1], O_RDONLY);
                    if (fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    
                    int success_result = dup2(fd, 0);

                    if(success_result == -1){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }

                    
                    close(fd);
                    user_args[redirect_in] = NULL;
                    user_args[redirect_in +1] = NULL;
                }

                // Execute the command in the child process
                execvp(user_args[0], user_args);

                // Display an error message if execvp fails            
                perror("execvp");
                exit(EXIT_FAILURE);


            } else { 
                // Check if the command is running in the background
                if (UserCom->background) {
                    // Display a message if the background process started successfully
                    printf("Background process initiated with Process ID: %d\n", pid);
                    // Save background process information
                    backgroundPros[countBackgroundPro].pid = pid;
                    backgroundPros[countBackgroundPro].status = 0;  // Initialize status

                    ++countBackgroundPro;

                    fflush(stdout);
                    
                }
                else {
                    
                    // Foreground process: wait for the child process to finish
                    int status;
                    pid_t child_pid = waitpid(pid, &status, 0);

                    // Check the exit status of the child process
                    if (child_pid == -1) {
                        perror("waitpid");
                        exit(EXIT_FAILURE);
                    }

                    // Update the last_info variable based on the child process status                    
                    if (WIFEXITED(status)) {
                        last_info = WEXITSTATUS(status);
                        
                    }
                    else if (WIFSIGNALED(status)) {
                        // Display a message if the child process terminated by a signal
                        int signal_number = WTERMSIG(status);
                        printf("Child process concluded due to signal %d: %s\n", signal_number, strsignal(signal_number));
                        fflush(stdout);
                        last_info = signal_number;
                    }
                }
            }
        }            
    }
}

int main() {
    int Com_blank;
    char User_input[2048];

    // Display the initial user prompt and set up signal handlers
    user_prompt();
    setup_handlers();

    while (1) {

        // Get user input from the command line
        fgets(User_input, sizeof(User_input), stdin);

        // Substitute occurrences of "$$" with the current process ID
        char* result = subDollars(User_input);

        // If substitution occurred, update the user input
        if (result != NULL) {
            strcpy(User_input, result);
        }

        // Remove newline characters from the user input
        removeNewline(User_input);

        // Check for blank lines or comments, and skip further processing if true
        Com_blank = processBlankandComment(User_input);
        if (Com_blank == 1) {
            user_prompt();
            continue;
        }

        // Parse the user input into a command structure
        struct command* ParsedInput = pareUserInput(User_input);

         // Process the parsed command
        processCom(ParsedInput);

    } 
    return 0; 
}


