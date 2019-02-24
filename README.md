# Shell
I implemented a shell in C that runs known commands with full and relative paths along with input and output redirection and background commands.

## Build
### Compile
```bash
>>>gcc -o tush tush.c
```
### Run
```bash
>>>./tush
$ (Run commands here)
```

### Run tests
Run tests to ensure proper functionality
```bash
>>>runtest PROMPT=\\\$ NAME=nlindsey
```

## Notes
Currently, piping is not yet implemented, but there is starting code that is included in the source file.

Also, the `runtest` may appear paused on the `errors.exp` test, but it does continue running and passing other tests. The `tush.log` file records the test results if this happens.