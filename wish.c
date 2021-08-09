#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

char **paths;
int pathLen = 1;
int execAux = 0;
int execP = 0;
void command(char *line);
void commandSelect(char **words, int count, int redir);
void rExecute(char **words, int index);
int find(char **words, int len);
void commandChangeDir(char **words);
void runC(char **words);
void pathAdd(char **words);
char **copy_command(int start, int end, char **command);
int countWord(char *line);
int commandCount(char *line);

static char error_message[25] = "An error has occurred\n"; //Mensaje de error

//Metodo principal
int main(int argc, char **argv)
{
    //bin
    char *bin = "/bin";
    paths = (char **)malloc(3 * sizeof(char *));
    paths[pathLen - 1] = bin;
    char *line;
    size_t len = 0;
    ssize_t lineSize = 0;

    // modo de shell interactivo
    if (argc == 1)
    {
        int seguir = 1;
        // Ciclo principal
        while (seguir == 1)
        {
            printf("wish> ");
            lineSize = getline(&line, &len, stdin);
            command(line);
        }
    }
    else if (argc == 2) // modo batch, recibe un archivo con instrucciones
    {
        
        FILE *file;
        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message) * sizeof(char));
            exit(1);
        }

        lineSize = getline(&line, &len, file);

        while (lineSize >= 0)
        {

            command(line);
            lineSize = getline(&line, &len, file);
        }
        exit(0);
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
}

void command(char *line)
{
    char *comandos;
    execAux = 0;
    execP = commandCount(line);
    int pids[execP];
    while ((comandos = strsep(&line, "&")) != NULL)
    {
        int countWords = countWord(comandos);
        char *words[countWords];
        int length = strlen(comandos);

        comandos[length] = '\0';
        for (int i = 0; i < length; i++)
        {
            if (comandos[i] == '\t' || comandos[i] == '\n')
                comandos[i] = ' ';
        }
        // Eliminar espacios del principio
        while (*comandos == ' ')
            comandos++;

        char *found;
        int i = 0;
        int aux = 0;
        while ((found = strsep(&comandos, " ")) != NULL)
        {
            if (strlen(found) > 0)
            {
                aux = 1;
                words[i++] = found;
            }
        }

        if (aux == 1)
        {
            words[i] = NULL;
            int redir = find(words, i);

            if (execP > 1)
            {
                if ((pids[execAux++] = fork()) == 0)
                {
                    commandSelect(words, countWords, redir);
                    exit(0);
                }
            }
            else
            {
                commandSelect(words, countWords, redir);
            }
        }
    }
    int status;
    for (size_t i = 0; i < execAux; i++)
    {
        waitpid(pids[i], &status, 0);
    }
}

void commandSelect(char **words, int count, int redir)
{

    if (strcmp(words[0], "exit") == 0)
    {
        if (count > 1)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else
            exit(0);
    }
    else if (strcmp(words[0], "cd") == 0)
    {

        commandChangeDir(words);
    }
    else if (strcmp(words[0], "path") == 0)
    {

        pathAdd(words);
    }
    else
    {
        if (!(*words[0]))
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else
        {
            if (redir > 0)
            {
                rExecute(words, redir);
            }

            else
                runC(words);
        }
    }
}

void rExecute(char **words, int index)
{
    char **args = copy_command(0, index, words);
    if (words[index + 1] == NULL || words[index + 2] != NULL)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else
    {
        int fd = open(words[index + 1], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        int std_out = dup(STDOUT_FILENO);
        int std_err = dup(STDERR_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        runC(args);
        close(fd);
        dup2(std_out, STDOUT_FILENO);
        dup2(std_err, STDERR_FILENO);
    }
}

int find(char **words, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (strcmp(words[i], ">\0") == 0)
        {
            return i;
        }
    }
    return 0;
}

void commandChangeDir(char **words)
{
    if (words[1] != NULL && words[2] == NULL)
    {
        int cdSuccess = chdir(words[1]);
        if (cdSuccess == -1)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
    }
    else
        write(STDERR_FILENO, error_message, strlen(error_message));
    return;
}

void runC(char **words)
{
    int status = 1;

    for (int i = 0; i < pathLen; i++)
    {

        int fullPathLen = strlen(paths[i]) + strlen(words[0]) + 1;
        char auxpath[fullPathLen];
        strcpy(auxpath, paths[i]);
        strcat(auxpath, "/");
        strcat(auxpath, words[0]);
        if (access(auxpath, X_OK) == 0)
        {

            int rc = fork();
            if (rc == 0)
            {
                if (execv(auxpath, words) == -1)
                {
                    exit(1);
                }
                else
                {

                    exit(0);
                }
            }
            else
            {
                wait(&status);
                if (status == 0)
                {
                    break;
                }
            }
        }
        else
        {
            status = 1;
        }
    }

    if (status == 1)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

void pathAdd(char **words)
{
    if (paths != NULL)
        free(paths);
    paths = (char **)malloc(sizeof(char *));
    char *path_name = NULL;
    int index = 0;
    char **p = words;
    while (*(++p))
    {
        path_name = (char *)malloc(strlen(*p) * sizeof(char));
        stpcpy(path_name, *p);
        paths[index] = path_name;
        index++;
        paths = (char **)realloc(paths, (index + 1) * sizeof(char *));
    }
    paths[index] = NULL;
    pathLen = index;
}

char **copy_command(int start, int end, char **command)
{
    char **new_command = (char **)malloc((end - start + 1) * sizeof(char *));
    for (int i = 0; i < end; i++)
        new_command[i] = command[i];
    new_command[end] = NULL;
    return new_command;
}
//Cuenta las palabras en la línea ingresada
int countWord(char *line)
{
    int count = 1;
    int length = strlen(line);

    for (int i = 1; i < length; i++)
    {
        if (line[i] != ' ' && line[i - 1] == ' ')
        {
            count++;
        }
    }
    return count;
}

int commandCount(char *line)
{
    int count = 1;
    int length = strlen(line);

    for (int i = 1; i < length; i++)
    {
        if (line[i] != '&' && line[i - 1] == '&')
        {
            count++;
        }
    }
    return count;
}