// UCLA CS 111 Lab 1 command interface

#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the flag is set.  */
void execute_command (command_t, bool);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);

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
  int curbefore;
  int maxbefore;
   pid_t pid;
  int num;
  RLWL* words;
  GraphNode** before;
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

int executeGraph(DependencyGraph* graph);
void createGraph(command_stream_t str,DependencyGraph* graph);