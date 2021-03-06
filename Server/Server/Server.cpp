/**
******************************************************************************
* @file    Server.cpp
* @author  Ian Glass
* @date    28-July-2018
* @brief   Server side async IPC implementation to receive client requests 
* to store and send student data.
******************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stdafx.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <winsock.h>
#include <string>
#include <iostream>
#include <vector>
/*----------------------------------------------------------------------------*/

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
		PipeHandle = CreateNamedPipe(TEXT("\\\\.\\pipe\\ConsolePipe"),
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			1,
			1024 * 16,
			1024 * 16,
			NMPWAIT_USE_DEFAULT_WAIT,
			NULL);
	}
	HANDLE Pipe() {
		return PipeHandle;
	}
	bool Connect() {
		return ConnectNamedPipe(PipeHandle, NULL);
	}
};

class Student {
private:
	char Name[1024] = "";
	int Age = 0;
public:
	void EditName(char arg[1024]) {
		strcpy_s(Name, arg);
	}
	char *GetName() {
		return Name;
	}
	void EditAge(int arg) {
		Age = arg;
	}
	int GetAge() {
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

int main() {
	/* create a DB of students as a vector of class Student with 1 'row'*/
	std::vector<Student> StudentTable(1);
	/* empty Student obj for vector.push_back */
	Student Empty;
	/* packet used for reading data from pipe */
	Payload Packet;
	/* buffer for writing data to pipe */
	char Buffer[1024];
	/* pipe opened on instantiation */
	CPipe Server;

	while (Server.Pipe() != INVALID_HANDLE_VALUE) {
		/* wait for a client process to connect to server */
		while (Server.Connect() != FALSE) {
			/* wait to receive data from client */
			while ((ReadFile(Server.Pipe(), &Packet, sizeof(Packet), NULL, NULL) != FALSE) && (GetLastError() != ERROR_BROKEN_PIPE)) {
				/* adds an empty Student at end of StudentTable vector */
				if (strcmp(Packet.CMD, "add") == 0) {
					StudentTable.push_back(Empty);
					sprintf_s(Buffer, "Added extra row to table, size is now %i", StudentTable.size());
				}
				/* retrieves the current StudentTable size */
				else if (strcmp(Packet.CMD, "size") == 0) {
					sprintf_s(Buffer, "Current table size is %i", StudentTable.size());
				}
				/* ensure index is in bounds for any CMD using packet.index */
				else if (Packet.Index < StudentTable.size()) {
					/* removes a Student row from the specified index */
					if (strcmp(Packet.CMD, "delete") == 0) {
						StudentTable.erase(StudentTable.begin() + Packet.Index);
						strcpy_s(Buffer, "Deleted row");
					}
					else if (strcmp(Packet.CMD, "edit") == 0) {
						if (strcmp(Packet.Type, "name") == 0) {
							StudentTable[Packet.Index].EditName(Packet.Data);
							strcpy_s(Buffer, "Changed student name");
						}
						else if (strcmp(Packet.Type, "age") == 0) {
							StudentTable[Packet.Index].EditAge(atoi(Packet.Data));
							strcpy_s(Buffer, "Changed student age");
						}
					}
					else if (strcmp(Packet.CMD, "get") == 0) {
						if (strcmp(Packet.Type, "name") == 0) {
							strcpy_s(Buffer, StudentTable[Packet.Index].GetName());
						}
						else if (strcmp(Packet.Type, "age") == 0) {
							_itoa_s(StudentTable[Packet.Index].GetAge(), Buffer, 10);
						}
					}
				}				
				/* catch any command format not caught by previous conditionals or out of index error */
				else {
					strcpy_s(Buffer, "Invalid command or index out of bounds");
				}
				/* perform blocking write */
				WriteFile(Server.Pipe(), &Buffer, sizeof(Buffer), NULL, NULL);
				/* flush pipe data */
				Packet = { 0 };
			}
			/* clear error code to prevent reaching this line again without new error */
			SetLastError(0);
			/* close pipe as client has been dropped */
			DisconnectNamedPipe(Server.Pipe());
		}
	}
	return 0;
}