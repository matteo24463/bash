#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define DELIMITER_COMMAND " "
#define DELIMITER_PIPE "|"

#define PREFIX_ENVVAR "$"


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

void parseArg(char *arg) {
    printf("ARG: %s\n", arg);
    if(startsWith(arg, PREFIX_ENVVAR)){
        removeFirstChar(arg);
        printf("ARG2: %s\n", arg);
        char *newArg = getenv(arg);
        printf("ARG1: %s\n", newArg);
        arg = getenv(arg);
    }
}

int parseCommand(const char *comando) {
    char *str, *token, *saveptr;
    int isValid = 1;
    int tokenNum = countToken(comando, DELIMITER_COMMAND);

    char **arrayArg = malloc(sizeof(char*) * tokenNum);
    int i;

    str = malloc(sizeof(char) * (strlen(comando) + 1));
    strcpy(str, comando);
    for(i = 0; ; str = NULL, ++i) {
        token = __strtok_r(str, DELIMITER_COMMAND, &saveptr);
        if (token == NULL) {
            break;
        }
        parseArg(token);
        arrayArg[i] = token;
        //isValid &= parseComandoValue(token);
    }

    free(str);
    free(arrayArg);//VALGRIND
    return isValid;
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



int main() {
    char s[] = "Banana";
    removeFirstChar(s);
    printf("%s\n", s);

    char *cmdLine = readLine();
    while(cmdLine != NULL) {
        add_history(cmdLine);

        int isValid = parseLine(cmdLine);

        printf("ISVALID: %d\n", isValid);

        cmdLine = readLine();
    }

    printf("Done!\n");

    return 0;
}

