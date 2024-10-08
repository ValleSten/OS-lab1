/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file(s)
 * you will need to modify the CMakeLists.txt to compile
 * your additional file(s).
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Using assert statements in your code is a great way to catch errors early and make debugging easier.
 * Think of them as mini self-checks that ensure your program behaves as expected.
 * By setting up these guardrails, you're creating a more robust and maintainable solution.
 * So go ahead, sprinkle some asserts in your code; they're your friends in disguise!
 *
 * All the best!
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// The <unistd.h> header is your gateway to the OS's process management facilities.
#include <unistd.h>
#include <sys/wait.h>
#include "parse.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

// Max size of working directory name
#define cwd_size 512

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
static int handle_builtin(Pgm *p);
static void handle_pipe(Command cmd);
static void execute_command(Command cmd);
void stripwhite(char *);
static void int_handler();

static char cwd[cwd_size];
static char *login;

int main(void)
{
  // Prevent zombies by removing terminated processes
  signal(SIGCHLD, SIG_IGN);

  for (;;)
  {

    // Gets the current working directory into cwd
    getcwd(cwd, cwd_size);
    login = getlogin();

    // Handle CTRL-C
    signal(SIGINT, int_handler);

    char *line;
    printf("%s:%s", login, cwd);
    line = readline("> ");

    // Handle CTRL-D
    // The test pass, but I'm not sure if it should handle if the line is not empty when CTRL-D is pressed
    if (line == NULL)
    {
      return 0;
    }

    // Remove leading and trailing whitespace from the line
    stripwhite(line);

    // If stripped line not blank
    if (*line)
    {
      add_history(line);

      Command cmd;
      if (parse(line, &cmd) == 1)
      {
        // Just prints cmd
        print_cmd(&cmd);

        Pgm *p = cmd.pgm;

        // Handle built-in function calls
        if (1 == handle_builtin(p))
        {
          continue;
        }

        // Create child process to execute system command
        int pid = fork();
        if (pid == -1)
        {
          fprintf(stderr, "fork error \n");
        }
        else if (pid == 0)
        {
          // Making sure that if this is a background process to not ignore ctrl c interupt
          if (cmd.background)
          {
            signal(SIGINT, SIG_IGN);
          }

          execute_command(cmd);
        }
        else
        {
          // Don't wait if background process.
          if (!cmd.background)
          {
            waitpid(pid, NULL, 0);
          }
        }
      }
      else
      {
        printf("Parse ERROR\n");
      }
    }

    // Clear memory
    free(line);
  }

  return 0;
}

/*
* Handles io redirection, dupes inputfile/outfile to
*
* standard input/output
*/
static void io_redirection(Command *cmd)
{
  if (cmd->rstdout)
  {
    int mode =  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;    //permission bits should be specified if using open to create file
    int flags = O_WRONLY| O_CREAT | O_TRUNC;  //flags used open-writeonly create if no file exists
    int fd_out = open(cmd->rstdout, flags,  mode); // or truncate outputfile if it is not empty
    if (fd_out == -1)                 //ret  value for failure
    {
      fprintf(stderr,"couldnt open output file");
      exit(1);
    }
    else
    {
      dup2(fd_out, STDOUT_FILENO);  //redirect stdout to given file for outpu
      close(fd_out);                
    }
  }
  if (cmd->rstdin)                  //check for input redirection
  {
    int fd_in = open(cmd->rstdin, O_RDONLY);   //open inputfile in readonly
    if (fd_in == -1)                  //return value of failure of open is -1
    {
      fprintf(stderr,"couldnt open input file");
      exit(1);                      //exit failure
    }
    else
    {
      dup2(fd_in, STDIN_FILENO);       //redirect stdin to inputfile
      close(fd_in);
    }
  }
}

/*
 * Executes any command.
 */
static void execute_command(Command cmd)
{
  Pgm *p = cmd.pgm;
  io_redirection(&cmd);
  if (p->next != NULL)
  {
    handle_pipe(cmd);
  }
  execvp(p->pgmlist[0], p->pgmlist);
}

/*
 * Executes piped commands.
 */
static void handle_pipe(Command cmd)
{
    Pgm *p = cmd.pgm;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "pipe error\n");
        return;
    }

    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork error\n");
        return;
    }

    if (pid == 0)
    {
      // Move to next program since parent will execute this program
      Pgm pgm = *p->next;
      cmd.pgm = p->next;
      if (p->next != NULL)
      {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe write end
        close(pipefd[1]);

        handle_pipe(cmd); // Recursive call for the next command
      }

      // Execute the current command
      execvp(pgm.pgmlist[0], pgm.pgmlist);
    }
    else
    {
      close(pipefd[1]);
      dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe read end
      close(pipefd[0]);

      waitpid(pid, NULL, 0); // Wait for the child process to finish

      // Execute the current command in the parent process
      execvp(p->pgmlist[0], p->pgmlist);
    }
}

/*
 * Handles the built-in functions "cd" and "exit".
 *
 * Returns 1 if the program was a built-in function, otherwise 0.
 */
static int handle_builtin(Pgm *p)
{
  if (strcmp(p->pgmlist[0], "cd") == 0)
  {
    if (p->pgmlist[1] != NULL)
    {
      int ret;
      ret = chdir(p->pgmlist[1]); // change the directory to the provided one
      if (ret != 0)               // if CHDIR fails print error
      {
        perror("Failed to change directory to specified path");
      }
    }
    else // if no path is provided, return to "HOME"
    {
      chdir(getenv("HOME"));
    }
    return 1;
  }
  else if (strcmp(p->pgmlist[0], "exit") == 0)
  {
    exit(0);
  }
  return 0;
}

/*
 * Handler for CTRL + C
 */
static void int_handler()
{
  printf("\n%s:%s> ", login, cwd);
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inspiration.
 */
static void print_cmd(Command *cmd_list)
{
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd_list->rstdin ? cmd_list->rstdin : "<none>");
  printf("stdout:     %s\n", cmd_list->rstdout ? cmd_list->rstdout : "<none>");
  printf("background: %s\n", cmd_list->background ? "true" : "false");
  printf("Pgms:\n");
  print_pgm(cmd_list->pgm);
  printf("------------------------------\n");
}

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
static void print_pgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    print_pgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/* Strip whitespace from the start and end of a string.
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  size_t i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}
