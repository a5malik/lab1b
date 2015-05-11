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

int intersect(char **a, int nA, char **b, int nB)
{
  int i, j,r;
  char same;
  for (i = 0; i < nA; i++)
    {
      for (j = 0; j < nB; j++)
	{
	  if (strlen(a[i]) != strlen(b[j]))
	    continue;
	  same = 't';
	  for ( r = 0; r < (int)strlen(a[i]); r++)
	    {
	      if (a[i][r] != b[j][r])
		{
		  same = 'f';
		  break;
		}
	    }
	  if(same == 'f')
	    continue;
	  else
	    return 1;
	}
    }
  return 0;
}



void addGraphNode(struct GraphNode *a, struct GraphNode *b) // adds b to the before of a
{
  if (a->curbefore == a->maxbefore)
    {
      a->maxbefore *= 2;
      a->before = (struct GraphNode **)realloc(a->before, a->maxbefore * sizeof(struct GraphNode *));
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
  cur->curWChars++;
}

RLWL* getLists2(command_t c, RLWL* cur)
{
  if (c->type == SIMPLE_COMMAND)
    {
      int i = 1;
      if (c->u.word[0][0] == 'e' &&
	    c->u.word[0][1] == 'x' &&
	    c->u.word[0][2] == 'e' &&
	    c->u.word[0][3] == 'c' &&
	  c->u.word[0][4] == '\0')
	i++;
      while (c->u.word[i] != '\0')
	{
	  if (c->u.word[i][0] != '-')
	    addRead(cur, c->u.word[i]);
	  i++;
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
	  addWrite(cur, c->output);
	}
    }
  else if (c->type == AND_COMMAND || c->type == OR_COMMAND || c->type == SEQUENCE_COMMAND || c->type == PIPE_COMMAND)
    {
      getLists2(c->u.command[0], cur);
      getLists2(c->u.command[1], cur);
    }
  return cur;
}
RLWL* getLists1(command_t c)
{
  RLWL* g = malloc(sizeof(RLWL));
  g->curRChars = 0;
  g->curWChars = 0;
  g->maxWChars = 10;
  g->maxRChars = 10;
  g->ReadList = (char **)malloc(g->maxRChars * sizeof(char *));
  g->WriteList = (char **)malloc(g->maxWChars * sizeof(char *));
  return getLists2(c, g);
}

void initQueue(Queue** q)
{
  //FIXME
  *q = (Queue*)malloc(sizeof(Queue));
  (*q)->cursize = 0;
  (*q)->maxsize = 10;
  (*q)->qu = (struct GraphNode**)malloc((*q)->maxsize*sizeof(struct GraphNode*));
}

void pushq(Queue* q, struct GraphNode* gn)
{
  if (q->cursize == q->maxsize)
    {
      q->maxsize *= 2;
      q->qu = (struct GraphNode**)realloc(q->qu, q->maxsize*sizeof(struct GraphNode*));
    }
  q->qu[q->cursize] = gn;
  q->cursize++;
}

void popq(Queue* q)
{
  if (q->cursize == 0)
    return;
  else q->cursize--;
}

void createGraph(command_stream_t str,DependencyGraph* graph)
{
  int num = 0;
  command_t command;
  int maxsize = 10;
  struct GraphNode* list = (struct GraphNode *)malloc(maxsize*sizeof(struct GraphNode));
  initQueue(&(graph->dependencies));
  initQueue(&(graph->no_dependencies));
  while ((command = read_command_stream(str)))
    {
      if (num == maxsize)
	{
	  maxsize *= 2;
	  list = (struct GraphNode *)realloc(list, maxsize*sizeof(struct GraphNode));
	}
      list[num].num = num;
      list[num].words = getLists1(command);
      list[num].pid = -1;
      list[num].command = command;
      list[num].curbefore = 0;
      list[num].maxbefore = 10;
      list[num].before = (struct GraphNode **)malloc(list[num].maxbefore*sizeof(struct GraphNode *));
      num++;
    }
  int i, j;
  for (i = 0; i < num; i++)
    {
      for (j = i + 1; j < num; j++)
	{
	  if (intersect(list[i].words->ReadList, list[i].words->curRChars, list[j].words->WriteList, list[j].words->curWChars) ||
	      intersect(list[i].words->WriteList, list[i].words->curWChars, list[j].words->ReadList, list[j].words->curRChars) ||
	      intersect(list[i].words->WriteList, list[i].words->curWChars, list[j].words->WriteList, list[j].words->curWChars ))
	    {
	      addGraphNode(&list[j], &list[i]);
	    }
	}
    }
  for( i = 0; i < num;i++)
    {
      if(list[i].curbefore == 0)
	pushq(graph->no_dependencies,&list[i]);
      else
	pushq(graph->dependencies,&list[i]);
    }
}


void executeNoDependencies(Queue* no_dependencies, int *curProcs, int *maxProcs)
{
  int curnode;
  for(curnode = 0;curnode < no_dependencies->cursize;curnode++)
    {
      if (maxProcs == NULL || (*curProcs < *maxProcs))
	{
	  pid_t pid = fork();
	  if(pid == 0)
	    {
	      execute_command(no_dependencies->qu[curnode]->command,false);
	      exit(0);
	    }
	  else
	    {
	      no_dependencies->qu[curnode]->pid = pid;
	    }
	  if (curProcs != NULL)
	    (*curProcs)++;
	}
      else
	{
	  execute_command(no_dependencies->qu[curnode]->command, false);
	}
    }
}

void executeDependencies(Queue* dependencies, int *curProcs, int *maxProcs)
{
  int status,curnode,i;
  for(curnode = 0;curnode < dependencies->cursize;curnode++)
    {
      for(i = 0; i < dependencies->qu[curnode]->curbefore;i++)
	{
	  if (dependencies->qu[curnode]->before[i]->pid == -1)
	    continue;
	  waitpid(dependencies->qu[curnode]->before[i]->pid,&status, 0);
	}
      if (maxProcs == NULL || (*curProcs < *maxProcs))
	{
	  pid_t pid = fork();
	  if(pid == 0)
	    {
	      execute_command(dependencies->qu[curnode]->command,false);
	      exit(0);
	    }
	  else
	    {
	      dependencies->qu[curnode]->pid = pid;
	    }
	  if (curProcs != NULL)
	    (*curProcs)++;
	}
      else
	{
	  execute_command(dependencies->qu[curnode]->command,false);
	}
    }
}
int executeGraph(DependencyGraph* graph, int *N)
{
  int *curProcs = NULL;
  int *maxProcs = NULL;
  if (N != NULL)
    {
      curProcs = (int *) malloc(sizeof(int));
      maxProcs = (int *) malloc(sizeof(int));
      *curProcs = 0;
      *maxProcs = *N;
    }
  executeNoDependencies(graph->no_dependencies, curProcs, maxProcs);
  executeDependencies(graph->dependencies, curProcs, maxProcs);
  return 0;
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
