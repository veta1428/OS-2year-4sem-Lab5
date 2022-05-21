#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>

#define BUFSIZE 512

std::vector<HANDLE> hClientsProcess;
std::vector<HANDLE> hClientsThread;

//requests
char get_to_modify[] = "read %d \0";
char modify[] = "modw %d %s %f \0";
char exit_cycle[] = "exit \0";
char release_read[] = "relr %d \0";
char release_write[] = "relw %d \0";

//responses
char record_data[] = "data %d %s %f \0";
char modif_data_resp[] = "updt %d \0";
char not_found[] = "404_ \0";
char server_internal_error[] = "500_ \0";

const wchar_t shared_pipe_name[] = TEXT("\\\\.\\pipe\\SERVER_REY_PIPE_%d");

VOID GetAnswerToRequest(LPTSTR pchRequest,
	LPTSTR pchReply,
	LPDWORD pchBytes);
DWORD WINAPI InstanceThread(LPVOID lpvParam);
std::vector<std::string> ParsedRequest(char* request);
VOID Send(char* response, HANDLE hPipe);
VOID Receive(char* request, HANDLE hPipe);
VOID ProcessRequest(char* request, HANDLE hPipe);

int main() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	int clientsNumber = 0;
	std::cout << "Enter number of clients: ";

	std::cin >> clientsNumber;

	for (int i = 0; i < clientsNumber; i++)
	{
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		wchar_t buffer[1000];
		wsprintfW(buffer, L"Client.exe %d", i + 1);

		// Start the child process. 
		if (!CreateProcess(NULL,   // No module name (use command line)
			buffer,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			CREATE_NEW_CONSOLE,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi)           // Pointer to PROCESS_INFORMATION structure
			)
		{
			printf("CreateProcess failed (%d).\n", GetLastError());
			return 0;
		}

		hClientsProcess.push_back(pi.hProcess);
		hClientsThread.push_back(pi.hThread);
	}

	BOOL   fConnected = FALSE;
	DWORD  dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;

	for (int i = 0; i < clientsNumber; i++)
	{
		wchar_t buffer[1000];
		wsprintf(buffer, shared_pipe_name, i + 1);

		hPipe = CreateNamedPipe(
			buffer,             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			BUFSIZE,                  // output buffer size 
			BUFSIZE,                  // input buffer size 
			0,                        // client time-out 
			NULL);                    // default security attribute 

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			std::cout << "CreateNamedPipe failed, GLE=%d.\n";
			return -1;
		}

		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

		fConnected = ConnectNamedPipe(hPipe, NULL) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected)
		{
			std::cout << "Connected to pipe :" << i + 1 <<"\n";

			hThread = CreateThread(
				NULL,              // no security attribute 
				0,                 // default stack size 
				InstanceThread,    // thread proc
				(LPVOID)hPipe,    // thread parameter 
				0,                 // not suspended 
				&dwThreadId);      // returns thread ID 
		}
		else {
			std::cout << "Could not connect";
			// The client could not connect, so close the pipe. 
			CloseHandle(hPipe);
		}
	}


	while (true)
	{

	}

	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{
	std::cout << "Just created";
	//std::cout << GetLastError();
	HANDLE hHeap = GetProcessHeap();
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	char buf_req[100];

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;

	// Do some extra error checking since the app will keep running even if this
	// thread fails.

	if (lpvParam == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL value in lpvParam.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	if (pchRequest == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL)
	{
		printf("\nERROR - Pipe Server Failure:\n");
		printf("   InstanceThread got an unexpected NULL heap allocation.\n");
		printf("   InstanceThread exitting.\n");
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	// Print verbose messages. In production code, this should be for debugging only.
	printf("InstanceThread created, receiving and processing messages.\n");

	// The thread's parameter is a handle to a pipe object instance. 

	hPipe = (HANDLE)lpvParam;

	// Loop until done reading
	while (1)
	{
		// Read client requests from the pipe. This simplistic code only allows messages
		// up to BUFSIZE characters in length.
		fSuccess = ReadFile(
			hPipe,        // handle to pipe 
			buf_req,    // buffer to receive data 
			100, // size of buffer 
			&cbBytesRead, // number of bytes read 
			NULL);        // not overlapped I/O 

		if (!fSuccess || cbBytesRead == 0)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				std::cout << "InstanceThread: client disconnected.\n";
			}
			else
			{
				std::cout << "InstanceThread ReadFile failed, GLE=%d.\n";
			}
			break;
		}

		std::cout << "Request from client: " << buf_req;
		printf(buf_req);

		ProcessRequest(buf_req, hPipe);
	}

	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);

	printf("InstanceThread exiting.\n");
	return 1;
}

VOID GetAnswerToRequest(LPTSTR pchRequest,
	LPTSTR pchReply,
	LPDWORD pchBytes)
	// This routine is a simple function to print the client request to the console
	// and populate the reply buffer with a default data string. This is where you
	// would put the actual client request processing code that runs in the context
	// of an instance thread. Keep in mind the main thread will continue to wait for
	// and receive other client connections while the instance thread is working.
{
	std::cout << "\nrepl func\n";
}

VOID ProcessRequest(char* request, HANDLE hPipe) 
{
	char* output = new char[BUFSIZE];
	std::vector<std::string> parsed_request = ParsedRequest(request);

	char* command = (char*)parsed_request[0].c_str();

	if (strncmp(command, get_to_modify, 4) == 0)
	{
		Send((char*)"Requested data", hPipe);
	}
	else if (strncmp(command, modify, 4) == 0)
	{
		Send((char*)"Requested modification", hPipe);
	}
	else if (strncmp(command, exit_cycle, 4) == 0)
	{
		Send((char*)"Requested exit", hPipe);
	}
	else if (strncmp(command, release_read, 4) == 0)
	{
		Send((char*)"Requested release read", hPipe);
	}
	else if (strncmp(command, release_write, 4) == 0)
	{
		Send((char*)"Requested release write", hPipe);
	}
	else
	{
		Send((char*)"400_", hPipe);
	}
}

std::vector<std::string> ParsedRequest(char* request) 
{
	std::string request_str = std::string(request);

	std::vector<std::string> words{};

	size_t pos = 0;
	while ((pos = request_str.find(" ")) != std::string::npos) {
		words.push_back(request_str.substr(0, pos));
		request_str.erase(0, pos + 1);
	}
	return words;
}

VOID Send(char* response, HANDLE hPipe) 
{
	DWORD cbBytesWritten = 0;

	BOOL fSuccess = WriteFile(
		hPipe,        // handle to pipe 
		response,     // buffer to write from 
		strlen(response) + 1, // number of bytes to write 
		&cbBytesWritten,   // number of bytes written 
		NULL);        // not overlapped I/O 

	if (!fSuccess)
	{
		printf("InstanceThread WriteFile failed, GLE=%d.\n", GetLastError());
	}
}

VOID Receive(char* request, HANDLE hPipe) 
{
	DWORD cbBytesRead = 0;
	BOOL fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		request,    // buffer to receive data 
		strlen(request), // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

	if (!fSuccess || cbBytesRead == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			std::cout << "InstanceThread: client disconnected.\n";
		}
		else
		{
			std::cout << "InstanceThread ReadFile failed, GLE=%d.\n";
		}
	}
}