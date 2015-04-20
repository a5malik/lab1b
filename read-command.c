// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include <stdlib.h>
#include <error.h>
#include <stdio.h>
#include <ctype.h>
/* FIXME: You may need to add #include directives, macro definitions,
static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
complete the incomplete type declaration in command.h.  */
enum op
{
	SEQ,
	AND,
	OR,
	PIPE,
	RIGHT,
	LEFT,
};
enum command_type types[6] = { SEQUENCE_COMMAND, AND_COMMAND, OR_COMMAND, PIPE_COMMAND, SUBSHELL_COMMAND, SUBSHELL_COMMAND };
int priority[6] = { 1, 2, 2, 3, -1, 0 };
char* optosym[4] = { ";", "&&", "||", "|" };
struct oper
{
	enum op type;
};
void printError(int curline);
typedef struct {
	struct command **contents;
	int cursize;
	int maxSize;
} stackCommands;

void initStackc(stackCommands* sc)
{
	sc->maxSize = 10;
	sc->contents = (struct command **)malloc(sc->maxSize * sizeof(struct command *));
}
void popc(stackCommands* sc)
{
	sc->cursize -= 1;
}

void pushc(stackCommands* sc, struct command* c)
{
	if (sc->cursize == sc->maxSize)
	{
		sc->maxSize *= 2;
		sc->contents = (struct command **)realloc(sc->contents, sc->maxSize * sizeof(struct command *));
	}
	sc->contents[sc->cursize] = c;
	sc->cursize += 1;
}

struct command* topc(stackCommands* sc)
{
	return sc->contents[sc->cursize - 1];
}

void emptyc(stackCommands* sc)
{
	int i;
	for (i = 0; i < sc->cursize; i++)
		free(sc->contents[i]);
	free(sc->contents);
	sc->maxSize = 10;
	sc->cursize = 0;
	sc->contents = (struct command **)malloc(sc->maxSize * sizeof(struct command *));
}

int sizec(stackCommands* sc)
{
	return sc->cursize;
}
typedef struct {
	struct oper *contents;
	int cursize;
	int maxSize;
} stackOper;

void initStacko(stackOper* sc)
{
	sc->maxSize = 10;
	sc->cursize = 0;
	sc->contents = (struct oper *)malloc(sc->maxSize * sizeof(struct oper));
}
void popo(stackOper* sc)
{
	sc->cursize -= 1;
}

void pusho(stackOper* sc, struct oper c)
{
	if (sc->cursize == sc->maxSize)
	{
		sc->maxSize *= 2;
		sc->contents = (struct oper *)realloc(sc->contents, sc->maxSize * sizeof(struct oper));
	}
	sc->contents[sc->cursize] = c;
	sc->cursize += 1;
}

struct oper topo(stackOper* sc)
{
	return sc->contents[sc->cursize - 1];
}

void emptyo(stackOper* sc)
{
	free(sc->contents);
	sc->maxSize = 10;
	sc->cursize = 0;
	sc->contents = (struct oper *)malloc(sc->maxSize * sizeof(struct oper));
}

int sizeo(stackOper* sc)
{
	return sc->cursize;
}

struct commandNode
{
	struct command *data;
	struct commandNode *next;
};

struct command_stream {
	struct commandNode *head;
	struct commandNode *tail;
	struct commandNode* cursor;
};

void initStream(struct command_stream *cs)
{
	cs->head = NULL;
	cs->tail = NULL;
	cs->cursor = NULL;
}

void addWord(struct command** com, int* count, int* length, char *wod)
{
	if (*length > 0)
	{
		(*com)->u.word[*count] = (char*)malloc((*length + 1)*sizeof(char));
		int i;
		for (i = 0; i < *length; i++)
		{
			(*com)->u.word[*count][i] = wod[i];
		}
		(*com)->u.word[*count][*length] = '\0';
		*length = 0;
		*count = *count + 1;
	}
}
void insert(struct command_stream* cs, struct command *c)
{
	struct commandNode* nodec = (struct commandNode *)malloc(sizeof(struct commandNode));
	nodec->data = c;
	nodec->next = NULL;
	if (cs->head == NULL)
	{
		cs->head = nodec;
		cs->tail = nodec;
		cs->cursor = nodec;
	}
	else
	{
		cs->tail->next = nodec;
		cs->tail = nodec;
		cs->tail->next = NULL;
	}
}
stackCommands s;// = *spointer;
//stackOper* opointer = (stackOper *)malloc(sizeof(stackOper));
stackOper o;// = *opointer;

void add(struct oper op, char finish_rem, int curline);
void reset(struct command** com, char** word, int* length, int *count, int *csize, int *mlength, char sub);


