covertBackdoor
==============

A covert backdoor application designed to be run on Linux based systems.

Author
======

Luke Queenan

Instructions
============

The application is broken into two parts, a client and a server. The server is the backdoor.

Server
======

To build and run the backdoor, execute the following commands:
	cd src/
	make server
	sudo ./server.out

The server will now just run on the system.

Client
======

To build the client, run the following commands from this directory:
	cd src/
	make client

The client application takes two flags, one optional and one required. The flags are described below.
	-f	The location of the configuration file containing the source 			(for spoofing) and destination (the backdoor) IP addresses and 			port numbers. The format of this file is "IP, portNumber". 			Comments are identified by a '#' at the start of the line.

	-c	The type of functionality you would like to use on the backdoor. 		This has three numerical options, 0 = execute a system call, 1 = 		retrieve a file, 2 = enable realtime key logger.

Once the commands have been supplied to the client program, it will prompt the user for a system command to execute or a file to retrieve if required. The program will display the results of the system call execution and the key logger to the screen along with saving the results to a file. The retrieve file command will result with a file being created using the same name on the local machine with all data being saved to it.