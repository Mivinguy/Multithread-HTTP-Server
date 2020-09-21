Michael Nguyen

CSE130, Assignment 2

mivinguy@ucsc.edu

Compile the program by running make in the terminal.

------------------------------------
    
    The makefile has a lot of warnings that come up, that I didn't have when 
    compiling on my terminal with 
    "g++ httpserver.cpp -o -pthread httpserver"
    But please just ignore the warnings that come up, the program still compiles 
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

**Final Notes:**

The program will print out the hex to the log file for PUT requests, but it 
won't be zero padded or formatted as I didn't have time to get it to work.