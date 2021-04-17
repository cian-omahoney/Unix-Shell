/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 2: 	Implement a simple shell
	
	Project:	assign2_19351611		
	File Name:   	main.c
	Description: 	Simple UNIX shell program.
			Reads line from stdin and executes command.
			This file contains the main function 
			for 'assign2_19351611'.
       			It is responsible with calling functions 
			which each perform a single well-defined task.	
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	15/2/2021
	Version:     	1.0

======================================================================*/



/*======================================================================
Systems header files
======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	// For close(),
#include <string.h>	// For strcpr()
#include <signal.h>	// For signal(), SIGINT, SIGQUIT
#include <fcntl.h>	// For open(), dup2()
#include <errno.h>	// For perror(), errno

#include "config.h"
#include "header.h"



/*======================================================================
 * FUNCTION:	main()
 * ARGUMENTS:	None.
 * RETURNS:	Exit success or failure.
 * DESCRIPTION:	Initialised shell with welcome message and by
 * 		setting signal handling functions.
 *		Until EOF character received or some error occurs:
 *			> Print shell prompt.
 *			> Read line from stdin.
 *			> Check for redirect symbol.
 *				If found, redirect output to file.
 *			> Run command requested.
 *			> Prompt for next command.
 *====================================================================*/
int main(void)
{
	int debug = DEBUG;

	size_t buffer_size = MAX_BUFFER;// Maximum size of line allowed to be read from stdin by getline().
	char *cmd_line = NULL;		// Array to hold line read by getline().
	ssize_t length = 0;		// Length of line read from stdin by getline()
	char **command = NULL;		// Array of array pointer to hold parsed command line strings.
	char *filename;
	int f;				// File descriptor if file is opened.
	int save_out;			// To hold stdout file descriptor/ 


	// Upon startup of shell, print a welcome message and some inportant information:
	if(shell_startup() != P_SUCCESS)
	{
		fprintf(stderr,"main(): Error printing shell startup.\n");
	}


	// Set function to handle quit signal received:
	// If unable to set signal handler function, terminate with failure.
	if(signal(SIGQUIT, sig_handler) == SIG_ERR)
	{
		fprintf(stderr, "main(): An error occurred while setting a signal handler.\n");
		return EXIT_FAILURE;
	}
	

	// The following loop is responsible for the majority of the functionality of the shell:
	while(length != -1)
	{
		// Print shell prompt in command line:
		if(shell_prompt() == P_FAILURE)
		{
			fprintf(stderr, "main(): An error occured while printing shell prompt.\n");
		}
		

		// Set function to handle interrupt signal received before fork() called:
		// If unable to set signal handler function, terminate with failure.
		if(signal(SIGINT, sig_handler) == SIG_ERR)
		{
			fprintf(stderr, "main(): An error occurred while setting a signal handler.\n");
			return EXIT_FAILURE;
		}
		

		// Read in line from stdin:
		if((length = getline(&cmd_line,&buffer_size,stdin)) == -1)
		{
			// Continue to next iteration of while loop.
			// Loop condition will fail and program will terminate..
			continue;
		}	
		

		// Trim trailing new-line character from string read by getline():
		cmd_line[--length] = '\0';
		

		// Parse string read by getline() into two tokens:
		// 	Before redirect character '>',
		// 	and after redirect character '>'.
		// If redirect used, first token will be a command and arguments.
		// 		     second token will be a filename.
		// If redirect not used, second token will be NULL.
		cmd_line = strtok(cmd_line,">");
		filename = strtok(NULL,">");


		// Attempt to redirect standard output to file specified:
		if(redirect_stdout_to_file(filename, &f, &save_out) == FAILURE)
		{
			// If unable to redirect standard output, set filename to NULL as any open files
			// have been closed.
			fprintf(stderr, "main(): Failed to redirect stdout to file '%s'.\n", filename);
			filename = NULL;
		}
		

		// Attempt to parse command line before redirect character into an array of commands
		// and arguments.
		if(parse_cmd(cmd_line, &command) != SUCCESS)
		{
			fprintf(stderr, "main(): Failed to parse command line input.\n");
		}


		// If a command was entered in the command line:
		if(command[0] != NULL)
		{
			// First check if a builtint command was issued:

			// Check if 'logout' or 'exit' command was issued:
			// If it was, cause while loop condition to fail and process to terminate.
			if((strcmp(command[0], "logout") == 0) || (strcmp(command[0], "exit") == 0))
				length = -1;


			else if(help(command));
			else if(change_directory(command));


			// If a builtin command was not issued:
			else
			{
				// Execute command issued in command line:
				// If execute_command() returns a failure, terminate process with failure.
				if(execute_command(command) == FAILURE)
				{
					// Free memory allocated:
					free(command);

					// Close open files:
					if(filename != NULL)
					{
						close(save_out);
						close(f);
					}

					return EXIT_FAILURE;		
				}
			}
		}
		

		// If a file was specified in the command line and the stdout
		// was successfully redirected to it, need to close files:
		if(filename != NULL)
		{	
			// Return output to normal:
			dup2(save_out,1);
			
			// Close open files:
			if(debug) printf("Closing files...");
			close(save_out);
			close(f);
		}


		// Free memory allocated in loop.
		free(command);	
	}
	

	// Once EOF received, end process.	
	printf("\n%s: Logging out...\n", PACKAGE);


	return EXIT_SUCCESS;

}// End of 'main()'.
