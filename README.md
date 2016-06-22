# chat-server-and-client
A program that demonstrates a simple chat server and client.

This program implements the chat server in C++ and the chat client in C based on the resource Beej’s Guide to Network Programming. 
Full references can be found in the .c and .cpp files.

*** This program was tested and will run on flip.engr.oregonstate.edu port 22 ***

*** This program was tested on the same computer, using two flip windows ***

Compile:

- Unzip the three files from franco_project1.zip (makefile, chatclient.c and chatserve.cpp)

- Type “make all” into the command line (without the parentheses)

Run:

- In the directory that you compiled the three files type “./chatserve port#” without parentheses, and where port# is the port 
you wish to use. Example ./chatserve 50200

- Next, open a new window (instance) of putty, and login to the flip server, go into the same directory as the three files

- Type “./chatclient localhost port#” without parentheses, and where port# is the port of the server. Example ./chatclient localhost 50200

Control of Program:

- If no clients are there, the server will wait for them

- Once a client is run (by following the directions above) then the following occurs:

		The chat client will be prompted to choose a handle name, enter your name and press the enter key. Example handle: kara&gt;

		Once the message indicating you are connected appears, “You are connected! Max message length is 500 characters…”, 

		**you, the client will send the first message**

		The server can then send a message to the client

		This messaging pattern is followed until the either:

			The client ends the chat session, by typing “\quit”, without parentheses, or the client times out (after 3 minutes)

			The server ends the chat session by typing “\quit”, without parentheses (this will allow the server to chat with other clients) or by using Ctrl-C to end the entire program

- Once a client quits the chat, the server can remain on, waiting for more clients.
