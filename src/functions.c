/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 2: 	Implement a simple shell
	
	Project:	assign2_19351611		
	File Name:   	functions.c
	Description: 	Simple UNIX shell program.
			Reads line from stdin and executes command.
			This file contains a number of functions 
			required to implement 'assign2_19351611'.
			
			These include:
				> sig_hander()
				> parent_sig_handler()
				> shell_startup()
				> decorative_line()
				> shell_prompt()
				> redirect_stdout_to_files()
				> help()
				> change_directory()
				> parse_cmd()
				> execute_command()
				
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	15/2/2021
	Version:     	1.0

======================================================================*/



/*======================================================================
Systems header files
======================================================================*/
#include <config.h>
#include "header.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>	// For time(), localtime(), strftime()
#include <string.h>	// For strcmp(), strtok()
#include <unistd.h>	// For chdir(), close(), close()
#include <errno.h>	// For perror(), errno
#include <fcntl.h>	// For dup(), dup2(), open()
#include <signal.h>	// For SIGINT, SIGQUIT
#include <sys/wait.h>	// For wait()



/*======================================================================
 * FUNCTION:	sig_handler()
 * ARGUMENTS:	Integer number of signal received.
 * RETURNS:	Nothing.
 * DESCRIPTION: Function to handle signals received while fork() has
 * 		not been called.
 *		Upon receipt of interrupt signal: Ignore and print prompt
 *		Upon receipt of quit signal:	 Quit process.
 *====================================================================*/
void sig_handler(int signo)
{
	// If interrupt signal recieved:
	if(signo == SIGINT)
	{
		printf("\n");
		shell_prompt();
	}
	// If quit signal received:
	if(signo == SIGQUIT)
	{
		printf("\n%s: Quiting...\n", PACKAGE);
		fflush(stdout);
		exit(EXIT_SUCCESS);
	}
} // End of 'sig_handler()'.



/*======================================================================
 * FUNCTION:	parent_sig_handler()
 * ARGUMENTS:	Integer number of signal received.
 * RETURNS:	Nothing.
 * DESCRIPTION: Function to handle signals recieved in parent process
 * 		after fork() has been called.
 *		Upon receipt of interrupt signal: Ignore.
 *		Upon receipt of quit signal:	  Ignore
 *====================================================================*/
void parent_sig_handler(int signo)
{
	if(signo == SIGINT)
	{
		printf("\n");
		fflush(stdout);
	}
	if(signo == SIGQUIT)
	{
		printf("Quiting...\n");
		fflush(stdout);
	}
} // End of 'parent_sig_handler()'.



/*======================================================================
 * FUNCTION:	shell_startup()
 * ARGUMENTS:	None.
 * RETURNS:	Print success or print failure.
 * DESCRIPTION: Function to print a welcome message and
 * 		important information to user upon startup of shell.
 *====================================================================*/
Print shell_startup(void)
{
	// Print escape sequence to clear screen.
	printf("\033[H\033[J");

	decorative_line('=');

	printf("\t\tWelcome to '%s':\n", PACKAGE);
	printf("\t\tA simple shell\n");
	
	decorative_line('_');	
	
	printf("Module:\t\tCOMP20200 - UNIX Programming\nAssignment:\t2\n");
	printf("Author:\t\tCian O'Mahoney\nStudent Number:\t19351611\n");
	printf("Email:\t\t%s\nVersion:\t%s\n",PACKAGE_BUGREPORT, VERSION);
	printf("Last Updated:\t%dth March 2021\n",LAST_UPDATED_ON);
	
	// Get current time and date.
	time_t raw_time;
	struct tm *info;
	char time_str[MAX_BUFFER];

	time(&raw_time);

	if((info = localtime(&raw_time)) == NULL)
	{
		fprintf(stderr, "shell_prompt(): Unable to get local time.\n");
		return P_FAILURE;
	}

	//Print current time and date.
	strftime(time_str,MAX_BUFFER,"%d %B %H:%M %Z %Y", info);
	printf("\nCurrent Time and Date:\t%s.\n", time_str);
	
	decorative_line('_');
	decorative_line('=');

	printf("\n");

	// Return print successful.
	return P_SUCCESS;

}// End of 'shell_startup()'.



