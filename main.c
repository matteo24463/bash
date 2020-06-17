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
        str = NULL;
    }

    free(str);
    return count;
}

int parseCommand(const char *comando, char *executable[], char *stdinRedirect, char *stdoutRedirect) {
    char *str, *token, *saveptr;
    int tokenNum = countToken(comando, DELIMITER_COMMAND);

    //char **arrayArg = malloc((sizeof(char*) * tokenNum) + 1);
    char *arrayArg[tokenNum+1];
    int i;

    str = malloc(sizeof(char) * (strlen(comando) + 1));
    strcpy(str, comando);
    for(i = 0; ; str = NULL, ++i) {
        token = __strtok_r(str, DELIMITER_COMMAND, &saveptr);
        if (token == NULL) {
            break;
        }
        printf("TOKEN: %s\n", token);

        if(i > 0) {
             printf("TOKEN2");
            if(startsWith(token, PREFIX_ENVVAR)){//Variabili d'ambiente
                removeFirstChar(token);
                token = getenv(token);
                strcpy(arrayArg[i], token);
                //arrayArg[i] = token;
            } 
            else if(startsWith(token, PREFIX_REDIRINPUT)){
                removeFirstChar(token);
                if(stdinRedirect != NULL) {
                    stdinRedirect = token;
                }
                else {
                    return 0;
                }
            }
            else if(startsWith(token, PREFIX_REDIROUTPUT)){
                removeFirstChar(token);
                if(stdoutRedirect != NULL) {
                    stdoutRedirect = token;
                }
                else {
                    return 0;
                }
            }
            else{
                printf("TOKEN3: %s: ", token);
                strcpy(arrayArg[i], token);
                printf("TOKEN3: %s: ", arrayArg[i]);
                //arrayArg[i] = token;
            }
        }
        else {
            printf("COMMAND1: ");
            strcpy(arrayArg[i], token);
            //arrayArg[i] = token;
            printf("COMMAND1: %s: ", arrayArg[i]);
        }
    }
    arrayArg[tokenNum] = NULL;

    printf("COMMAND");
    printf("COMMAND: %s: ", arrayArg[0]);
    executable = arrayArg;
    printf("COMMAND: %s: ", executable[0]);

    free(str);
    //free(arrayArg);//VALGRIND
    return 1;
}

int pipeCommand(char **command1, char **command2) {

    pid_t forkRet = fork();

    if(forkRet == 0) {
        //EXEC coomand1
        int fd[2];
        if (pipe(fd) != 0) {
            printf("Pipe fallita\n");
            return 0;
        }

        pid_t childPid = fork(); 

        if (childPid == -1) {
            printf("Fork 2 fallita\n");
            return 0;
        }
        else if (childPid == 0) {
            dup2(fd[1], 1);
            close(fd[0]);
            close(fd[1]);
            execvp(command1[0], command1);
            printf("Esecuzione di %s fallita", command1[0]);
            return 0;
        }
        else {
            dup2(fd[0], 0);
            close(fd[0]);
            close(fd[1]);
            execvp(command2[0], command2);
            printf("Esecuzione di %s fallita", command2[0]);
            return 0;
        }

        //exit(EXIT_SUCCESS);
    }
    else if(forkRet < 0){
        perror("Fork 1 fallita");
        return 0;
    }

    wait(NULL);
    return 1;
}

int execCommands(char **executables[], int size, int stdin, int stdout) {

        printf("EXECUTING:\n");
        printf("EXECUTING: %s\n", executables[0][0]);

    int fd[2];
    if (pipe(fd) != 0) {
        printf("Pipe fallita\n");
        return 0;
    }

    pid_t forkRet = fork();

    if(forkRet == 0) {
        printf("CHILD:\n");
        printf("CHILD: %d\n", size);
        dup2(stdin, 0);
        if(size > 1) {
            dup2(fd[1], 1);
        }
        else{
            dup2(stdout, 1);
        }
        close(fd[0]);
        close(fd[1]);

        execvp(executables[0][0], executables[0]);
    }
    else if(forkRet < 0){
        perror("Fork 1 fallita");
        return 0;
    }

    wait(NULL);
    for(int i = 0; i < size; ++i) {
        executables[i] = executables[i+1];
    }

    if(size > 0) {
        return execCommands(executables, size-1, fd[0], stdout);
    }
    else {
        return 1;
    }

}

int parseLine(const char *linea) {
    char *str, *token, *saveptr;
    int isValid = 1;

    str = malloc(sizeof(char) * (strlen(linea) + 1));
    strcpy(str, linea);


    char **executable = NULL;
    char *stdinRedirect = NULL;
    char *stdoutRedirect = NULL;

    int size = countToken(linea, DELIMITER_PIPE);//TODO: count
    char **executables[size];

    int i;
    for(i = 0; ; str = NULL, ++i) {
        token = __strtok_r(str, DELIMITER_PIPE, &saveptr);
        if (token == NULL) {
            break;
        }
        isValid &= parseCommand(token, executable, stdinRedirect, stdoutRedirect);
        executables[i] = executable;
        printf("SAVING: %s\n", executable[0]);

    }


    execCommands(executables, size, 0, 1);

    /*for(int i = 0; i < size; ++i) {
        free(executables[i]);
    }*/

    free(str);
    return isValid;
}

int main(int argc, char *argv[]) {

    char *cmdLine = readLine();
    while(cmdLine != NULL) {
        add_history(cmdLine);

        int isValid = parseLine(cmdLine);

        printf("ISVALID: %d\n", isValid);

        cmdLine = readLine();
    }

    printf("Done!\n");

    return EXIT_SUCCESS;
}
