/**********************************************************************************************************
** Programming Assignment #1												                 chatclient.c
** Author: Kara Franco
** CS.372-400 Intro to Computer Networks 
** Due Date: February 7, 2016
** Description: A program that demonstrates a simple chat client that will connect with a chat server, 
** named chatserve, and allow the exchange of messages one at a time. Requires the user to input
** host and port # on command line and then choose a handle once the program begins.
** Source used as reference for the design of this program (other miscellanenous sources listed in code): 
http://beej.us/guide/bgnet/output/html/multipage/index.html
***********************************************************************************************************/

// printf, sscanf
#include <stdio.h>
// malloc
#include <stdlib.h>
// strtok
#include <string.h>
// socket()
#include <sys/socket.h>
//in_addr
#include <arpa/inet.h>
// signal
#include <signal.h>
// time_t, time()
#include <time.h>

/*************************************** Global Variables ************************************************/
#define HANDLE_MAX 10
#define MESSAGE_MAX 500
// 180 seconds = 3 minutes
#define TIMEOUT 180

char* tempHandle;
int serverSocket;
char* serverAddress;
struct sockaddr_in serverAddr;
int port;
char* message;
char* handle;
char* receiveBuffer;
time_t lastMessage;
char* toQuit = "\\quit";

/************************************ Function Declarations **********************************************/
int readPortAddress(int argc, char** argv);		
int createSocket();									
int connectToServer();								
int sendMessage();									
int receiveMesssage();								
int messageControl();									
void startContact(int argc, char** argv);			
void signalTimeout(int sig);						

/**************************************** Main Function *****************************************************/
int main(int argc, char** argv) {
	// set aside memory for the client handle
	handle = malloc(HANDLE_MAX * sizeof(char));
	// set aside memory for the message
	message = malloc(MESSAGE_MAX * sizeof(char));
	// set aside memory for the message to be recieved
	receiveBuffer = malloc(MESSAGE_MAX * sizeof(char));
	// signal for the timeout
	signal(SIGALRM, signalTimeout);					

	startContact(argc, argv);

	// set up the temporary handle 
	tempHandle = malloc(HANDLE_MAX * sizeof(char));
	printf("Please choose a handle (10 characters max)\n");
	// source: http://stackoverflow.com/questions/18426206/how-can-i-use-fgets-to-scan-from-the-keyboard-a-formatted-input-string
	fgets(tempHandle, HANDLE_MAX, stdin);
	strtok(tempHandle, "\n");

	if (send(serverSocket, tempHandle, strlen(tempHandle), 0) == -1) {
		fprintf(stderr, "Error: Handle could not be sent\n");
	}
	// call getHandle (function below) to retrieve handle from server
	if (getHandle(handle) == 0) {
		printf("Error: getHandle returned 0\n");
		return 0;
	}
	// talk to the server
	printf("You are connected! Max message length is %d characters (Type \\quit to end connection)\n", MESSAGE_MAX);
	messageControl();

	return 0;
}

/********************************************* Function Definitions *************************************************/

/******************************************************************************
** readPortAddress() 
** Description: A bool function that read the clients address and port number 
** from the command line and gives an error message if the address and/or 
** port number are not found; used in startContact().
** Parameters: address and port number from the command line 
** Output: 0 is error / 1 is success
******************************************************************************/
int readPortAddress(int argc, char** argv) {
	if (argc < 3) {
		printf("Error: address or port not found.\n");
		return 0;
	}
	// take in the third argument as port number
	sscanf(argv[2], "%d", &port);
	// take in the second arguement as server address
	serverAddress = argv[1];
	return 1;
}

/*****************************************************************************
** createSocket() 
** Description: A bool function that reads the socket and aquires the 
** relevant address info, also gives an error message if the socket cannot 
** be created, used in startContact().
** Parameters: none
** Output: 0 is error / 1 is success
*****************************************************************************/
int createSocket() {
	// source: http://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming
	if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "Problem creating the server socket.\n");
		return 0;
	}
	// use memset to point to the memory for the address
	// below source: Beej's Guide
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	return 1;
}

