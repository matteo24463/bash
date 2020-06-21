#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//readline/add_hystory
#include <readline/readline.h>
#include <readline/history.h>

//fork/exec/wait
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//open/close
#include <sys/stat.h>
#include <fcntl.h>

#define DELIMITER_COMMAND " "
#define DELIMITER_PIPE "|"

#define PREFIX_ENVVAR "$"
#define PREFIX_REDIRINPUT "<"
#define PREFIX_REDIROUTPUT ">"

char *PATH;

struct executableCommand 
{
    char **command;
    char *stdinRedirect;
    char *stdoutRedirect;
};

char *readLine(){ 
    char rightString[strlen(PATH) + 2];
    strcpy(rightString, "uBash:");
    strcat(rightString, PATH);
    strcat(rightString, ">");
    

    return readline(rightString);
}

int startsWith(const char *str, const char *pre)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

void removeFirstChar(char *str){
    int i, j;
    for(i=0; str[i]!='\0'; i++) {
		if(i==0 || (str[i]==' ' && str[i+1]!=' ')) {
			for(j=((i==0)?i:i+1); str[j]!='\0'; j++) {
				str[j]=str[j+1];
            }
		}
	}
}

int countToken(const char *intputStr, char *delimiter) {
    int count = 0;

    char str[strlen(intputStr) + 1];
    strcpy(str, intputStr);

    char *token = strtok(str, delimiter);
    while (token != NULL) {
        count++;
        token = strtok(NULL, delimiter);
    }
    return count;
}

int parseCommand(const char *comando, struct executableCommand *commandStruct) {
    char *saveptr;
    int tokenNum = countToken(comando, DELIMITER_COMMAND);

    commandStruct->stdinRedirect = NULL;
    commandStruct->stdoutRedirect = NULL;

    commandStruct->command = malloc(sizeof(char*) * (tokenNum + 1));
    char **executable = commandStruct->command;

    char str[strlen(comando) + 1];// = malloc(sizeof(char) * (strlen(comando) + 1));
    strcpy(str, comando);

    int i = 0;
    char *token = __strtok_r(str, DELIMITER_COMMAND, &saveptr);
    while(token != NULL) {
        if(i > 0) {
            if(startsWith(token, PREFIX_ENVVAR)){//Variabili d'ambiente
                removeFirstChar(token);
                token = getenv(token);
                
                executable[i] = malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(executable[i], token);
            } 
            else if(startsWith(token, PREFIX_REDIRINPUT)){
                removeFirstChar(token);
                if(commandStruct->stdinRedirect == NULL) {
                    if(strlen(token) > 0) {
                        commandStruct->stdinRedirect = malloc(sizeof(char) * (strlen(token) + 1));
                        strcpy(commandStruct->stdinRedirect, token);
                    }
                    else {
                        printf("Redirezione stdin non valida\n");
                        return 0;
                    }
                }
                else {
                    printf("E' presente più una redirezione di stdin in %s\n", executable[0]);
                    return 0;
                }
            }
            else if(startsWith(token, PREFIX_REDIROUTPUT)){
                removeFirstChar(token);
                if(commandStruct->stdoutRedirect == NULL) {
                    if(strlen(token) > 0) {
                        commandStruct->stdoutRedirect = malloc(sizeof(char) * (strlen(token) + 1));
                        strcpy(commandStruct->stdoutRedirect, token);
                    }
                    else {
                        printf("Redirezione stdout non valida\n");
                        return 0;
                    }
                }
                else {
                    printf("E' presente più una redirezione di stdout in %s\n", executable[0]);
                    return 0;
                }
            }
            else{
                executable[i] = malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(executable[i], token);
            }
        }
        else {//Comnmand name
            executable[i] = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(executable[i], token);
        }

        token = __strtok_r(NULL, DELIMITER_COMMAND, &saveptr);
        i++;
    }
    if(i == 0) {//IF Command empty
        printf("Comando vuoto\n");
        return 0;
    }

    executable[tokenNum] = (char*)NULL;

    return 1;
}


