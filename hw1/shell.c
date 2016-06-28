#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#include "tokenizer.h"

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
	{cmd_pwd,"pwd","print the current working path"},
	{cmd_cd, "cd", "change the current working directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(struct tokens *tokens) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  exit(0);
}

/* print the current working path */
int cmd_pwd(struct tokens *tokens)
{
	char cwd[1024];
	if(getcwd(cwd,sizeof(cwd)))
	{
		printf("%s\n",cwd);
		return 1;
	}
	else
	{
		printf("%s","error");
		return 0;
	}
}

/*change the current working directory*/
int cmd_cd(struct tokens *tokens)
{
	char *b=tokens_get_token(tokens,1);
	struct stat sb;
	if(stat(b,&sb)==0 && S_ISDIR(sb.st_mode))
	{
		if(chdir(b)==0)
		{
			return 1;
		}
		else
		{
			printf("error");
		}
	}
	else
	{	
		printf("no such directory");
	}	
	return 1;	
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

/*find cmd in the path*/
void get_cmd(char* c,char* a)
{
	char *md=strrchr(c,'/');
	int length=strlen(md);
	int i=0;
	while(i<length-1)
	{
		a[i]=md[i+1];
		i++;
	}
}

int get_num_path(char *path)
{
	int num=0;
	int i=0;
	int len=strlen(path);
	while(i<len)
	{
		if(path[i]==':' || i==(len-1))
		{
			num++;
		}
		i++;
	}
	return num;
}

int main(int argc, char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      //fprintf(stdout, "This shell doesn't know how to run programs.\n");
		char *path=tokens_get_token(tokens,0);
		//structure that store the information of file and directory in the system
		struct stat sb;
		// whether the path is a regular file 
		if(stat(path,&sb)==0 && S_ISREG(sb.st_mode))
		{
			pid_t fpid;
			fpid=fork();
			if(fpid < 0)
			{
				printf("error in fork");
			}	
			else if(fpid==0)
			{
				size_t length=tokens_get_length(tokens);
				char *arg[length];
				get_cmd(path,arg[0]);
				size_t i=1;
				//get arguments of the cmd from input
				while(i<length)
				{
					arg[i]=tokens_get_token(tokens,i);
					i++;
				}
				//the last item of the array of the exec function should be NULL 
				arg[length]=NULL;
				execv(path,arg);
			}
			else
			{
				wait(NULL);
			}

		}
		else
		{
			//get $PATH of the system	
			char *pathbuf;
			size_t n;
			n=confstr(_CS_PATH,NULL,(size_t)0);
			pathbuf=malloc(n);
			if(pathbuf==NULL)
				abort();
			confstr(_CS_PATH,pathbuf,n);
			
			//transfer to array
			int len=strlen(pathbuf);
			char p[len];
			int i=0;
			while(i<len)
			{
				p[i]=pathbuf[i];
				i++;
			}
			
			//get the the number of path
			int num=get_num_path(pathbuf);
			printf("%d\n",num);
			
			//separate each path 
			char delims[]=":";
			char *result=NULL;
			char *path_array[num];
			i=0;
			result=strtok(p,delims);
			while(result!=NULL)
			{
				path_array[i]=result;
				printf("%s\n",path_array[i]);
				i++;
				result=strtok(NULL,delims);
			}
			
			//find the cmd in path
			i=0;
			int cmd_len=strlen(path);
			int exe=0;
			while(i<num)
			{
				char *path_cmd=malloc(strlen(path_array[i])+cmd_len+1);
				strcpy(path_cmd,path_array[i]);
				strcat(path_cmd,"/");
				strcat(path_cmd,path);
				//printf("%s\n",path_cmd);	

				if(stat(path_cmd,&sb)==0 && S_ISREG(sb.st_mode))
				{
					pid_t fpid;
					fpid=fork();
					if(fpid < 0)
					{
						printf("error in fork");
					}	
					else if(fpid==0)
					{
						size_t length=tokens_get_length(tokens);
						char *arg[length];
						get_cmd(path_cmd,arg[0]);
						size_t i=1;
						//get arguments of the cmd from input
						while(i<length)
						{
							arg[i]=tokens_get_token(tokens,i);
							i++;
						}
						//the last item of the array of the exec function should be NULL 
						arg[length]=NULL;
						execv(path_cmd,arg);
					}
					else
					{
						wait(NULL);
					}
					exe=1;
					break;
				}
	
				i++;
			}
			if(exe!=1)
			{
				printf("no such cmd");
			}
		}
	}

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
