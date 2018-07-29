// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdint.h>
#include <winsock.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <windows.h>
#include <typeinfo>
#include <assert.h>
#include <winerror.h>
#include <thread>

class CPipe {
private:
	HANDLE PipeHandle;
public:
	/* close pipe when CPipeClient expires */
	~CPipe() {
		CloseHandle(PipeHandle);
	}
	/* open pipe on instantiation */
	CPipe() {
		PipeHandle = CreateFile(TEXT("\\\\.\\pipe\\ConsolePipe"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
			NULL);
	}
	HANDLE Pipe() {
		return PipeHandle;
	}
};

class Student{
private:
	char Name[1024];
	int Age;
public:
	void Set_Age(int arg) {
		Age = arg;
	}
	int Get_Age() {
		return Age;
	}
};

/* message payload struct */
typedef struct {
	char CMD[1024];
	int Index;
	char Type[1024];
	char Data[1024];
} Payload;

/* thread fn for async data reading */
void ReadData(HANDLE Pipe) {
	char readBuffer[1024];
	/* perform blocking read from server */
	ReadFile(Pipe, &readBuffer, sizeof(readBuffer), NULL, NULL);
	printf("%s\n", readBuffer);
}

/* handles the various input formats*/
void HandleInputs(Payload *packet) {
	/* <add> and <size> expects no arguments */
	/* <delete> format: delete index*/
	if (strcmp(packet->CMD, "delete") == 0) {
		std::cin >> packet->Index;
	}
	/* <get> format: get type index  */
	else if (strcmp(packet->CMD, "get") == 0) {
		std::cin >> packet->Type >> packet->Index;
	}
	/* <edit> format: edit type index data */
	else if (strcmp(packet->CMD, "edit") == 0) {
		std::cin >> packet->Type >> packet->Index >>packet->Data;
	}
}

int main(void)
{
	/* pipe opened on creation */
	CPipe Client;
	Payload packet;

	if (Client.Pipe() != INVALID_HANDLE_VALUE) {
		while (std::cin >> packet.CMD) {
			if (strcmp(packet.CMD, "exit") == 0) { break; } /* Close program */
			HandleInputs(&packet);
			/* perform non-blocking write to server */
			WriteFile(Client.Pipe(), &packet, sizeof(packet), NULL, NULL);
			/* perform async read from server through thread */
			std::thread first(ReadData, Client.Pipe());
			/* reach code which needs value from server so ensure read thread is complete */
			first.join();
		}
	}
	return 0;
}