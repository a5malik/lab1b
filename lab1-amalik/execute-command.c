#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <fcntl.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

typedef struct {
  char** ReadList;
  int curRChars;
  int maxRChars;
  char** WriteList;
  int curWChars;
  int maxWChars;
} RLWL;


typedef struct {
  command_t command;
  struct GraphNode** before;
  pid_t pid;
} GraphNode;

typedef struct
{
  //FIXME
} Queue;
typedef struct {
  Queue* no_dependencies;
  Queue* dependencies;
} DependencyGraph;

void addRead(RLWL* cur, char* g)
{
  if (cur->curRChars == cur->maxRChars)
    {
      cur->maxRChars *= 2;
      cur->ReadList
	}
}

RLWL getLists(command_t c, RLWL* cur)
{
  if (c->type == SIMPLE_COMMAND)
    {
      int i = 1;
      while (c->u.word[i] != "\0")
	{
	  i++;
	  if (c->u.word[i][0] == '-')
	    continue;
	  addRead(RLWL, c->u.word[i])
	    }
    }
}
RLWL getLists(command_t c)
{
  RLWL* g;
  g->curRChars = 0;
  g->curWChars = 0;
  g->maxWChars = 10;
  g->maxRChars = 10;
  g->ReadList = (char **)malloc(g->maxRChars * sizeof(char *));
  g->WriteList = (char **)malloc(g->maxWChars * sizeof(char *));
  return getLists(c, g);
}

void initQueue(Queue* q)
{
  //FIXME
}

void pushq(Queue* q, GraphNode* gn)
{
  //FIXME
}

void popq(Queue* q, GraphNode* gn)
{
  //FIXME
}

DependencyGraph createGraph(command_stream_t str)
{
  //FIXME
}

int executeNoDependencies(Queue* no_dependencies)
{
  for each GraphNode i in no_dependcies
    {
      pid_t pid = fork()
	if (pid == 0)
	  {
	    execute_command(i->command, true);
	    exit(0);
	  }
	else
	  {
	    i->pid = pid;
	  }
    }
}

int executeDependencies(Queue* no_dependencies)
{
  for each GraphNode i in dependencies list
    {
      int status;
      for each GraphNode j  in i->before
	{
	  waitpid(i - pid, &status, 0);
	}
      pid_t pid = fork();
      if (pid == 0)
	{
	  execute->command(i->command);
	  exit(0);
	}
      else
	{
	  i->pid = pid;
	}
    }
}
int executeGraph(DependencyGraph* graph)
{

}
int
command_status(command_t c)
{
  return c->status;
}
void execute_simple(command_t c)
{
  int status;
  int fd2, fd3;
  pid_t pd = fork();
  if (pd == 0)
    {
      if (c->input != NULL)
	{
	  //      fd = dup(0);
	  fd2 = open(c->input, O_RDONLY, 0644);
	  //printf("Input is %s, fd2 is %d", c->input, fd2);
	  if (dup2(fd2, 0)<0)
	    exit(1);
	}
      if (c->output != NULL)
	{
	  fd3 = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	  if (dup2(fd3, 1)<0)
	    exit(1);
	}
      if (c->u.word[0][0] == 'e' &&
	  c->u.word[0][1] == 'x' &&
	  c->u.word[0][2] == 'e' &&
	  c->u.word[0][3] == 'c' &&
	  c->u.word[0][4] == '\0')
	execvp(c->u.word[1], &(c->u.word[1]));
      else
	execvp(c->u.word[0], c->u.word);
    }
  else
    {
      waitpid(pd, &status, 0);
      c->status = WEXITSTATUS(status);

    }
}

void execute_andor(command_t c, bool time_travel)
{
  int status;
  pid_t pd = fork();
  if ((pd == 0))
    {
      execute_command(c->u.command[0], time_travel);
      int st = c->u.command[0]->status;
      if (st == 0)
	exit(0);
      else
	exit(1);
      //exit(c->u.command[0]->status);
    }
  else
    {
      waitpid(pd, &status, 0);
      int j = WEXITSTATUS(status);
      if ((c->type == AND_COMMAND && j == 0)
	  || (c->type == OR_COMMAND && j != 0))
	{
	  pid_t pd2 = fork();
	  if (pd2 == 0)
	    {
	      execute_command(c->u.command[1], time_travel);
	      int st = c->u.command[1]->status;
	      if (st == 0)
		exit(0);
	      else
		exit(1);
	      //exit(c->u.command[1]->status);
	    }
	  else
	    {
	      waitpid(pd2, &status, 0);
	      c->status = WEXITSTATUS(status);
	    }
	}
      else
	c->status = WEXITSTATUS(status);
    }
}

void execute_seq(command_t c, bool time_travel)
{

  pid_t pd = fork();
  if (pd == 0)
    {
      execute_command(c->u.command[0], time_travel);
      int st = c->u.command[0]->status;
      if (st == 0)
	exit(0);
      else
	exit(1);
    }
  else
    {
      int status;
      waitpid(pd, &status, 0);
      pid_t pd2 = fork();
      if (pd2 == 0)
	{
	  execute_command(c->u.command[1], time_travel);
	  int st = c->u.command[1]->status;
	  if (st == 0)
	    exit(0);
	  else
	    exit(1);
	  //exit(c->u.command[1]->status);
	}
      else
	{
	  waitpid(pd2, &status, 0);
	  c->status = WEXITSTATUS(status);
	}
    }
}

void execute_subshell(command_t c, bool time_travel)
{
  // pid_t firstPid = fork();
  //if (firstPid < 0)
  //error(1, errno, "fork failed");
  //if (firstPid == 0)
  //  {
  if (c->output != NULL)
    {
      int fd = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0644);
      dup2(fd, 1);
    }
  execute_command(c->u.subshell_command, time_travel);

  //  int st = c->u.subshell_command->status;
  //if (st == 0)
  //exit(0);
  //else
  //exit(1);
  // }
  // else
  // {
  //int status;
  //waitpid(firstPid, &status, 0);
  c->status = c->u.subshell_command->status;
  //}
}

void execute_pipe(command_t c, bool time_travel)
{
  int fd[2];
  pipe(fd);
  pid_t firstPid = fork();
  if (firstPid == 0) // first child
    {
      close(fd[1]);
      dup2(fd[0], 0);
      execute_command(c->u.command[1], time_travel);
      int st = c->u.command[1]->status;
      if (st == 0)
	exit(0);
      else
	exit(1);
    }
  else
    {
      pid_t secondPid = fork();
      if (secondPid == 0)
	{
	  close(fd[0]);
	  dup2(fd[1], 1);
	  execute_command(c->u.command[0], time_travel);
	  int st = c->u.command[0]->status;
	  if (st == 0)
	    exit(0);
	  else
	    exit(1);
	}
      else
	{
	  close(fd[0]);
	  close(fd[1]);
	  int status;
	  waitpid(-1, &status, 0);
	  waitpid(-1, &status, 0);
	  int estatus = WEXITSTATUS(status);
	  c->status = estatus;
	}
    }
}
void
execute_command(command_t c, bool time_travel)
{
  time_travel = false;
  switch (c->type)
    {
    case SIMPLE_COMMAND:
      execute_simple(c); break;
    case OR_COMMAND:
    case AND_COMMAND:
      execute_andor(c, time_travel);break;
    case SEQUENCE_COMMAND:
      execute_seq(c, time_travel); break;
    case PIPE_COMMAND:
      execute_pipe(c, time_travel); break;
    case SUBSHELL_COMMAND:
      execute_subshell(c, time_travel); break;
    }
}
