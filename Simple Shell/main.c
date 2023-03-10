#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#define DIRECTORY_LENGTH 1000
#define COMMAND_LENGTH 1000
#define WORD_LENGTH 40

FILE* file;

//terminates zombie process using waitpid(-1, ...)
//prints the id of the terminated process into the logfile
void reap_child_zombie()
{
    int status;
    int pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // `pid` exited with `status`
        write_to_log_file(pid);
    }
}

//handles the logfile
void write_to_log_file(int pid)
{
    file = fopen("/home/mohamed/Desktop/Simple Shell/logfile.txt", "a+");
    fprintf(file, "Child Zombie Process with id: %d Terminated\n", pid);
    fclose(file);
}

void on_child_exit()
{
    reap_child_zombie();
}

//changes the current directory to be the home directory
void setup_environment()
{
    chdir(getenv("HOME"));
    printf("%s$ ", getenv("HOME"));
}

//self-explanatory
void print_current_directory()
{
    char directory[DIRECTORY_LENGTH];
    getcwd(directory, sizeof(char)*DIRECTORY_LENGTH);
    printf("%s$ ", directory);
}

//takes the full command and puts the characters of the first word in
//a location in memory pointed at by the character pointer "firstword"
void get_command_first_word(char* command, char* firstword)
{
    for(int i = 0; i < WORD_LENGTH; i++){
        if(command[i] != ' '){
            firstword[i] = command[i];
        }else break;
    }
}

//takes the command and replaces each $ and the name following it by
//the value stored in the shell then returns the new evaluated command
char* evaluate_command(char* command)
{
    char* newcommand = (char*) malloc(COMMAND_LENGTH);
    char* newcommand_start = newcommand;
    int flag = 0;
    while(command[0] != '\0'){
        if(flag) sprintf(newcommand, " "), newcommand++;
        else flag = 1;
        char* temp = (char*) malloc(WORD_LENGTH);
        get_command_first_word(command, temp);
        command += strlen(temp) + 1;
        if(temp[0] == '"' && temp[1] == '$' || temp[0] == '$'){ //here I hardcoded all possible combinations of the format that the $ can be typed in
            char* name = (char*) malloc(WORD_LENGTH);           //("$...), (..$..), (...$..."), etc...
            if(temp[0] == '"' && temp[strlen(temp) - 1] == '"')
                strncpy(name, temp + 2, strlen(temp) - 3);
            else if(temp[strlen(temp) - 1] == '"')
                strncpy(name, temp + 1, strlen(temp) - 2);
            else if(temp[0] == '"')
                strncpy(name, temp + 2, strlen(temp) - 2);
            else
                strncpy(name, temp + 1, strlen(temp) - 1);
            char* value = (char*) malloc(WORD_LENGTH);
            value = getenv(name);
            sprintf(newcommand, "%s", value), newcommand += strlen(value);
        }else{
            sprintf(newcommand, "%s", temp);
            newcommand += strlen(temp);
        }
    }
    return newcommand_start;
}

//executes the built in shell commands that does not work with execvp
void execute_shell_builtin(char* firstword, char* command)
{
    if(!strcmp(firstword, "cd"))
        execute_cd(command);
    else if(!strcmp(firstword, "export"))
        execute_export(command);
    else if(!strcmp(firstword, "echo"))
        execute_echo(command);
}

//executes cd using chdir()
void execute_cd(char* command)
{
    if(strlen(command) <= 3){
        return;
    }
    char* new_directory = (char*) malloc(COMMAND_LENGTH);
    strncpy(new_directory, command + 3, strlen(command) - 3);
    if(!strcmp(new_directory, "~")){
        chdir(getenv("HOME"));
    }else if(chdir(new_directory) != 0){
        printf("bash: cd: %s: No such file or directory\n", new_directory);
    }
}

