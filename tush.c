#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Used to control input/output redirection
int in;
int out;

void pwd() {
  char cwd[100];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
    printf("%s\n", cwd);
}

int get_num_args(char *options) {
  int count = 0;
  for (int i = 0; options[i] != '\0'; i++)
    if (options[i] == ' ')
      count++;
  // Add 1 more to count for the last argument after last space
  return ++count;
}

// Execute a command by prepending the PATH to the given command
void rel_path(char *cmd, char **args) {
  char *path = getenv("PATH");
  if (path == NULL) {
    fprintf(stderr, "Error: PATH environment variable not set\n");
    return;
  }

  int PATH_MAX_VAR = 4096; // Rename to avoid macro conflict
  char full_path[PATH_MAX_VAR];
  char *path_copy = strdup(path);
  char *token, *saveptr;

  token = strtok_r(path_copy, ":", &saveptr);
  while (token != NULL) {
    if (realpath(token, full_path) != NULL) {
      strncat(full_path, "/", PATH_MAX_VAR - strlen(full_path) - 1);
      strncat(full_path, cmd, PATH_MAX_VAR - strlen(full_path) - 1);
      if (access(full_path, X_OK) == 0) {
        execvp(full_path, args);
        perror("execvp");
      }
    } else {
    }
    token = strtok_r(NULL, ":", &saveptr);
  }

  free(path_copy);
  fprintf(stderr, "Error: Command not found in PATH\n");
}

void cd(char *options) {
  char *path = options;
  if (path == NULL) {
    path = getenv("HOME");
  }
  if (path == NULL) {
    printf("cd: missing argument or HOME not set\n");
    return;
  }
  int result = chdir(path);
  if (result != 0) {
    if (errno == ENOTDIR)
      printf("not a directory\n");
    else if (errno == EACCES)
      printf("permission denied\n");
    else if (errno == ENOENT)
      printf("no such file or directory\n");
  }
}

// Kills background processes
void cleanup() {
  int pid = 1;
  while (pid < 0)
    waitpid(-1, NULL, WNOHANG);
}

void execute_cmd(char *cmd, char *options) {
  int num_args = 0;
  if (options != NULL) {
    num_args = get_num_args(options);
  }
  // Args include executable command, its arguments, and '\0'
  char **args = malloc(sizeof(char *) * (num_args + 2));
  args[0] = cmd;
  // Iterate through any number of arguments
  if (options != NULL) {
    char *saveptr;

    // args[1] = strtok_r(options, " ", &saveptr); // Redundant?
    // Note: logic in original code seems to call strtok, then strtok_r on
    // same string. Ensure consistency.
    char *token = strtok_r(options, " ", &saveptr);
    if (token)
      args[1] = token;

    for (int i = 2; i <= num_args; i++)
      args[i] = strtok_r(NULL, " ", &saveptr);
  }
  args[num_args + 1] = '\0';
  if (num_args > 0 && args[num_args] && strcmp(args[num_args], "&") == 0)
    args[num_args] = NULL;

  // Check to see if command has the full path
  if (strstr(cmd, "/") == NULL) {
    rel_path(cmd, args);
  } else {
    int result = execv(cmd, args);
    if (result != 0) {
      if (errno == EACCES)
        printf("permission denied\n");
      else if (errno == ENOEXEC)
        printf("exec format error\n");
    }
  }
  free(args);
}

// Runs given commands with optional arguments, as well as controls
// redirection
void run_command(char *cmd, char *options, char *inptr, char *outptr) {
  if (cmd == NULL)
    return;
  pid_t pid = fork();
  if (pid < 0)
    printf("Error forking\n");
  else if (pid == 0) {
    // If redirecting input
    if (in && inptr != NULL) {
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
    if (out && outptr != NULL) {
      char *output;
      options = strtok(options, " ");
      int fdo = creat(outptr, 0644);
      dup2(fdo, STDOUT_FILENO);
      close(fdo);
      out = 0;
    }

    execute_cmd(cmd, options);
  } else {
    // Wait for background processes
    if (!(options != NULL && strstr(options, "&"))) {
      int status;
      waitpid(pid, &status, 0);
    }
  }
}

int main(int argc, char **argv) {
  setbuf(stdout, NULL);
  // Kill any existing background processes
  signal(SIGCHLD, cleanup);
  char *cmd = "";
  char *options;
  char *line = NULL;
  size_t size = 0;
  // Run shell until user exits
  while (1) {
    in = 0;
    out = 0;
    // Prompt for my shell
    printf("$ ");
    if (getline(&line, &size, stdin) == -1) {
      break;
    }
    // Separate executable command from arguments
    if (line != NULL && *line != '\0') {
      cmd = strtok(line, " \n\r");
      // Use cmd safely here
    } else {
      // Handle the case where line is a null pointer or an empty string
      // (e.g., print an error message or assign a default value to cmd)
      cmd = "";
    }
    options = strtok(NULL, "\n");
    char *inptr = NULL;
    char *outptr = NULL;
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
      outptr = outptr + 2;
    }
    if (options != NULL && strstr(options, "<") != NULL) {
      in = 1;
      inptr = strstr(options, "<");
      *inptr = '\0';
      inptr = inptr + 2;
    }
    if (strcmp(cmd, "exit") == 0)
      break;
    else if (strcmp(cmd, "version") == 0)
      printf("nlindsey tu shell v0.9\n");
    else if (strcmp(cmd, "pwd") == 0)
      pwd();
    else if (strcmp(cmd, "cd") == 0)
      cd(options);
    else
      run_command(cmd, options, inptr, outptr);
  }

  free(line);
  return 0;
}
