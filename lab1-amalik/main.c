// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] [-n NUMBER] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  bool time_travel = false;
  bool hasN = false;
  program_name = argv[0];
  int *N = NULL;
  //  char *c;
  int i = 0;
  for (;;)
    {
      switch (getopt (argc, argv, "ptn0123456789"))
	{
	case 'p': print_tree = true; i--; break;
	case 't': time_travel = true; i--; break;
	case 'n': hasN = true; break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': break;
	default: usage (); break;
	case -1: goto options_exhausted;
	}
      i++;
    }
 options_exhausted:;
  // There must be exactly one file argument.
  if (hasN)
    optind++;
  if (optind != (argc - 1))
    usage ();
  if (hasN)
    {
      for (i = 0; i < argc; i++)
	{
	  if (argv[i][0] == '-' && argv[i][strlen(argv[i])-1] == 'n')
	    break;
	}
      N = (int *)malloc(sizeof(int));
      *N = 0;
      unsigned int j;
      for (j = 0; j < strlen(argv[i+1]); j++)
	{
	  if (!(argv[i+1][j] >= '0' && argv[i+1][j] <= '9'))
	    usage();
	  *N += (argv[i+1][j] - '0') * pow(10, strlen(argv[i+1])-j-1);
	}
    }
  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
    command_stream_t command_stream =
      make_command_stream (get_next_byte, script_stream);

    command_t last_command = NULL;
    command_t command;
    if(time_travel)
      {
	DependencyGraph graph;
	createGraph(command_stream,&graph);
	int fstatus = 0;
	executeGraph(&graph, N);
	return fstatus;
      }
    else
      {
	while ((command = read_command_stream (command_stream)))
	  {
	    if (print_tree)
	      {
		printf ("# %d\n", command_number++);
		print_command (command);
	      }
	    else
	      {
		last_command = command;
		execute_command (command, time_travel);
	      }
	  }

	return print_tree || !last_command ? 0 : command_status (last_command);
      }
}
