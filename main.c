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

struct executableCommand 
{
    char **command;
    char *stdinRedirect;
    char *stdoutRedirect;
};

char *readLine(){
    return readline("uBash>");
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
    char *str, *saveptr;

    str = malloc(sizeof(char) * (strlen(intputStr) + 1));
    strcpy(str, intputStr);

    while (__strtok_r(str, delimiter, &saveptr) != NULL) {
        count++;
        if(str != NULL) {
            free(str);
        }
        str = NULL;
    }

    //free(str);
    return count;
}

struct executableCommand parseCommand(const char *comando) {
    char *str, *token, *saveptr;
    int tokenNum = countToken(comando, DELIMITER_COMMAND);

    struct executableCommand *commandStructNULL = NULL;//TODO: REMOVE
    struct executableCommand commandStruct;

    commandStruct.command = malloc((sizeof(char*) * (tokenNum + 1)));
    char **executable = commandStruct.command;

    int i;

    str = malloc(sizeof(char) * (strlen(comando) + 1));
    strcpy(str, comando);
    for(i = 0; ; str = NULL, ++i) {
        token = __strtok_r(str, DELIMITER_COMMAND, &saveptr);
        if (token == NULL) {
            break;
        }
        if(i > 0) {
            if(startsWith(token, PREFIX_ENVVAR)){//Variabili d'ambiente
                removeFirstChar(token);
                token = getenv(token);
                
                executable[i] = malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(executable[i], token);
                //arrayArg[i] = token;
            } 
            else if(startsWith(token, PREFIX_REDIRINPUT)){
                removeFirstChar(token);
                if(commandStruct.stdinRedirect != NULL) {
                    commandStruct.stdinRedirect = malloc(sizeof(char) * (strlen(token) + 1));
                    strcpy(commandStruct.stdinRedirect, token);
                    //commandStruct.stdinRedirect = token;
                }
                else {
                    return *commandStructNULL;
                }
            }
            else if(startsWith(token, PREFIX_REDIROUTPUT)){
                removeFirstChar(token);
                if(commandStruct.stdoutRedirect != NULL) {
                    commandStruct.stdoutRedirect = malloc(sizeof(char) * (strlen(token) + 1));
                    strcpy(commandStruct.stdoutRedirect, token);
                    //commandStruct.stdoutRedirect = token;
                }
                else {
                    return *commandStructNULL;
                }
            }
            else{
                executable[i] = malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(executable[i], token);
            }
        }
        else {
            executable[i] = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(executable[i], token);
        }

        if(str != NULL) {
            free(str);
        }
    }
    executable[tokenNum] = (char*)NULL;

    return commandStruct;
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

            dup2(redirIN, 0);
            if(redirIN != 0) {
                close(redirIN);
            }
            if(i < (size-1)) {//IF NOT LAST
                dup2(fd[1], 1);
            }
            else{
                dup2(stdOUT, 1);
            }
            close(fd[0]);
            close(fd[1]);
            execvp(executables[i].command[0], executables[i].command);
        }
        else if(forkRet < 0){
            perror("Fork fallita");
            return 0;
        }

        wait(NULL);
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

int parseLine(const char *linea) {
    char *str, *token, *saveptr;
    int isValid = 1;

    str = (char*)malloc(sizeof(char) * (strlen(linea) + 1));
    strcpy(str, linea);


    int size = countToken(linea, DELIMITER_PIPE);
    struct executableCommand executables[size];

    int stdIN = 0;
    int stdOUT = 1;

    int i;
    for(i = 0; ; str = NULL, ++i) {
        token = __strtok_r(str, DELIMITER_PIPE, &saveptr);
        if (token == NULL) {
            break;
        }

        struct executableCommand commandStruct = parseCommand(token);

        printf("REDIR: %s %s", commandStruct.stdinRedirect, commandStruct.stdoutRedirect);
        if(commandStruct.stdinRedirect != NULL) {
            if(i == 0) {
                stdIN = open(commandStruct.stdinRedirect, O_RDONLY);
            }
            else{
                printf("Redirezione dell'input in %d-esima posizione.", i);
                return 0;
            }
        }

        if(commandStruct.stdoutRedirect != NULL) {
            if(i == (size-1)){
                stdOUT = open(commandStruct.stdinRedirect, O_WRONLY | O_CREAT, S_IWUSR);
            }
            else {
                printf("Redirezione dell'output in %d-esima posizione.", i);
                return 0;
            }
        }

        executables[i] = commandStruct;
        if(str != NULL){
            free(str);
        }
    }

    /*for(i =0; i < size; ++i) {
        printf("ARGS: ");
        int j = 0;
        char* command;
        while((command = executables[i].command[j++]) != NULL){
            printf("%s ", command);
        }
        printf("NULL \n");
    }*/


    execCommands(executables, size, stdIN, stdOUT);

    int j;
    for(i = 0; i < size; ++i) {
        j = 0;
        char* command;
        while((command = executables[i].command[j++]) != NULL){
            free(command);
        }
        free(executables[i].command);
    }
    //free(executables);

    free(str);
    return isValid;
}

int main(int argc, char *argv[]) {

    char *cmdLine = readLine();
    while(cmdLine != NULL) {
        add_history(cmdLine);

        int isValid = parseLine(cmdLine);

        printf("ISVALID: %d\n", isValid);

        free(cmdLine);
        cmdLine = readLine();
    }

    free(cmdLine);

    printf("\nDone!\n");

    return EXIT_SUCCESS;
}