int execCommands(struct executableCommand *executables, int size, int stdIN, int stdOUT) {
    int i;

    int redirIN = stdIN;
    int fd[2];

    for(i = 0; i < size; ++i) {
        
        if (pipe(fd) != 0) {
            printf("Pipe fallita\n");
            return 0;
        }
        
        pid_t forkRet = fork();
        if(forkRet == 0) {//CHILD
            dup2(redirIN, STDIN_FILENO);
            if(redirIN != 0) {
                close(redirIN);
            }
            
            if(i < (size-1)) {//IF NOT LAST
                dup2(fd[1], STDOUT_FILENO);
            }
            else{
                dup2(stdOUT, STDOUT_FILENO);
            }
            close(fd[0]);
            close(fd[1]);

            char *pathArray[2] = {PATH, NULL};

            execvpe(executables[i].command[0], executables[i].command, pathArray);
            exit(EXIT_FAILURE);
        }
        else if(forkRet < 0){
            perror("Fork fallita");
            return 0;
        }

        int processResult;
        wait(&processResult);
        if(WIFEXITED(processResult) == 0){
            printf("Esecuzione di %s fallita. Codice errore %d\n", executables[i].command[0], WEXITSTATUS(processResult));
        }
        

        close(fd[1]);
        if(redirIN != 0) {
            close(redirIN);
        }
        redirIN = fd[0];
    }

    if(redirIN != 0) {
        close(redirIN);
    }
    if(stdOUT != 1){
        close(stdOUT);
    }
    return 1;
}

void execCD(const char *pathString){
    free(PATH);
    PATH = malloc(sizeof(char) * (strlen(pathString)+1));
    strcpy(PATH, pathString);
}

void parseLine(const char *linea) {
    char *saveptr;
    int isValid = 1;

    int size = countToken(linea, DELIMITER_PIPE);
    struct executableCommand executables[size];

    int stdIN = STDIN_FILENO;
    int stdOUT = STDOUT_FILENO;

    char str[strlen(linea) + 1];
    strcpy(str, linea);

    int i = 0;
    int k = 0;//number of commands to free at the end
    char *token = __strtok_r(str, DELIMITER_PIPE, &saveptr);
    while(token != NULL) {

        struct executableCommand commandStruct;
        int result = parseCommand(token, &commandStruct);

        executables[i] = commandStruct;
        k++;

        if(result == 0) {
            printf("Comando non valido\n");
            isValid = 0;
            break;
        }

        if(strcmp(commandStruct.command[0], "cd") == 0) {
            //executables[i] = commandStruct;//TO be free at the end
            if(size == 1){
                if(commandStruct.stdinRedirect == NULL && commandStruct.stdoutRedirect == NULL){
                    if(countToken(token, DELIMITER_COMMAND) == 2){
                        execCD(commandStruct.command[1]);
                        isValid = 0;
                        break;
                    }
                    else{
                        printf("Il comando cd ammette esattamente un argomento\n");
                        isValid = 0;
                        break;
                    }

                }
                else{
                    printf("Il comando cd non ammette redirezioni\n");
                    isValid = 0;
                    break;
                }
            }
            else{
                printf("Il comando cd non ammette pipe\n");
                isValid = 0;
                break;
            }
        }

        if(commandStruct.stdinRedirect != NULL) {
            if(i == 0) {

                char *stringPath = malloc(strlen(PATH) + strlen(commandStruct.stdinRedirect) + 2);
                strcpy(stringPath, PATH);
                strcat(stringPath, "/");
                strcat(stringPath, commandStruct.stdinRedirect);

                stdIN = open(stringPath, O_RDONLY);

                free(stringPath);
            }
            else{
                printf("Redirezione dell'input in %d-esima posizione\n", i);
                isValid = 0;
                break;
            }
        }

        if(commandStruct.stdoutRedirect != NULL) {
            if(i == (size-1)){
                stdOUT = open(commandStruct.stdoutRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            else {
                printf("Redirezione dell'output in %d-esima posizione\n", i);
                isValid = 0;
                break;
            }
        }

        //executables[i] = commandStruct;
        
        token = __strtok_r(NULL, DELIMITER_PIPE, &saveptr);
        i++;
    }

    if(isValid == 1) {
        execCommands(executables, size, stdIN, stdOUT);
    }
    
    int j;
    for(i = 0; i < k; ++i) {
        j = 0;
        char* command;
        if(executables[i].command != NULL) {
            while((command = executables[i].command[j++]) != NULL){
                free(command);
            }
            free(executables[i].command);
        }
        if(executables[i].stdinRedirect != NULL) {
            free(executables[i].stdinRedirect);
        }
        if(executables[i].stdoutRedirect != NULL) {
            free(executables[i].stdoutRedirect);
        }
    }
}

int main(int argc, char *argv[]) {
    char *currentPath = getenv("PWD");
    PATH = malloc(sizeof(char) * (strlen(currentPath)+1));
    strcpy(PATH, currentPath);

    char *cmdLine;// = readLine();
    while((cmdLine = readLine()) != NULL) {
        add_history(cmdLine);

        parseLine(cmdLine);

        free(cmdLine);
        //cmdLine = readLine();
    }

    //free(cmdLine);
    free(PATH);

    printf("\nDone!\n");

    return EXIT_SUCCESS;
}
