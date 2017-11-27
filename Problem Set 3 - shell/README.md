Simple Version of Unix Shell <br />
Capable of launching one program at a time, with arguments, waiting for completion, and reporting the exit status and usage statistics. <br />
Each command must be in the form: <br />

command {argument {argument...} } {redirection_operation {redirection_operation...}} <br />

The following redirect arguments are supported: <br />

'<filename'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Open filename and redirect to stdin) <br />
'>filename'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Open/create/truncate filename and redirect to stdout) <br />
'2>filename'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Open/create/truncate filename and redirect to stderr) <br />
'>>filename'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Open/create/append filename and redirect to stdout) <br />
'2>>filename'&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Open/create/append filename and redirect to stderr) <br />

The shell forks and execs the commands with a clean file descriptor environment (only 0, 1, and 2 are open). <br />
This program can also be invoked as a shell interpreter (i.e., shell /tmp/myscript.sh). If this is done, the program will open and execute each line of the script as a command. Otherwise, the shell will read commands from stdin until EOF. <br />
The shell program will exit with status 0 if no errors were encountered and all commands were launched. Otherwise, it will return with a nonzero status. <br />
