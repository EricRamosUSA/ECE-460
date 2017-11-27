#copycat
Program that mimics the "cat" shell command
copycat concatenates and copies files from arguments in the forms:
  copycat [-b ###] [-o outfile] infile1 [...infile2...]
  copycat [-b ###] [-o outfile]
Where [...] denotes an optional argument
If no outfile is designated, the program will output to stdout
If no infile is designated, the program takes inputs from stdin
If a hyphen '-' is designated as an infile, the program will take inputs for stdin, then continues if more infiles were specified.
