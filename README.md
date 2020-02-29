# ns_shell
Simple command-line interpreter written in C

This program was a university course assignment, with the following requirements:

1) To be able to request the execution of the program with an undetermined number of arguments, which could be, in this order of priority, a shell built-in, an executable file that is in the working directory, or an executable file that is found in the directories of the PATH environment variable.

2) Allow the use of pipes (**|**), spawning a process for each command, and the use of input and output redirecction (**<>**).

3) A command line terminated with **&** will not read the standard input and will output to /dev/null by default.

4) Command lines that don't end with **&**, or use redirection of input or output, can end in **\[** to indicate a *'document here'*. In this case, the command will use as standard input the lines written by the user until a line with only **\]** is read.

5) The ***var=value*** command should set a environment variable. For any variable the shell must replace **$var** with the value of the variable.

6) The **%** operator will store the command output into a environment variable, using the following syntax: ***x % cmd***.

7) The **$RESULT** environment variable will store the exit status returned by the last command.

8) The **ifok** command must execute its arguments as a simple command if the previous command completed its execution successfuly.

9) The **ifnot** command must execute its arguments as a simple command if the previous command completed its execution without success.

10) **True** and **false** commands must be analogous to UNIX commands.

The **run.sh** script was written as a launcher for commands used in the development process.