bool isValidChar(char c)
{
	return (c == '!' || c == '%' || c == '+' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '^' || c == '_' ||
		(c >= '0' && c <= '9') || (tolower(c) >= 'a' && tolower(c) <= 'z') || c == '|' || c == '&' || c == '<' || c == '>' || c == ';' ||
		c == ' ' || c == '\n' || c == '\t' || c == ')' || c == '(' || c == '\\' || c == '#');
}

bool isvalid(char c)
{
	return (c == '!' || c == '%' || c == '+' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '^' || c == '_' ||
		(c >= '0' && c <= '9') || (tolower(c) >= 'a' && tolower(c) <= 'z'));

}
void reset(struct command** com, char** word, int* length, int *count, int* csize, int* mlength, char sub)
{
	if (sub == 'f')
	{
		(*com) = (struct command*)malloc(sizeof(struct command));
		(*com)->type = SIMPLE_COMMAND;
		(*com)->u.word = (char**)malloc(10 * sizeof(char*));
	}
	else if (sub == 't')
	{
		(*com) = topc(&s);
	}
	(*com)->input = (*com)->output = NULL;
	free((*word));
	*word = (char*)malloc(12 * sizeof(char));
	*length = 0;
	*count = 0;
	*csize = 10;
	*mlength = 12;
}

void skipws(int(*getbyte) (void *), void *arg, char* c, int* curline)
{
	while (*c == '\n' || *c == ' ' || *c == '#' || *c == '\\')
	{
		if (*c == '\\')
		{
			*c = (char)getbyte(arg);
			*c = (char)getbyte(arg);
		}
		else if (*c == '#')
		{
			while ((*c = (char)getbyte(arg)) >= 0 && *c != '\n');
		}
		else
		  {
		    if(*c == '\n')
		      *curline+=1;
		    *c = (char)getbyte(arg);
		  }
	}

}
command_stream_t
make_command_stream(int(*getbyte) (void *), void *arg)
{
  initStackc(&s);
  initStacko(&o);
  struct command_stream *command_stream_t = (struct command_stream *)malloc(sizeof(struct command_stream));
  initStream(command_stream_t);
  struct command*com = (struct command*)malloc(sizeof(struct command));
  com->type = SIMPLE_COMMAND;
  com->input = com->output = NULL;
  struct oper o;
  char c;
  bool extra = false;
  bool done = false;
  int csize = 10;
  com->u.word = (char**)malloc(10 * sizeof(char*));
  int count = 0;
  int length = 0;
  int mlength = 12;
  char* word = (char*)malloc(12 * sizeof(char));
  c = getbyte(arg);
  int curline = 1;
  skipws(getbyte, arg, &c, &curline);
  while (c >= 0)
    {
      done = false;
      if (!isValidChar(c))
	printError(curline);
      extra = false;
      if (isvalid(c))
	{
	  word[length] = c;
	  length++;
	  if (length >= mlength)
	    {
	      mlength += 5;
	      word = (char*)realloc(word, mlength*sizeof(char));
	    }
	}
      else
	{
	  if (c == '#')
	    {
	      while ((c = (char)getbyte(arg)) >= 0 && c != '\n');
	    }
	  else if (c == '\\')
	    {
	      while (c == '\\')
		{
		  c = (char)getbyte(arg);
		  c = (char)getbyte(arg);
		}
	      continue;
	    }
	  else if (c == ' ')
	    {
	      if (length > 0)
		{
		  addWord(&com, &count, &length, word);
		  if (count == csize - 1)
		    {
		      csize += 5;
		      com->u.word = (char**)realloc(com->u.word, csize*sizeof(int));
		    }
		}
	      else
		{
		  c = (char)getbyte(arg);
		  continue;
		}
	    }

	  else if (c == '|' || c == '&' || c == ';' || c == '\n' || c == '(' || c == ')')
	    {
	      /*if (com->type == SUBSHELL_COMMAND)
		{
		com = (struct command*)malloc(sizeof(struct command));
		com->type = SIMPLE_COMMAND;
		com->u.word = (char**)malloc(10 * sizeof(char*));
		com->input = com->output = NULL;
		count = 0;
		csize = 10;
		}
	      */
	      addWord(&com, &count, &length, word);
	      if (c == '|')
		{
		  c = (char)getbyte(arg);
		  if (c == '|')
		    {
		      o.type = OR;
		    }
		  else
		    {
		      o.type = PIPE;
		    }
		}
	      else if (c == '&')
		{
		  o.type = AND;
		  c = (char)getbyte(arg);
		  if (c != '&')
		    printError(curline);
		}
	      else if (c == ';')
		{
		  o.type = SEQ;
		  c = (char)getbyte(arg);
		}
	      else if (c == '(')
		{
		  o.type = LEFT;
		}
	      else if (c == ')')
		{
		  o.type = RIGHT;
		}
	      else if (c == '\n')
		{
		  curline += 1;
		  while ((c = (char)getbyte(arg)) == ' ');
		  if (c < 0) break;
		  while (c == '\\')
		    {
		      c = (char)getbyte(arg);
		      c = (char)getbyte(arg);
		    }
		  if (c == '#')
		    {
		      while ((c = getbyte(arg)) >= 0 && c != '\n');
		    }      
		  if (c != '\n' && (com->u.word != NULL || com->u.subshell_command != NULL))
		    {
		      o.type = SEQ;
		      extra = true;
		    }
		  else
		    {
		      extra = false;
		      if (count > 0)
			{
			  com->u.word[count] = '\0';
			  pushc(&s, com);
			}
		      o.type = SEQ;
		      add(o, 't', curline);
		      insert(command_stream_t, topc(&s));
		      popc(&s);
		      done = true;
		      reset(&com, &word, &length, &count, &csize, &mlength, 'f');
		      c = (char)getbyte(arg);
		      skipws(getbyte, arg, &c, &curline);
		      continue;
		    }
		}
	    
	  if (o.type != PIPE && o.type != SEQ)
	    {
	      c = (char)getbyte(arg);
	    }
	  if (count > 0)
	    {
	      com->u.word[count] = '\0';
	      pushc(&s, com);
	    }
	  add(o, 'f', curline);
	  if (o.type == RIGHT)
	    reset(&com, &word, &length, &count, &csize, &mlength, 't');
	  else
	    reset(&com, &word, &length, &count, &csize, &mlength, 'f');
	  if (o.type != RIGHT)
	    {
	      skipws(getbyte, arg, &c,&curline);
	    }
	  extra = true;
	}
      else if (c == '>' || c == '<')
	{
	  char* ptr;
	  int insize = 12;
	  ptr = (char*)malloc(insize * sizeof(char));
	  int i = 0;
	  char t = c;
	  while ((c = (char)getbyte(arg)) == ' ');
	  if (!isvalid(c))
	    printError(curline);
	  while ((c >= 0))
	    {
	      if (!isvalid(c))
		{
		  extra = true;
		  break;
		}
	      ptr[i] = c;
	      i++;
	      if (i == insize - 1)
		{
		  insize += 5;
		  ptr = (char*)realloc(ptr, insize*sizeof(char));
		}
	      c = (char)getbyte(arg);
	    }
	  ptr[i] = '\0';
	  if (t == '<')
	    com->input = ptr;
	  else com->output = ptr;
	}
    }

  if (!extra)
    c = (char)getbyte(arg);
}
if (!done)
  {
    addWord(&com, &count, &length, word);
    if (count > 0)
      {
	com->u.word[count] = '\0';
	pushc(&s, com);
      }
    o.type = SEQ;
    add(o, 't', curline);
    if (sizec(&s) != 1)
      printError(curline);
    insert(command_stream_t, topc(&s));
    popc(&s);
  }
return command_stream_t;

}

