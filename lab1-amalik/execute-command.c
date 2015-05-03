#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <fcntl.h>
#include <string.h>
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

int intersect(char **a, int nA, char **b, int nB)
{
  for (int i = 0; i < nA; i++)
    {
      for (int j = 0; j < nB; j++)
	{
	  if (strlen(a[i]) != strlen(b[j]))
	    continue;
	  for (int r = 0; r < strlen(a[i]); r++)
	    {
	      if (a[i][r] != b[j][r])
		continue;
	    }
	  return 1;
	}
    }
  return 0;
}

typedef struct {
  command_t command;
  int curbefore;
  int maxbefore;
  struct GraphNode** before;
  pid_t pid;
  int num;
  RLWL* words;
} GraphNode;

typedef struct
{
  int cursize;
  int maxsize;
  GraphNode** qu;
} Queue;
typedef struct {
  Queue* no_dependencies;
  Queue* dependencies;
} DependencyGraph;

void addGraphNode(GraphNode *a, GraphNode *b) // adds b to the before of a
{
  if (a->curbefore == a->maxbefore)
    {
      a->maxbefore *= 2;
      a->before = (GraphNode **)realloc(a->before, a->maxbefore * sizeof(GraphNode *));
    }
  a->before[a->curbefore] = b;
  a->curbefore++;
}
void addRead(RLWL* cur, char* g)
{
  if (cur->curRChars == cur->maxRChars)
    {
      cur->maxRChars *= 2;
      cur->ReadList = (char **)realloc(cur->ReadList, cur->maxRChars * sizeof(char *));
    }
  cur->ReadList[cur->curRChars] = g;
  cur->curRChars++;
}

void addWrite(RLWL* cur, char* g)
{
  if (cur->curWChars == cur->maxWChars)
    {
      cur->maxWChars *= 2;
      cur->WriteList = (char **)realloc(cur->WriteList, cur->maxWChars * sizeof(char *));
    }
  cur->WriteList[cur->curWChars] = g;
  cur->curRChars++;
}

RLWL* getLists(command_t c, RLWL* cur)
{
  if (c->type == SIMPLE_COMMAND)
    {
      int i = 1;
      while (c->u.word[i] != "\0")
	{
	  i++;
	  if (c->u.word[i][0] == '-')
	    continue;
	  addRead(cur, c->u.word[i])
	    }
      if (c->input != NULL)
	{
	  addRead(cur, c->input);
	}
      if (c->output != NULL)
	{
	  addWrite(cur, c->output);
	}
    }
  else if (c->type == SUBSHELL_COMMAND)
    {
      if (c->output != NULL)
	{
	  addWrite(cur, c->output)
	    }
    }
  else if (c->type == AND_COMMAND || c->type == OR_COMMAND || c->type == SEQUENCE_COMMAND || c->type == PIPE_COMMAND)
    {
      getLists(c->u.command[0], cur);
      getLists(c->u.command[1], cur);
    }
  return cur;
}
RLWL* getLists(command_t c)
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
  q->cursize = 0;
  q->maxsize = 10;
  q->qu = (GraphNode**)malloc(q->maxsize*sizeof(GraphNode*));
}

void pushq(Queue* q, GraphNode* gn)
{
  if (cursize == maxsize)
    {
      maxsize *= 2;
      q->qu = (GraphNode**)realloc(q->qu, q->maxsize*sizeof(GraphNode*));
    }
  q->qu[cursize] = gn;
  q->cursize++;
}

void popq(Queue* q, GraphNode* gn)
{
  if (q->cursize == 0)
    return;
  else q->cursize--;
}

DependencyGraph createGraph(command_stream_t str)
{
  int num = 0;
  command_t command;
  int maxsize = 10;
  GraphNode* list = (GraphNode *)malloc(maxsize*sizeof(GraphNode));
  while ((command = read_command_stream(str)))
    {
      if (num == maxsize)
	{
	  maxsize *= 2;
	  list = (GraphNode *)realloc(list, maxsize*sizeof(GraphNode));
	}
      list[num].num = num;
      list[num].words = getLists(command);
      list[num].pid = -1;
      list[num].command = command;
      list[num].curbefore = 0;
      list[num].maxbefore = 10;
      list[num].before = (GraphNode **)malloc(list[num].maxbefore*sizeof(GraphNode *));
      num++;
    }
  for (int i = 0; i < num; i++)
    {
      for (int j = i + 1; j < num; j++)
	{
	  if (intersect(list[i].words->ReadList, list[i].words->curRChars, list[j].words->WriteList, list[j].words->curWChars) ||
	      intersect(list[i].words->WriteList, list[i].words->curWChars, list[j].words->ReadList, list[j].words->curRChars) ||
	      intersect(list[i].words->WriteList, list[i].words->curWChars, list[j].words->WriteList, list[j].words->curWChars ))
	    {
	      addGraphNode(&list[j], &list[i]);
	    }
	}
    }
}

int executeNoDependencies(Queue* no_dependencies)
{
  for (int curnode = 0; curnode < no_dependencies->cursize; curnode++)
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
  for (int curnode = 0; curnode < dependencies->cursize; curnode++)
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
  executeNoDependencies(graph->no_dependencies);
  executeDependencies(graph->dependencies);
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
