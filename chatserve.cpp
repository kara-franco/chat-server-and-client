/**********************************************************************************************************
** Programming Assignment #1												                 chatserve.cpp
** Author: Kara Franco
** CS.372-400 Intro to Computer Networks
** Due Date: February 7, 2016
** Description: A program that demonstates a simple chat server that works on a set port number specified by 
** the user on the command line. Server waits for a client to join the same port and then connects a line to
** send messages back and forth.
** Source used as reference for the design of this program (other miscellanenous sources listed in code):
** http://beej.us/guide/bgnet/output/html/multipage/index.html
************************************************************************************************************/

// printf, sscanf
#include <stdio.h>
// malloc
#include <stdlib.h>
// socket(), inet_ntoa
#include <sys/socket.h>
//in_addr, inet_ntoa
#include <arpa/inet.h>
// strtok
#include <string.h>
// signal
#include <signal.h>
// time_t, time()
#include <time.h>
#include <unistd.h>

/*************************************** Global Variables ************************************************/
#define MESSAGE_MAX 500
#define MESSAGE_PENDING 1
#define HANDLE_MAX 10
// 180 seconds = 3 minutes
#define TIMEOUT 180

int serverSocket;
int clientSocket;
int port;
struct sockaddr_in serverAddr;
struct sockaddr_in clientAddr;
time_t lastMessage;
char* toQuit = "\\quit";
char* tempHandle;

/************************************ Function Declarations **********************************************/
int readPort(int argc, char** argv);											
int createSocket();																
int listenOnSocket();															
int clientConnected(unsigned int* clientLen);									
void setHandle(char* clientHandle);											
int messageClient(char* message, int messageLen);									
int sendMessage(char* clientHandle, char* message);									
int receiveMessage(char* clientHandle, char* receivedBuffer);						
int messageControl(char* clientHandle, char* message, char* receivedBuffer);		
void startChatServer(int argc, char** argv);										
void signalTimeout(int sig);													
void endConnection(int sig);													

/**************************************** Main Function *****************************************************/
int main(int argc, char** argv) {
	// initialize and set memory aside for the handle, messages and recieving buffer
	char* clientHandle;
	char* message;
	char* receivedBuffer;
	clientHandle = new char(HANDLE_MAX);
	tempHandle = new char(HANDLE_MAX);
	message = new char(MESSAGE_MAX);
	receivedBuffer = new char(MESSAGE_MAX);
	unsigned int clientLen;
	// source: http://www.cplusplus.com/reference/csignal/signal/
	// set up the signals for the timeout and end connection
	signal(SIGALRM, signalTimeout);
	signal(SIGINT, endConnection);
	// call startChatServer to start get the port, create the socket, and listen for clients
	startChatServer(argc, argv);

	// server is running
	printf("Waiting for clients to connect... (Press ctrl-c to quit out)\n");
	clientLen = sizeof(clientAddr);
	for (;;) {
		if (clientConnected(&clientLen) == 1) {
			//process this client
			printf("Connected to client %s\n", inet_ntoa(clientAddr.sin_addr));
			setHandle(clientHandle);
			//tell client their handle
			while (messageClient(clientHandle, strlen(clientHandle)) != 1) {
				printf("Error: Failed show client their handle. Trying again...\n");
				sleep(2);
			}
			//client gets to send the first message
			printf("Now chatting with client %s. Max character length is %d (Type \\quit in chat to end the connection)\n", clientHandle, MESSAGE_MAX);
			// when the client disconnects from the chat control, then let the server connect with another client
			messageControl(clientHandle, message, receivedBuffer);
			printf("Client has disconnected. Waiting for another client...\n");
		}
	}

	return 0;
}

/********************************************* Function Definitions *************************************************/

/********************************************************************************
** readPort() 
** Description: A bool function that reads in the port number from the command 
** line and sends an error if no port is found. Used in startCharServer().
** Parameters: server and port number 
** Output: 0 is error / 1 is success 
*******************************************************************************/
int readPort(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Error: No port entered.\n");
		return 0;
	}
	else {
		// take in the second argument as port number
		sscanf(argv[1], "%d", &port);
		return 1;
	}
}

/*****************************************************************************
** createSocket()
** Description: A bool function that creates the socket given by the parsed 
** information and specifies the current address data. Used in startChatServer()
** Parameters: none
** Output: 0 is error / 1 is success
******************************************************************************/
int createSocket() {
	// source: http://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming
	if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "Error: Problem creating the server socket.\n");
		return 0;
	}
	// use memset to point to the memory forthe address
	// below source: Beej's Guide
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	return 1;
}

/******************************************************************************
** listenOnSocket() 
** Description: A bool function that binds the socket to an address, then 
** waits for connections. Used in startChatServer(). 
** Parameters: none
** Output: 0 if problem with server or problem listening, otherwise 1 success
********************************************************************************/
int listenOnSocket() {
	//bind to a local address
	if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		fprintf(stderr, "Error: Problem binding the server socket.\n");
		return 0;
	}
	// source: http://www.linuxhowtos.org/C_C++/socket.htm
	// if listen returned -1
	if (listen(serverSocket, MESSAGE_PENDING) < 0) {
		fprintf(stderr, "Error: Problem in listening for incoming connections.\n");
		return 0;
	}

	return 1;
}