/*****************************************************************************
** connectToServer()
** Description: A bool function that runs the process to connect to the 
** chat server, which is used in startContact(). 
** Parameters: none
** Outpus: 0 is error / 1 is success 
****************************************************************************/
int connectToServer() {
	// if connect returns -1 then there was an error, else return a 1 for success
	if (connect(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		fprintf(stderr, "Problem connecting to server %s on port %d\n", serverAddress, port);
		return 0;
	}
	return 1;
}

/*****************************************************************************
** startContact() 
** Description: A function that uses the readPortAddress, createSocket, 
** and connectToServer functions to start the chat process. This function will
** check for errors. 
** Parameters: address and port number from the command line 
******************************************************************************/
void startContact(int argc, char** argv) {
	if (readPortAddress(argc, argv) == 0) {
		printf("Error: readPortAddress returned 0\n");
		exit(0);
	}

	if (createSocket() == 0) {
		printf("Error: Create socket returned 0\n");
		exit(0);
	}

	if (connectToServer() == 0) {
		exit(0);
	}
}

/******************************************************************************
** getHandle() 
** Description: A bool function that retrieves the handle from the server. The 
** handle retrieved is the server's handle name; used in Main Function.
** Parameters: handle variable (with amount of memory for handle
** Output: 0 is error / 1 is success 
*******************************************************************************/
int getHandle(char* handle) {
	int recvVal;
	while ((recvVal = recv(serverSocket, handle, HANDLE_MAX, 0)) == -1) {
		fprintf(stderr, "Problem retrieving the server handle. Retrying...\n");
	}
	if (recvVal == 0) {
		fprintf(stderr, "The server has closed the connection\n");
		return 0;
	}
	else {
		return 1;
	}
}

/********************************************************************************
** messageControl() 
** Description: A function that controls the process of sending and receiving 
** messages, called in main. Messages are passed until a timeout occurs or one 
** of the users closes the connection. 
** Parameters: none
** Output: 0 to indicate the chat has ended
********************************************************************************/
int messageControl() {
	lastMessage = time(NULL);
	// source: http://cboard.cprogramming.com/linux-programming/148316-using-alarm-other-signals.html
	alarm(180);
	for (;;) {
		sendMessage();
		// if server closed it's connection, end chat
		if (receiveMesssage() == -1) { return 0; }
	}

	return 0;
}

/**********************************************************************************
** signalTimeout() 
** Description: A function that controls the timeout for the chat program that 
** that happens at 3 minutes of no messages being sent.
** Parameters: signal
**********************************************************************************/
void signalTimeout(int sig) {
	if ((time(NULL) - lastMessage) > TIMEOUT) {
		// use newlines to make easier to read
		printf("\nTimed out. You are not connected to the server\n"); 
		exit(0);
	}

	alarm(1);
}

/*********************************************************************************
** sendMessage() 
** Description: A function that takes in the clients's input and sends it to the 
** chat server; used in messageControl(). This function also checks for when the 
** user wants to end the chat (the \quit option) 
** Parameters: none
** Output: exit 0 upon quitting, or error
*********************************************************************************/
int sendMessage() {
	printf("%s ", handle);
	fgets(message, MESSAGE_MAX, stdin);
	lastMessage = time(NULL);

	if (send(serverSocket, message, strlen(message), 0) == -1) {
		fprintf(stderr, "Error: Message could not be sent!\n");
	}
	// check if client wants to quit
	if (strncmp(message, toQuit, 5) == 0) {
		printf("Quitting chat...\n");
		exit(0);
	}

	// clear message memory 
	memset(message, 0, MESSAGE_MAX);

	return 0;
}

/*****************************************************************************************
** recieveMessage() 
** Description: A bool function that receives incoming messages from the chat server 
** and displays the message to the client; used in messageControl(). 
** Parameters: none
** OUtput: -1 if server connection lost, 0 if message wasn't stored, 1 if the message received
*******************************************************************************************/

int receiveMesssage() {
	int recVal = recv(serverSocket, receiveBuffer, MESSAGE_MAX, 0);
	// if recVal is 0, then the connection to the server has been lost
	if (recVal == 0) {
		fprintf(stderr, "Error: The server has closed the connection!\n");
		return -1;
	}
	// source: http://stackoverflow.com/questions/3091010/recv-socket-function-returning-data-with-length-as-0
	// if the message was not stored correctly
	else if (recVal == -1) {
		fprintf(stderr, "Error: Problem storing client message!\n");
		return 0;
	}
	else {
		printf("Server> %s\n", receiveBuffer);
		lastMessage = time(NULL);

		//clear received message
		memset(receiveBuffer, 0, MESSAGE_MAX);

		return 1;
	}
}