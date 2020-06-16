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

/*void parseEnvVar(char *arg) {
    if(startsWith(arg, PREFIX_ENVVAR)){//Variabili d'ambiente
        removeFirstChar(arg);
        //printf("ARG2: %s\n", arg);
        //char *newArg = getenv(arg);
        //printf("ARG1: %s\n", newArg);
        arg = getenv(arg);
    }
}

int redirectStdOut(char *arg) {

}

int redirectStdIn(char *arg) {
    if(startsWith(arg, PREFIX_REDIRINPUT)){
        removeFirstChar(arg);
        int newFileDescriptor = open(arg, O_RDWR | O_CREAT | O_TRUNC, 0644);
        int saveStdInFileDescriptor =  dup(stdin);
        dup2(stdin, newFileDescriptor);

    }
    else if(startsWith(arg, PREFIX_REDIROUTPUT)){
        removeFirstChar(arg);

    }
    
}*/

void execCommand(char *const *arrayArg) {
    pid_t forkRet = fork();

    if(forkRet == 0) {
        //Solo figlio
        execvp(arrayArg[0], arrayArg);
        exit(EXIT_SUCCESS);
    }
    else if(forkRet < 0){
        perror("Fork fallita");
        exit(EXIT_FAILURE);
    }
    //Solo padre
    wait(NULL);
}

int parseCommand(const char *comando) {
    char *str, *token, *saveptr;
    int tokenNum = countToken(comando, DELIMITER_COMMAND);

    char **arrayArg = malloc((sizeof(char*) * tokenNum) + 1);
    int i;

    int saveStdInFileDescriptor = -1;
    int saveStdOutFileDescriptor = -1;
    int newInFileDescriptor = 0;
    int newOutFileDescriptor = 0;

    str = malloc(sizeof(char) * (strlen(comando) + 1));
    strcpy(str, comando);
    for(i = 0; ; str = NULL, ++i) {
        token = __strtok_r(str, DELIMITER_COMMAND, &saveptr);
        if (token == NULL) {
            break;
        }

        if(i > 0) {
            //parseArg(token);

            if(startsWith(token, PREFIX_ENVVAR)){//Variabili d'ambiente
                removeFirstChar(token);
                token = getenv(token);
                arrayArg[i] = token;
            } 
            else if(startsWith(token, PREFIX_REDIRINPUT)){
                if(i == 1) {
                    removeFirstChar(token);
                    newInFileDescriptor = open(token, O_RDWR | O_CREAT | O_TRUNC, 0644);
                    saveStdInFileDescriptor =  dup(stdin);
                    dup2(stdin, newInFileDescriptor);
                }
                else {
                    printf("Redirezione stdin non come primo argomento(%d)", i);
                    return 0;
                }
            }
            else if(startsWith(token, PREFIX_REDIROUTPUT)){
                if(i == tokenNum -1) {
                    removeFirstChar(token);
                    newOutFileDescriptor = open(token, O_RDWR | O_CREAT | O_TRUNC, 0644);
                    saveStdInFileDescriptor =  dup(stdout);
                    dup2(stdout, newOutFileDescriptor);
                }
                else {
                    printf("Redirezione stdout non come ultimo argomento(%d)", i);
                    return 0;
                }
            }
            else{
                arrayArg[i] = token;
            }
        }
        else {
            arrayArg[i] = token;
        }


        //isValid &= parseComandoValue(token);
    }
    arrayArg[tokenNum] = NULL;

    execCommand(arrayArg);

    if(saveStdInFileDescriptor >= 0) {
        dup2(saveStdInFileDescriptor, stdin);
        close(newInFileDescriptor);
    }
    if(saveStdOutFileDescriptor >= 0) {
        dup2(saveStdOutFileDescriptor, stdout);
        close(newOutFileDescriptor);
    }

    //TODO: close di saveStdInFileDescriptor e saveStdOutFileDescriptor??



    free(str);
    free(arrayArg);//VALGRIND
    return 1;
}

int parseLine(const char *linea) {
    char *str, *token, *saveptr;
    int isValid = 1;

    str = malloc(sizeof(char) * (strlen(linea) + 1));
    strcpy(str, linea);
    for(; ; str = NULL) {
        token = __strtok_r(str, DELIMITER_PIPE, &saveptr);
        if (token == NULL) {
            break;
        }
        isValid &= parseCommand(token);
    }

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

