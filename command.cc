
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
	if (_numberOfAvailableArguments == _numberOfArguments + 1)
	{
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **)realloc(_arguments,
									  _numberOfAvailableArguments * sizeof(char *));
	}

	_arguments[_numberOfArguments] = argument;

	// Add NULL argument at the end
	_arguments[_numberOfArguments + 1] = NULL;

	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
	if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
	{
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
													_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	}

	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			free(_simpleCommands[i]->_arguments[j]);
		}

		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}

	if (_outFile)
	{
		free(_outFile);
	}

	if (_inputFile)
	{
		free(_inputFile);
	}

	if (_errFile)
	{
		free(_errFile);
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		printf("  %-3d ", i);
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
		}
	}

	printf("\n\n");
	printf("  Output       Input        Error        Background        Append\n");
	printf("  ------------ ------------ ------------ ------------ ------------\n");
	printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
		   _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
		   _background ? "YES" : "NO",
		   _append ? "YES" : "NO");
	printf("\n\n");
}

void Command::execute()
{
	// Don't do anything if there are no simple commands
	if (_numberOfSimpleCommands == 0)
	{
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();

	// execution
	int defaultin = dup(0);	 // Input:    defaultin
	int defaultout = dup(1); // Output:   file
	int defaulterr = dup(2); // Error:    defaulterr

	int infd = dup(0), outfd = dup(1), errfd = dup(2);
	// Create file descriptor

	if (_currentCommand._inputFile)
	{
		infd = open(_inputFile, O_RDONLY, 0666);
	}

	if (_currentCommand._errFile)
	{
		int flags = _currentCommand._append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC;
		errfd = open(_currentCommand._errFile, flags, 0666);
	}

	// Redirect input
	if (infd)
	{
		dup2(infd, 0);
		close(infd);
	}

	// Redirect err
	if (errfd)
	{
		dup2(errfd, 2);
		close(errfd);
	}

	int pid;
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		dup2(infd, 0);
		if (i == _numberOfSimpleCommands - 1)
		{

			if (_currentCommand._outFile)
			{
				int flags = _currentCommand._append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC;
				outfd = open(_currentCommand._outFile, flags, 0666);
			}
			else if (_currentCommand._errFile)
			{

				
				int flags = _currentCommand._append ? O_WRONLY | O_CREAT | O_APPEND : O_WRONLY | O_CREAT | O_TRUNC;
				outfd = open(_currentCommand._errFile, flags, 0666);
			}
			else
			{
				outfd = defaultout;
			}
		}
		else // piping
		{
			int fdpipe[2];
			if (pipe(fdpipe) == -1)
			{
				perror("error");
				exit(2);
			}
			outfd = fdpipe[1];
			infd = fdpipe[0];
		}
		dup2(outfd, 1);
		close(outfd);
		// Create new process for command
		const char *command_name = _simpleCommands[i]->_arguments[0];
		pid = fork();
		if (pid == -1)
		{
			perror("error\n");
			exit(2);
		}

		if (pid == 0)
		{
			// Child

			// close file descriptors that are not needed
			close(outfd);
			close(infd);
			close(errfd);
			close(defaultin);
			close(defaultout);
			close(defaulterr);

			execvp(command_name, _simpleCommands[i]->_arguments);

			// exec() is not suppose to return, something went wrong
			perror("error");
			exit(2);
		}
	}

	dup2(defaultin, 0);
	dup2(defaultout, 1);
	dup2(defaulterr, 2);

	// // Close file descriptors that are not needed
	close(outfd);
	close(infd);
	close(errfd);
	close(defaultin);
	close(defaultout);
	close(defaulterr);

	// if process is not a background process
	if (_currentCommand._background == 0)
	{
		waitpid(pid, 0, 0);
	}
	// Clear to prepare for next command
	clear();

	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse(void);

int main()
{
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}