/*======================================================================
 * FUNCTION:	decorative_line()
 * ARGUMENTS:	Character to make line out of.
 * RETURNS:	Print.
 * DESCRIPTION: Funcion to print decorative line made of characters
 * 		of type passed to function.
 *====================================================================*/
Print decorative_line(char character)
{
	int i;	// For loop iterating.

	for(i=0; i<WIDTH; i++) printf("%c", character);	
	printf("\n");

	return P_SUCCESS;
}// End of 'decorative_line'.



/*======================================================================
 * FUNCTION:	shell_prompt()
 * ARGUMENTS:	None.
 * RETURNS:	Print success or print failure.
 * DESCRIPTION: Function to print shell prompt containing time, date,
 * 		current working directory and special character.
 *====================================================================*/
Print shell_prompt(void)
{
	time_t raw_time;
	struct tm *info;
	char time_str[TIME_BUF_SIZE];
	char current_directory[MAX_BUFFER];
	
	// Get current date and time.
	time(&raw_time);
	if((info = localtime(&raw_time)) == NULL)
	{
		fprintf(stderr, "shell_prompt(): Unable to get date and time.");
		return P_FAILURE;
	}
	
	// Print current date and time before directory and character.
	strftime(time_str,TIME_BUF_SIZE,"%d/%m %H:%M", info);
   	printf("[%s]", time_str);
	
	// Get current directory.
	if(getcwd(current_directory, MAX_BUFFER) == NULL)
	{
		perror("shell_prompt: getcwd()");
		return P_FAILURE;
	}

	printf(":%s", current_directory);
	printf("%c ", PROMPT_CHARACTER);

	// Flush stdout stream.
	fflush(stdout);

	return SUCCESS;

}// End of 'shell_prompt()'.



/*======================================================================
 * FUNCTION:	redirect_stdout_to_file()
 * ARGUMENTS:	filename: Name of time to print stdout to.
 * 		file_des: File descriptor of file to print to.
 * 		save_out: To hold file description of stdout to allow
 * 			  normal printing to stdout to resume once
 * 			  finished printing to file.
 * RETURNS:	Operation success or failure.
 * DESCRIPTION: Function to redirect stdout to a given file if 
 * 		redirection symbol '>' used in command line.
 *====================================================================*/
