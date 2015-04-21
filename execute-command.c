// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}
void execute_simple(command_t c)
{
	int status;
	pid_t pd = fork();
	if(pd == 0)
	{
		execvp(c->u.word[0],c->u.word);
	}
	else
	{
		waitpid(pd,&status,0);
		c->status = status;
	}
}

void execute_andor(command_t c,bool time_travel)
{
	int status;
	pid_t pd = fork();
	if(pd == 0)
	{
		execute_command(c->u.command[0],time_travel);
		exit(c->u.command[0]->status);
	}
	else
	{
		waitpid(pd,&status,0);
		if((c->type == AND_COMMAND && WEXITSTATUS(status) == 0) 
			|| (c->type == OR_COMMAND && WEXITSTATUS(status)!= 0))
		{
			pid_t pd2 = fork();
			if(pd2 == 0)
			{
				execute_command(c->u.command[1],time_travel);
				exit(c->u.command[1]->status);
			}
			else
			{
				waitpid(pd2,&status,0);
				c->status = status;
			}
		}
		else 
			c->status = status;
	}
}

void execute_seq(command_t c, bool time_travel)
{
	int status;
	pid_t pd = fork();
	if(pd == 0)
	{
		execute_command(c->u.command[0],time_travel);
		exit(c->u.command[0]->status);
	}
	else 
	{
		pid_t pd2 = fork();
		if(pd2 == 0)
		{
			execute_command(c->u.command[1],time_travel);
			exit(c->u.command[1]->status);
		}
		else
		{
			waitpid(pd2,&status,0);
			c->status = status;
		}
	}
}

void
execute_command (command_t c, bool time_travel)
{
  time_travel = false;
  switch(c->type)
  {
	case SIMPLE_COMMAND:
			execute_simple(c); break;
	case OR_COMMAND:
	case AND_COMMAND:
			execute_andor(c,time_travel);	break;
	case SEQUENCE_COMMAND:
			execute_seq(c,time_travel);break;
	case PIPE_COMMAND: 	break;
	case SUBSHELL_COMMAND:	break;
  }
}
