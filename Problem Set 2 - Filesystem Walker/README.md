Recursive filesystem walker <br />
Simplified version of find(1) <br />
Program is invoked as: ./walker options startingpath <br />

Options: <br />
-u user: Only list nodes owned by the user (specified by given name or by uid number) <br />
-m mtime: Only list nodes not modified in the specified mtime. If mtime is negative, walker will only list nodes that have been modified at most mtime seconds ago. <br />
-x : walker will only stay within the original mounted filesystem (i.e., not cross volumes). A message will be printed to stderr stating: "Note: not crossing mount point at [volume name]" <br />
-l target: Only symlinks that resolve to another node "target" will be listed. <br />
