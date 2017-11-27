Program that mimics the "cat" shell command <br />
copycat concatenates and copies files from arguments in the forms: <br />
  copycat [-b ###] [-o outfile] infile1 [...infile2...] <br />
  copycat [-b ###] [-o outfile] <br />
Where [...] denotes an optional argument <br />
If no outfile is designated, the program will output to stdout <br />
If no infile is designated, the program takes inputs from stdin <br />
If a hyphen '-' is designated as an infile, the program will take inputs for stdin, then continues if more infiles were specified. <br />
