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
void HandleInputs(Payload *Packet) {
	/* <add> and <size> expects no arguments */
	/* <delete> format: delete index*/
	if (strcmp(Packet->CMD, "delete") == 0) {
		std::cin >> Packet->Index;
	}
	/* <get> format: get type index  */
	else if (strcmp(Packet->CMD, "get") == 0) {
		std::cin >> Packet->Type >> Packet->Index;
	}
	/* <edit> format: edit type index data */
	else if (strcmp(Packet->CMD, "edit") == 0) {
		std::cin >> Packet->Type >> Packet->Index >> Packet->Data;
	}
}

int main(void)
{
	/* pipe opened on creation */
	CPipe Client;
	Payload Packet;

	/* continue if pipe is open */
	if (Client.Pipe() != INVALID_HANDLE_VALUE) {
		while (std::cin >> Packet.CMD) {
			if (strcmp(Packet.CMD, "exit") == 0) { break; } /* Close program */
			HandleInputs(&Packet);
			/* perform non-blocking write to server */
			WriteFile(Client.Pipe(), &Packet, sizeof(Packet), NULL, NULL);
			/* perform async read from server through thread */
			std::thread first(ReadData, Client.Pipe());
			/* flush pipe data */
			Packet = { 0 };
			/* reach code which needs value from server so ensure read thread is complete */
			first.join();
		}
	}
	return 0;
}