/**********************************************************************************
** startChatServer() 
** Description: A function that uses and monitors readPort, createSocket and 
** listenOnSocket and checks for errors. 
** Parameters: input from the command line
**********************************************************************************/
void startChatServer(int argc, char** argv) {
	if (readPort(argc, argv) == 0) {
		exit(0);
	}

	if (createSocket() == 0) {
		exit(0);
	}

	if (listenOnSocket() == 0) {
		exit(0);
	}

}

/************************************************************************************
** clientConnected() 
** Description: A bool function that monitors if a client has connected to the 
** server's port number. Used in Main.
** Parameter: memory put aside for the client address
** Output: 0 is error / 1 is success
**********************************************************************************/
int clientConnected(unsigned int* clientLen) {
	clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, clientLen);
	if (clientSocket < 0) {
		//no client connected
		return 0;
	}
	else {
		//client connected
		return 1;
	}
}

/**********************************************************************************
** setHandle() 
** Description: A function that sets the handle to be displayed at the beginning 
** of the connected client's messages; used in main funciton. 
** Parameters:  clientHandle
*********************************************************************************/
void setHandle(char* clientHandle) {
	int recVal = recv(clientSocket, tempHandle, MESSAGE_MAX, 0);
	// print clientHandle with ">"
	sprintf(clientHandle, "%s>", tempHandle);
	return;

}

/************************************************************************************
** messageClient() 
** Description: A bool function that sends the initial message to client, and returns 
** back whether or not the message was sent. Used in sendMessage().
** Parameter: server message and message length
** Output:  0 is error / 1 is success
************************************************************************************/
int messageClient(char* message, int messageLen) {
	if (send(clientSocket, message, messageLen, 0) < 0) {
		return 0;
	}
	else {
		return 1;
	}
}

/********************************************************************************
** messageControl() 
** Description: A function that controls the process of sending and receiving 
** messages until timeout or interruption from a signal. Called in main function.
** Parameters: clientHandle, the message and the return message buffer
** Output: 0 indicate the chat has ended.
*********************************************************************************/
int messageControl(char* clientHandle, char* message, char* receivedBuffer) {
	for (;;) {
		if (receiveMessage(clientHandle, receivedBuffer) == -1) { return 0; }
		if (sendMessage(clientHandle, message) == 0) {
			return 0;
		}
	}

	return 0;
}

/*********************************************************************************
** receiveMessage() 
** Description: A funtion that reads in the message sent from the client and 
** displays it to the server user. Used in messageControl().
** Parameters: clienthandle and the message buffer
** Output: -1 if client connection lost, 0 if message wasn't stored, 1 if the message received
*************************************************************************************/
int receiveMessage(char* clientHandle, char* receivedBuffer) {
	int recVal = recv(clientSocket, receivedBuffer, MESSAGE_MAX, 0);
	// if recVal is 0, then the connection to the client has been lost
	if (recVal == 0) {
		fprintf(stderr, "Error: Client has ended the connection\n");
		return -1;
	}
	// source: http://stackoverflow.com/questions/3091010/recv-socket-function-returning-data-with-length-as-0
	// if the message was not stored correctly
	else if (recVal == -1) {
		fprintf(stderr, "Error: Problem storing client message\n");
		return 0;
	}
	else {
		printf("%s %s\n", clientHandle, receivedBuffer);

		//clear received message
		memset(receivedBuffer, 0, MESSAGE_MAX);

		return 1;
	}
}

/***************************************************************************
** sendMessage() 
** Description: A function that reads in the servers (user) input and 
** sends it to the client, Used in messageControl. 
** Parameters: clienthable and the message read in from the server user
** Output: 0 if the client wants to quit
****************************************************************************/
int sendMessage(char* clientHandle, char* message) {
	printf("Server> ");
	lastMessage = time(NULL);
	fgets(message, MESSAGE_MAX, stdin);

	messageClient(message, strlen(message));
	// check if client wants to quit
	if (strncmp(message, toQuit, 5) == 0) {
		printf("Quitting out of chat...\n");
		close(clientSocket);
		return 0;
	}

	lastMessage = time(NULL);
	memset(message, 0, MESSAGE_MAX);

	return 1;
}

/************************************************************************
** signalTimout() 
** Description: A function that	makes the server to disconnect after 
** 3 minutes of inactivity, used in main functon.
** Parameter: signal 
**************************************************************************/
void signalTimeout(int sig) {
	if ((time(NULL) - lastMessage) > TIMEOUT) {
		printf("\n");
		close(clientSocket);
	}

	alarm(1);
}

/***********************************************************************************
** endConnection() 
** Description: A function that closes the connection between the client and server
** when the signal for CTRL^C is encountered; used in main function.
** Parameter: signal
*************************************************************************************/
void endConnection(int sig) {
	printf("Manually shutting down the server...\n");
	exit(0);
}