Operation redirect_stdout_to_file(char* filename, int* file_des, int* save_out)
{
	// If filename given:
	if(filename != NULL)
	{
		// Trim any leading space from filename.
		// Trailing spaces have already been removed by strtok().
		while(filename[0] == ' ')
			filename++;
		
		// Make duplicte of stdout file descriptor.
		if((*save_out = dup(STDOUT_FD)) == -1)
		{
			perror("redirect_stdout_to_file(): dup()");
			return FAILURE;
		}
		
		// Open file and assign file descriptor to file_des.
		if((*file_des = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
		{
			// Close open files:
			close(*save_out);
			perror("redirect_stdout_to_file(): open()");
			return FAILURE;
		}
		
		// Make open file descriptor duplicate of stdout file descriptor.
		// This will redirect everything printed to stdout to file.
		if(dup2(*file_des, STDOUT_FD) == -1)
		{
			// Close open files:
			close(*save_out);
			close(*file_des);
			perror("redirect_stdout_to_file(): dup2()");
			return FAILURE;
		}
	}

	return SUCCESS;

} // End of 'redirect_stdout_to_file()'.



/*======================================================================
 * FUNCTION:	change_directory()
 * ARGUMENTS:	Command line parsed into strings for each argument.
 * RETURNS:	Boolean true when user has called 'cd'.
 * 		Boolean false when user has not.
 * DESCRIPTION: Function to change working directory of shell when 
 * 		user calls builtin command 'cd'.
 *====================================================================*/
Boolean change_directory(char** cmd_line)
{
	// If user has not called command 'cd' in first argument:
	if(strcmp(cmd_line[0],"cd") != 0)
	     	return FALSE;
	else
	{
		// If no path specified or user specifies path '~':
		// Change to path given by environment variable 'HOME'.
		if((cmd_line[1] == NULL) || (strcmp(cmd_line[1],"~") == 0))
			if((cmd_line[1] = getenv("HOME")) == NULL)
			{
				fprintf(stderr,"cd: No home directory located.\n");
				return TRUE;
			}

		// Change directory to path supplied as second
		// argument in command line.
		// If unable to change to path given, print error.
		if(chdir(cmd_line[1]) == -1)
		{
			fprintf(stderr, "cd: %s: ", cmd_line[1]);
			perror(NULL);
		}

		return TRUE;
	}
} // End of 'change_directory()'.



/*======================================================================
 * FUNCTION:	help()
 * ARGUMENTS:	Command line parsed into strings for each argument.
 * RETURNS:	Boolean true when user has called 'help',
 * 		Boolean falue when user has not.
 * DESCRIPTION: Function to print help information when builtin command
 * 		'help' called.
 *====================================================================*/
Boolean help(char** cmd_line)
{
	// If user has not called 'help' 
	if(strcmp(cmd_line[0],"help") != 0)
		return FALSE;

	else
	{
		// Option argument entered with help command.	
		char* second_arg = cmd_line[1];
		
		// If no argument supplied with help:
		// Print general help message.
		if(second_arg == NULL)
		{
			printf("\nHELP INFORMATION:\n\n");
			printf("GENERAL USAGE:\tcommand [option(s)]... [filename(s)]\n");
			printf("For more information regarding user commands try 'man'.\n");
			printf("For more information regarding builtin commands try 'help [builtin command]'\n\n");
		}
		// If 'cd' argument supplied with help:
		// Print help message for builtin command 'cd'.
		else if(strcmp(second_arg,"cd") == 0)	
		{
			printf("\nCD:\t\tBUILTIN COMMAND\n\n");
			printf("NAME:\t\tcd\n");
			printf("DESCRIPTION:\tChange working directory.\n");
			printf("USAGE:\t\tcd [path]\n\n");
		}
		// If 'help' argument supplied with help:
		// Print help message for built in command 'help'.
		else if(strcmp(second_arg,"help") == 0)
		{
			printf("\nHELP:\t\tBUILTIN COMMAND\n\n");
			printf("NAME:\t\thelp\n");
			printf("DESCRIPTION:\tDispay information about builtin commands.\n");
			printf("USAGE:\t\thelp [builtin command]\n\n");
		}
		// If 'logout' argument supplied with help:
		// Print help message for built in command 'logout'.
		else if(strcmp(second_arg,"logout") == 0)
		{
			printf("\nLOGOUT:\t\tBUILTIN COMMAND\n\n");
			printf("NAME:\t\tlogout\n");
			printf("DESCRIPTION:\tLog out from current user session.\n");
			printf("USAGE:\t\tlogout\n\n");
		}
		// If 'exit' argument supplied with help:
		// Print help message for built in command 'exit'.
		else if(strcmp(second_arg,"exit") == 0)
		{
			printf("\nEXIT:\t\tBUILTIN COMMAND\n\n");
			printf("NAME:\t\texit\n");
			printf("DESCRIPTION:\tExit shell.\n");
			printf("USAGE:\t\texit\n\n");
		}
		// If argument supplied with help command, but argument does not match any command with information stored:
		// Offer alternative sources for information.
		else
		{
			printf("help: no help topics match '%s'.",second_arg);
		      	printf(" Try 'help help' or 'man -k %s' or 'info %s'.\n",second_arg,second_arg);
		}

		return TRUE;
	}
} // End of 'help()'.



/*======================================================================
 * FUNCTION:	parse_cmd()
 * ARGUMENTS:	Pointer to unparsed command line.
 * 		Pointer to array of array pointers, which will
 * 		hold parsed strings.
 * RETURNS:	Operation success or failure.
 * DESCRIPTION: Function to parse command line input into a number
 * 		of argument strings.
 *====================================================================*/
Operation parse_cmd(char* unparsed, char*** parsed)
{	
	int count = 0;			// To count number of tokens extracted from unparsed string.
	char* token = NULL;		// To temporarily hold token extracted from unparsed string.
	char** token_holder = NULL;	// Array to temporarily hold tokens parsed from command line string.
	
	// Allocate memory for first token in token_holder.
	if((token_holder = (char **)malloc(sizeof(char*))) == NULL)
	{	
		fprintf(stderr, "parse_cmd(): malloc: Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}
	
	// Extract first token from unparsed string, deliminated by a space character.
	token = strtok(unparsed, " ");
	
	// Extract any remaining tokens from unparsed string.
	while(token != NULL)
	{	
		// Put tokens into token_holder.
		token_holder[count] = token;
		count++;
		
		// Allocate memory for any further tokens extracted from unparsed string.
		if((token_holder = (char **)realloc(token_holder, (count + 1) * sizeof(char*))) == NULL)
		{
			fprintf(stderr, "parse_cmd(): realloc: Failed to reallocate memory.\n");
			free(token_holder);
			exit(EXIT_FAILURE);
		}

		// Extract another token from unparsed string, deliminated by a space character.
		token = strtok(NULL, " ");
	}

	// Add final NULL pointer token to token_holder, to ensure token_holder is NULL terminated.
	token_holder[count] = token;
	
	// Point 'parsed' pointer to token_holder memory address.
	*parsed = token_holder;
	
	return SUCCESS;

} // End of 'parse_cmd()'.



/*======================================================================
 * FUNCTION:	execute_command()
 * ARGUMENTS:	Array of array pointers, holding command line
 * 		arguments parsed as strings.
 * RETURNS:	Operation success if successfully executed command,
 * 		or if command not found.
 * 		Operation failure if some other error occured and 
 * 		command was not executed.
 * DESCRIPTION: Function to execute the program referred to in the
 * 		command argument as a child process.
 * 		The parent process will wait for the command finish
 * 		before returning.
 *====================================================================*/
Operation execute_command(char** command)
{
	int debug = DEBUG;

	pid_t child_pid;	// Child process identifier.
	int child_status;	// To hold child process exit status.

	// Fork process:
	child_pid = fork();

	// If fork() failed, return with failure:
	if(child_pid == -1)
	{
		perror("execute_command(): fork()");
		return FAILURE;
	}

	// In child process:
	if(child_pid == 0)
	{
		// Replace process with that specified in command line:
		execvp(command[0], command);

		// If unable to execute program called:
		// Terminate child process. 
		fprintf(stdout, "%s: command not found.\n", command[0]);
		exit(EXIT_SUCCESS);
	}

	// In parent process:
	else
	{
		// Set function to handle interrupt signal received by parent process:
		// If unable to set signal handler function, return with failure.
		if(signal(SIGINT, parent_sig_handler) == SIG_ERR)
		{
			fprintf(stderr, "execute_command(): signal(): An error occurred while setting a signal handler.\n");
			return FAILURE;
		}
		
		if(debug) fprintf(stdout ,"Parent waiting.\n");

		// Wait for child process to finish:
		wait(&child_status);
	}
		
	if(debug) fprintf(stdout, "Parent exiting.\n");
	
	return SUCCESS;

} // End of 'execute_command()'.