void add(struct oper op, char finish_rem, int curline)
{
	if (finish_rem == 'f')
	{
		if (op.type == LEFT)
			pusho(&o, op);
		else if (sizeo(&o) == 0)
			pusho(&o, op);
		else
		{
			while (sizeo(&o) > 0 && topo(&o).type != LEFT && priority[topo(&o).type] >= priority[op.type])
			{
				struct command* newcom = (struct command*)malloc(sizeof(struct command));
				newcom->input = newcom->output = NULL;
				newcom->type = types[topo(&o).type];
				if (sizec(&s) < 2)
					printError(curline);
				newcom->u.command[1] = topc(&s); popc(&s);
				newcom->u.command[0] = topc(&s); popc(&s);
				pushc(&s, newcom);
				popo(&o);
			}
			if (op.type == RIGHT && topo(&o).type == LEFT)
				popo(&o);
			else
				pusho(&o, op);
			if (op.type == RIGHT)
			{
				struct command* ncom = (struct command*)malloc(sizeof(struct command));
				ncom->input = ncom->output = NULL;
				ncom->type = SUBSHELL_COMMAND;
				ncom->u.subshell_command = topc(&s); popc(&s);
				pushc(&s, ncom);
			}
		}
	}
	else if (finish_rem == 't')
	{
		while (sizeo(&o) > 0)
		{
			struct command* newcom = (struct command*)malloc(sizeof(struct command));
			newcom->input = newcom->output = NULL;
			newcom->type = types[topo(&o).type];
			if (sizec(&s) < 2)
				printError(curline);
			newcom->u.command[1] = topc(&s); popc(&s);
			newcom->u.command[0] = topc(&s); popc(&s);
			pushc(&s, newcom);
			popo(&o);
		}

	}
}

void printError(int curline)
{
	fprintf(stderr, "%d: Errorrrrrrrrr", curline);
	exit(1);
}

command_t
read_command_stream(command_stream_t str)
{
	if (str->cursor == NULL)
		return NULL;
	struct commandNode* temp = str->cursor;
	str->cursor = str->cursor->next;
	return temp->data;
}

