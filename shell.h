#ifndef SHELL_HEADER
#define SHELL_HEADER

// prints who the user is
void whoami();

// prints the current directory the user is in
void ls();

// reprints the given string or line until symbol
void echo(String line);

// exits the program and prints bye bye
void exit();

// read inputs
char* read_line();

#endif /* SHELL_HEADER */
