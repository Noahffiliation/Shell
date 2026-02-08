#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_OUTPUT 4096

// Helper function to run a command in the shell and capture output
void run_tush(const char *input, char *output) {
  int pipe_in[2], pipe_out[2];
  pipe(pipe_in);
  pipe(pipe_out);

  pid_t pid = fork();
  if (pid == 0) {
    // Child process (tush)
    dup2(pipe_in[0], STDIN_FILENO);
    dup2(pipe_out[1], STDOUT_FILENO);
    dup2(pipe_out[1], STDERR_FILENO); // Capture stderr too

    close(pipe_in[0]);
    close(pipe_in[1]);
    close(pipe_out[0]);
    close(pipe_out[1]);

    execl("./tush", "./tush", NULL);
    perror("execl");
    exit(1);
  } else {
    // Parent process (test runner)
    close(pipe_in[0]);
    close(pipe_out[1]);

    write(pipe_in[1], input, strlen(input));
    close(pipe_in[1]);

    int total_read = 0;
    int n;
    while ((n = read(pipe_out[0], output + total_read,
                     MAX_OUTPUT - 1 - total_read)) > 0) {
      total_read += n;
    }
    if (n < 0) {
      perror("read");
    }
    // printf("DEBUG: total_read=%d\n", total_read);
    // printf("DEBUG: content='%s'\n", output);

    int status;
    wait(&status);
    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status) != 0) {
        // printf("DEBUG: Tush exited with status %d\n", WEXITSTATUS(status));
      }
    } else if (WIFSIGNALED(status)) {
      // printf("DEBUG: Tush killed by signal %d\n", WTERMSIG(status));
    }
  }
}

void assert_contains(const char *haystack, const char *needle,
                     const char *test_name) {
  if (strstr(haystack, needle) == NULL) {
    printf("FAIL: %s\nExpected to find '%s'\n", test_name, needle);
    printf("Actual Output:\n%s\n", haystack);
    exit(1);
  } else {
    printf("PASS: %s\n", test_name);
  }
}

void test_version() {
  char output[MAX_OUTPUT];
  run_tush("version\nexit\n", output);
  assert_contains(output, "nlindsey tu shell v0.9", "test_version");
}

void test_pwd() {
  char output[MAX_OUTPUT];
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  run_tush("pwd\nexit\n", output);
  assert_contains(output, cwd, "test_pwd");
}

void test_cd_error() {
  char output[MAX_OUTPUT];
  run_tush("cd /nonexistentpath\nexit\n", output);
  assert_contains(output, "no such file or directory", "test_cd_error");
}

void test_exit() {
  // Basic test to ensure it exits cleanly, implementation of run_tush relies on
  // exit working
  char output[MAX_OUTPUT];
  run_tush("exit\n", output);
  printf("PASS: test_exit\n");
}

void test_echo() {
  char output[MAX_OUTPUT];
  run_tush("echo hello\nexit\n", output);
  assert_contains(output, "hello", "test_echo");
}

void test_redirection_out() {
  char output[MAX_OUTPUT];
  run_tush("echo detected > test_out.txt\nexit\n", output);

  FILE *f = fopen("test_out.txt", "r");
  if (f) {
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    fclose(f);
    assert_contains(buffer, "detected", "test_redirection_out");
    remove("test_out.txt");
  } else {
    printf("FAIL: test_redirection_out (file not created)\n");
    exit(1);
  }
}

void test_redirection_in() {
  char output[MAX_OUTPUT];
  FILE *f = fopen("test_in.txt", "w");
  fprintf(f, "input_content");
  fclose(f);

  run_tush("cat < test_in.txt\nexit\n", output);
  assert_contains(output, "input_content", "test_redirection_in");
  remove("test_in.txt");
}

void test_invalid_command() {
  char output[MAX_OUTPUT];
  run_tush("nonexistentcommand\nexit\n", output);
  assert_contains(output, "Command not found in PATH", "test_invalid_command");
}

int main() {
  printf("Running tests...\n");
  test_version();
  test_pwd();
  test_cd_error();
  test_exit();
  test_echo();
  test_redirection_out();
  test_redirection_in();
  test_invalid_command();
  printf("All tests passed!\n");
  return 0;
}
