#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "tokens.h"
#include <sys/wait.h>

#define CAP_STEP 256

int limit = 255;
char **previous;
int locationOut;
int locationIn;


char* read_line() {
  char *line = NULL;
    size_t buflen = 0;
    int f;
    f = getline(&line, &buflen, stdin);
    return line;
}

//method that redirects output
//takes in set of tokens, integer and has no return value
void output_redirection(char **tokens, int n){
tokens[n] = '\0';// assigning null value to the end of an array
pid_t child = fork();

if(child ==0){ 
  //closing stdout
   if(close(1) == -1){
     printf("Error in closing STDOUT");
      exit(1);
   }
  //Creating a file and truncating it if it exists
 int fd = open(tokens[n+1],O_WRONLY | O_CREAT | O_TRUNC, 0644);
 setenv("PATH=/bin/","HOME=/",0);
 if(execvp(tokens[0], tokens) < 0){
   printf("Error in the execvle");
    exit(1);
 }
 if(child > 0){
  wait(NULL);
 exit(1);
 }
  tokens[n] = ">";
  }
}

//method that redirects output
//takes in set of tokens, integer and has no return value
void input_redirection(char **tokens, int n){
  tokens[n] = '\0';
  pid_t child = fork();

  if(child < 0){
    printf("The child process was unsuccesful");
  } else if(child == 0){
    if(close(0) == -1){
      printf("Error in closing STDOUT");
      exit(1);
     }
      //Creating a file and truncating it if it exists
    int fd = open(tokens[n + 1], O_RDONLY);
    //assert(fd == 0);
    setenv("PATH=/bin/","HOME=/",0);
    if (execvp(tokens[0], tokens) < 0){
      printf("Error in the execlp");
      exit(1);
    }
    tokens[n] = "<";
    }
  if(child > 0){
  wait(NULL);
  }
}

//checks whether a certain character is present in the tokens, it is a helper to detect redirection statements  
//takes in tokens as input and returns an integer value
int check(char **tokens){
for (int i =0; tokens[i] != NULL; i++){
if(strcmp(tokens[i],"<") == 0){
locationIn = i;
return 1;
}
else if(strcmp(tokens[i],">") == 0) {
locationOut = i;
return 2;
   }
  }
return 0;
 }

//A method made to execute shell commands
//takes tokens as input and has no return statement as the function is void
void execute(char **tokens) {
  char  *current;
  char  *stringDir;
  char  *path;
  
  if (strcmp(tokens[0], "source") == 0) {
      FILE *file = fopen(tokens[1], "r");
      char line[256];
      while(fgets(line, sizeof(line), file) != NULL) {
        char **tokens2 = get_tokens(line);
        execute(tokens2);
      }
      return; 
  } else if(strcmp(tokens[0], "prev") == 0) {
    execute(previous);
    return;
  } else {
    previous = tokens;
  }
  if (strcmp(tokens[0], "exit") == 0) {
    printf("Bye bye.\n");
    free(previous);
    exit(0);
    return;
  } else if(strcmp(tokens[0], "help") == 0){
      printf("The built in commands in the program are as follows :\n 1.cd - Changes the directory that the user is in\n 2. source - Executes a script.\n 3. help - tells us what all the built in commands do\n 4. prev - prints and executes the previous command line\n");
  return;
  }
  else if(check(tokens) == 2){
   output_redirection(tokens, locationOut);
    return; 
  }
  else if(check(tokens) == 1){
    input_redirection(tokens,locationIn);
    return;
  }
  else if (strcmp(tokens[0], "cd") == 0) {
      char buf[256];
      if(tokens[1] == NULL) {
       chdir(getenv("HOME"));
       return;
      }
      getcwd(buf, 256);
      stringDir = strcat(buf, "/");
      path = strcat(stringDir, tokens[1]);
     chdir(path);
    return;
  }
  
  pid_t pid = fork();
  
  if (pid == 0) {
    execvp(tokens[0], tokens);
    printf("%s: command not found\n", tokens[0]);
    exit(0);
  } else if (pid > 0) {
    int status;
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  } else {
    perror("problem forking");
  }
}
    
 
//A method to check whether the tokens contains a pipe
// takes tokens as input and returns an integer value
int tokens_contain_pipe(char **tokens) {
  for (int i = 0; tokens[i] != NULL; i++) {
    if (strcmp(tokens[i], "|") == 0) {
      return 1;
    }
  }
 return 0;
}

//A method to execute the pipe command
// takes an integer and tokens as inputs and has no return statement as the function is void
void pipe_execute(int n, char **tokens[]) {
  int pipe_fds[2]; // 0 in read, 1 is write
  int i;
  int status;
  int in = 0;
  int in_copy = dup(STDIN_FILENO);
  for (i = 0; i < n - 1; i++) {
    pipe(pipe_fds);
    int child_pid = fork();
    if (child_pid == 0) {
      if (in != 0) {
        dup2(in, 0);
        close(in);
       }
      if (pipe_fds[1] != 1) {
        dup2(pipe_fds[1], 1);
        close(pipe_fds[1]);
      }
      execute(tokens[i]);
      //execvp(tokens[i][0], tokens[i]);
      exit(0);
     }
    do {
      waitpid(child_pid, &status, WUNTRACED);
     } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    close(in);
    in = pipe_fds[0];
    close(pipe_fds[1]);
   }
  if (in != 0) {
     dup2(in, 0);
     close(in);
   }
   execute(tokens[i]);
   close(0);
   dup2(in_copy, STDIN_FILENO);
   close(in_copy);
   //execvp(tokens[i][0], tokens[i]);
  return;
}

//the main method for shell
int main(int argc, char **argv) {
printf("Welcome to mini-shell.\n");// prints the welcome message
  while (1) {
    printf("shell $: ");
    char *commandLine = read_line();
    char *stringSplit = strtok(commandLine, ";");
    while (stringSplit != NULL) { // splits strings by sequencing first
      char **tokens = get_tokens(stringSplit);
      if (tokens_contain_pipe(tokens)) {
        char *pipeSplit = strtok(stringSplit, "|");
        char **allPipeTokens[CAP_STEP];
        int i = 0;
        while(pipeSplit != NULL) { // splits strings into commands seperated by pipe '|' and executes it via piping
          char **pipeTokens = get_tokens(pipeSplit);
          allPipeTokens[i] = pipeTokens;
          i++;
          pipeSplit = strtok(NULL, "|");
        }
        pipe_execute(i, allPipeTokens);
      } else if (tokens[0] != NULL) { // else executes commands normally
       execute(tokens);   
      }
      stringSplit = strtok(NULL, ";");
      }
    free(commandLine);
    free(stringSplit);
  }
  return 0;
}
