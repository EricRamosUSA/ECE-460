Modified copycat to accept arguments in the form:

catgrepmore pattern infile1 [...infile2...]

For each infile listed, the file is opened but is written to a pipeline of "grep pattern" piped into "more". <br />
This program displays the lines of the file that match the pattern one page at a time. When the user exits the pager program, the next file is processed, if any. <br />
If the user sends a keyboard interrupt SIGINT, the total number of files and bytes processed is sent to stderr before exiting. <br />
