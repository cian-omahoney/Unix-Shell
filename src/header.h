/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 2: 	Implement a simple shell
	
	Project:	assign2_19351611		
	File Name:   	header.h
	Description: 	Simple UNIX shell program.
			Reads line from stdin and executes command.
			This file is a header file for assign2_19351611.
       			It contains constant definitions, type definitions,
			macro definitions and function prototypes 
			used by main.c and functions.c
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	15/2/2021
	Version:     	1.0

======================================================================*/



// Include guards for 'header.h'
#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED



/*======================================================================
 CONSTANT DEFINITIONS
======================================================================*/

#define LAST_UPDATED_ON 15	// Date of last code update.

#define DEBUG 0			// Debug argument:
				//	0: Print minimum
				//	1: Print additional information

#define TIME_BUF_SIZE 13	// Size of prompt time buffer.

#define MAX_BUFFER 256		// Maximum buffer used by getline.

#define STDOUT_FD 1		// Standard output file descriptor.

#define WIDTH 80		// Width of decorative printing.

#define PROMPT_CHARACTER '#'	// Special character to print in prompt.


/*======================================================================
 TYPE DEFINITIONS
======================================================================*/

typedef enum{FAILURE, SUCCESS} Operation;
typedef enum{P_FAILURE, P_SUCCESS} Print;
typedef enum{FALSE, TRUE} Boolean;


/*======================================================================
 MACRO DEFINITIONS
======================================================================*/


/*======================================================================
 FUNCTION PROTOTYPES
======================================================================*/

void sig_handler(int);
void parent_sig_handler(int);

Print shell_prompt(void);
Print shell_startup(void);
Print decorative_line(char);

Operation redirect_stdout_to_file(char*,int*,int*);
Operation parse_cmd(char*, char***);
Operation execute_command(char**);

Boolean change_directory(char **);
Boolean help(char **);

#endif
