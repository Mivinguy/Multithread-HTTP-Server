A multithreaded HTTP server that responds to multiple or single simple GET and PUT commands to read and write text files that are specifically named by 27 ASCII names. Stores files persistently in the directory that the server is in. Logging is an option where the server will store a record of each request, with header information and the file data dumped as hex. 

Compile the program by running make in the terminal.

------------------------------------
    
    The makefile has a lot of warnings that come up, that I didn't have when 
    compiling on my terminal with 
    "g++ httpserver.cpp -o -pthread httpserver"
    Ignore the warnings that come up, the program still compiles 
    and runs
    
------------------------------------

Run the program using the command line:

sudo ./httpserver 'IPaddress' 'port' 'threads' 'log'

Options to use:

    -N #, to choose how many threads are made
    
    -l 'filename', to have logging to a chosen filename
    
    -p #, choose what port

Example commands:

- sudo ./httpserver localhost -p 8080 -N 8 

- sudo ./httpserver localhost -p 8080 -N 8 -l logfile.txt

- sudo ./httpserver localhost -p 8080 -l logfile.txt

- sudo ./httpserver localhost

Interact with the server using a client (developed using curl).

For PUT requests, I used:

- curl localhost:'port' --upload-file 'filename'. 

Ex:

- curl localhost:8080 --upload-file ABCDEFabcdef012345XYZxyz-t1

For GET requests:

- curl localhost:'port' --request-target 'filename'

Ex:

- curl localhost:8080 --request-target ABCDEFabcdef012345XYZxyz-t1
