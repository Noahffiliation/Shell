#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>

// Used to control input/output redirection
int in;
int out;

void pwd() {
    char cwd[100];
    if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
}

int get_num_args(char *options) {
    int count = 0;
    for (int i = 0; options[i] != '\0'; i++) if (options[i] == ' ') count++;
    // Add 1 more to count for the last argument after last space
    return ++count;
}

// Execute a command by prepending the PATH to the given command
void rel_path(char *cmd, char **args) {
    char *path = getenv("PATH");
    char *cpy = malloc(sizeof(char) * strlen(path));
    // Deep copy so we don't modify the PATH environment variable
    strcpy(cpy, path);
    char *full_path = malloc(sizeof(char) * 400);
    // Split PATH by : to get each variable
    char *token = strtok(cpy, ":");
    while (token != NULL) {
        strcpy(full_path, token);
        // Concatenate a / and the command to build the full executable path
        strcat(full_path, "/");
        strcat(full_path, cmd);
        execv(full_path, args);
        // Iterate to the next path in PATH
        token = strtok(NULL, ":");
    }
}

void cd(char* options) {
    int result = chdir(options);
    if (result != 0) {
        if (errno == ENOTDIR) printf("not a directory\n");
        else if (errno == EACCES) printf("permission denied\n");
        else if (errno == ENOENT) printf("no such file or directory\n");
    }
}

// Kills background processes
void cleanup() {
    int pid = 1;
    while (pid < 0) waitpid(-1, NULL, WNOHANG);
}

// TODO: Finish implementing pipes
void piping(char *cmd, char *options) {
    int pipefd[2] = {0, 0};
    printf("pipe: %d\n", pipe(pipefd));
    char *out = strstr(options, "|");
    *out= '\0';
    out = out+2;
    printf("cmd: %s\n", cmd);
    printf("out: %s\n", out);
    printf("options: %s\n", options);
    pid_t pid = fork();
    if (pid < 0) printf("Error forking\n");
    else if (pid == 0) {
        printf("CHILD\n");
        dup2(pipefd[1], STDOUT_FILENO);
        fprintf(stderr, "pipe[0]: %d\n", pipefd[0]);
        fprintf(stderr, "pipe[1]: %d\n", pipefd[1]);
        close(pipefd[1]);
        printf("HERE\n");
        close(pipefd[0]);

        int num_args = 0;
        if (options != NULL) {
            num_args = get_num_args(options);
        }
        // Args include executable command, its arguments, and '\0'
        char **args = malloc(sizeof(char*) * (num_args + 2));
        args[0] = cmd;
        // Iterate through any number of arguments
        args[1] = strtok(options, " ");
        for (int i = 2; i <= num_args; i++) args[i] = strtok(NULL, " ");
        args[num_args+1] = '\0';
        if (strcmp(args[num_args], "&") == 0) args[num_args] = NULL;
        // Check to see if command has the full path
        if (strstr(cmd, "/") == NULL) rel_path(cmd, args);
        else execv(cmd, args);
    } else {
        printf("PARENT\n");
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        int num_args = 0;
        if (options != NULL) {
            num_args = get_num_args(options);
        }
        // Args include executable command, its arguments, and '\0'
        char **args = malloc(sizeof(char*) * (num_args + 2));
        args[0] = cmd;
        // Iterate through any number of arguments
        args[1] = strtok(options, " ");
        printf("args[1]: %s\n", args[1]);
        for (int i = 2; i <= num_args; i++) args[i] = strtok(NULL, " ");
        args[num_args+1] = '\0';
        // Check to see if command has the full path
        printf("cmd: %s\n", cmd);
        if (strstr(cmd, "/") == NULL) rel_path(cmd, args);
        else execv(cmd, args);
    }
}

// Runs given commands with optional arguments, as well as controls redirection
void run_command(char *cmd, char *options, char *inptr, char* outptr) {
    pid_t pid = fork();
    if (pid < 0) printf("Error forking\n");
    else if (pid == 0) {
        // If redirecting input
        if (in) {
            int fdi;
            char *input;
            options = strtok(options, " ");
            
            char *ws = strstr(inptr, " ");
            if (ws != NULL) {
                *ws = '\0';
            }
            fdi = open(inptr, O_RDONLY);
            dup2(fdi, 0);
            close(fdi);
            in = 0;
        }

        // If redirecting output
        if (out) {
            char *output;
            options = strtok(options, " ");
            int fdo = creat(outptr, 0644);
            dup2(fdo, STDOUT_FILENO);
            close(fdo);
            out = 0;
        }

        // Get number of arguments for the command
        int num_args = 0;
        if (options != NULL) {
            num_args = get_num_args(options);
        }
        // Args include executable command, its arguments, and '\0'
        char **args = malloc(sizeof(char*) * (num_args + 2));
        args[0] = cmd;
        // Iterate through any number of arguments
        args[1] = strtok(options, " ");
        // printf("args[1]: %s\n", args[1]);
        for (int i = 2; i <= num_args; i++) args[i] = strtok(NULL, " ");
        args[num_args+1] = '\0';
        if (strcmp(args[num_args], "&") == 0) args[num_args] = NULL;
        // Check to see if command has the full path
        if (strstr(cmd, "/") == NULL) rel_path(cmd, args);
        else {
            int result = execv(cmd, args);
            if (result != 0) {
                if (errno == EACCES) printf("permission denied\n");
                else if (errno == ENOEXEC) printf("exec format error\n");
            }
        }
    } else {
        // Wait for background processes
        if (!(options != NULL && strstr(options, "&"))) {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

int main(int argc, char **argv) {
    // Kill any existing background processes
    signal(SIGCHLD, cleanup);
    char *cmd;
    char *options;
    char *line = NULL;
    // Run shell until user exits
    while (1) {
        size_t size;
        // Prompt for my shell
        printf("$ ");
        getline(&line, &size, stdin);
        // Separate executable command from arguments
        cmd = strtok(line, " \n");
        options = strtok(NULL, "\n");
        char *inptr;
        char *outptr;
        // Control for piping
        // if (options != NULL && strstr(options, "|") != NULL) {
        //     // printf("cmd: %s\n", cmd);
        //     // printf("options: %s\n", options);
        //     piping(cmd, options);
        // }
        // Check for input/output redirection
        if (options != NULL && strstr(options, ">") != NULL) {
            out = 1;
            outptr = strstr(options, ">");
            *outptr = '\0';
            outptr = outptr+2;
        }
        if (options != NULL && strstr(options, "<") != NULL) {
            in = 1;
            inptr = strstr(options, "<");
            *inptr = '\0';
            inptr = inptr+2;
        }
        if (strcmp(cmd, "exit") == 0) break;
        else if (strcmp(cmd, "version") == 0) printf("nlindsey tu shell v0.9\n");
        else if (strcmp(cmd, "pwd") == 0) pwd();
        else if (strcmp(cmd, "cd") == 0) cd(options);
        else run_command(cmd, options, inptr, outptr);
    }
    return 0;
}