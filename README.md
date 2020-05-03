# Shell
I implemented a shell in C that runs known commands with full and relative paths along with input and output redirection and background commands.

## Requirements
- dejagnu for `runtest`
```
sudo apt install dejagnu
```

## Build
```bash
$ make
```

## Run
```bash
$ ./tush
$ (Run commands here)
```

## Testing
Run tests to ensure proper functionality
```bash
$ runtest PROMPT=\\\$ NAME=your-name-here
```

## Notes
Currently, piping is not yet implemented, but there is starting code that is included in the source file.

Also, the `runtest` may appear paused on the `errors.exp` test, but it does continue running and passing other tests. The `tush.log` file records the test results if this happens.