//stores the value of the given name using setenv()
void execute_export(char* command)
{
    char* name = (char*) malloc(COMMAND_LENGTH);
    char* value = (char*) malloc(COMMAND_LENGTH);
    int name_index = 0;
    while(command[0] != ' ') command++;
    command++;
    while(command[0] != '='){
        name[name_index++] = command[0];
        command++;
    }
    command++;
    if(command[0] == '"')
        strncpy(value, command + 1, strlen(command) - 2);
    else
        strncpy(value, command, strlen(command));
    setenv(name, value, 1);
}

//prints a given string of words or variables
void execute_echo(char* command)
{
    if(strlen(command) <= 5){
        return;
    }
    char* expression = (char*) malloc(COMMAND_LENGTH);
    if(command[5] == '"' && command[strlen(command) - 1] == '"') //handles all possible combinations of user input (with or without quotes)
        strncpy(expression, command + 6, strlen(command) - 7);
    else if(command[strlen(command) - 1] == '"')
        strncpy(expression, command + 5, strlen(command) - 6);
    else if(command[5] == '"')
        strncpy(expression, command + 6, strlen(command) - 6);
    else
        strncpy(expression, command + 5, strlen(command) - 5);
    printf("%s\n", expression);
}

//parses the arguments from the given "statement" and stores them as individual strings in the 2-d array "result"
void get_arguments(char* statment, char** result)
{
    char* temp = (char*) malloc(COMMAND_LENGTH);
    strncpy(temp, statment, strlen(statment));
    int words = 1;
    while(temp[0] != '\0'){
        if(temp[0] == ' ')
            words++;
        temp++;
    }
    if(words == 1){
        result[0] = NULL; //in case of commands like "ls" with no arguments, the "command_args" (in execute_command()) should contain NULL
    }else{ //else get the arguments
        int result_index = 0;
        while(statment[0] != '\0'){
            char* a_word = (char*) malloc(WORD_LENGTH);
            get_command_first_word(statment, a_word);
            statment += (strlen(a_word) + 1);
            result[result_index++] = a_word;
        }
        result[result_index] == NULL;
    }
}

//executes commands other than the built in ones using execvp()
void execute_command(char* statement, int background)
{
    int index = 0;
    char* expression = (char*) malloc(COMMAND_LENGTH);
    strncpy(expression, statement, strlen(statement));
    char* command = (char*) malloc(WORD_LENGTH);
    while(expression[0] != ' ' && expression[0] != '\0'){
        expression++;
        index++;
    }
    if(expression[0] != '\0')
        expression++;
    strncpy(command, statement, index);
    char** command_args = (char**) malloc(WORD_LENGTH);
    get_arguments(statement, command_args);
    pid_t pid = fork();
    if(pid == 0){
        execvp(command, command_args);
        printf("%s: command not found\n", command);
        exit(1);
    }else{
        if(!background) waitpid(pid, NULL, 0);
    }
}

//the shell loop exits only when the exit command is recieved
void shell()
{
    while(1){
        char* command = (char*) malloc(COMMAND_LENGTH); //the whole input
        char* evaluated_command = (char*) malloc(COMMAND_LENGTH); //the evaluated input ($ case)
        int background = 0; //sets to 1 if the evaluated command ends in an & sign
        gets(command, COMMAND_LENGTH, stdin); //reads the given input using gets() ....
                //(Note: I tried using fgets() instead to avoid any possible buffer overflow but it dodn't work at all)
        evaluated_command = evaluate_command(command);
        if(evaluated_command[strlen(evaluated_command) - 1] == '&'){
            evaluated_command[strlen(evaluated_command) - 2] = '\0';
            background = 1;
        }
        if(!strcmp(evaluated_command, "exit")) exit(0);
        char firstword[WORD_LENGTH] = {'\0'};
        get_command_first_word(evaluated_command, firstword);
        if(!strcmp(firstword, "cd") || !strcmp(firstword, "export") || !strcmp(firstword, "echo")){
            execute_shell_builtin(firstword, evaluated_command);
        }else{
            execute_command(evaluated_command, background);
        }
        print_current_directory();
    }
}

//driver code
int main()
{
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();
    return 0;
}
