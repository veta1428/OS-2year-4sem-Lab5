#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>

#define BUFSIZE 200

const wchar_t shared_pipe_name[] = TEXT("\\\\.\\pipe\\SERVER_REY_PIPE_%d");

VOID Send(char* response, HANDLE hPipe);
VOID Receive(char* request, HANDLE hPipe);

//requests
std::string get_to_modify = "read ";
std::string modify = "modw ";
std::string exit_cycle = "exit ";
std::string release_read = "relr ";
std::string release_write = "relw ";

struct Employee
{
	int ID;
	char name[10];
	double hours;
};

int main(int argc, char** argv)
{
	int processId = std::stoi(argv[1]);

	std::cout << "Hi, I am client #" << processId;
	HANDLE hPipe;

	BOOL   fSuccess = FALSE;
	DWORD  cbRead = 0, cbToWrite = 0, cbWritten = 0, dwMode = 0;
	wchar_t buffer[1000];
	wsprintf(buffer, shared_pipe_name, processId);

	// Try to open a named pipe; wait for it, if necessary. 
	while (1)
	{
		hPipe = CreateFile(
			buffer,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// retry
		if (GetLastError() != ERROR_PIPE_BUSY)
			continue;

		// All pipe instances are busy, so wait 
		if (!WaitNamedPipe(buffer, INFINITE))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return -1;
		}
	}

	while (true)
	{
		std::cout << "\nOptions:\n1. Get record by ID\n2. Modify record by ID\n3. Exit\n4. Free reading\n5. Free writing\n6. Draw a line\n";
		int selected = 0;
		std::cin >> selected;

		switch (selected)
		{
		case 1:
		{
			std::cout << "Enter id: ";
			int id = 0;
			std::cin >> id;

			std::string command = std::string(get_to_modify);
			std::string request = command
				.append(std::to_string(id))
				.append(" \0");

			Send((char*)request.c_str(), hPipe);

			char* response = new char[100];
			Receive(response, hPipe);
			std::cout << "Server responded: " << response << "\n";

			break;
		}
		case 2:
		{
			Employee e;
			std::cout << "Enter data to modify:\nId: ";
			int id = 0;
			std::cin >> id;
			e.ID = id;
			std::cout << "Name: ";
			std::string name;
			std::cin >> name;

			int max = 10;
			if (name.size() < 10)
				max = name.size();

			for (size_t k = 0; k < max; k++)
				e.name[k] = name[k];
			for (size_t k = max; k < 10; k++)
			{
				e.name[k] = '\0';
			}

			double hours = 0;
			std::cout << "Enter hours: ";
			std::cin >> hours;
			e.hours = hours;

			std::string command = std::string(modify);
			std::string request = command
				.append(std::to_string(e.ID))
				.append(" ")
				.append(e.name)
				.append(" ")
				.append(std::to_string(e.hours))
				.append(" \0");

			Send((char*)request.c_str(), hPipe);

			char* response = new char[100];
			Receive(response, hPipe);
			std::cout << "Server responded: " << response << "\n";
			break;
		}
		case 3:
		{
			std::string command = std::string(exit_cycle);
			Send((char*)command.c_str(), hPipe);
			return 0;
		}
		case 4:
		{
			std::cout << "Enter id: ";
			int id = 0;
			std::cin >> id;

			std::string command = std::string(release_read);
			std::string request = command
				.append(std::to_string(id))
				.append(" \0");

			Send((char*)request.c_str(), hPipe);

			char* response = new char[100];
			Receive(response, hPipe);
			std::cout << "Server responded: " << response << "\n";
			break;
		}
		case 5:
		{
			char buf[100];
			std::cout << "Enter id: ";
			int id = 0;
			std::cin >> id;

			std::string command = std::string(release_write);
			std::string request = command
				.append(std::to_string(id))
				.append(" \0");

			Send((char*)request.c_str(), hPipe);

			char* response = new char[100];
			Receive(response, hPipe);
			std::cout << "Server responded: " << response << "\n";
			break;
		}
		case 6:
		{
			std::cout << "\n_____________________________________________________________________\n";
			break;
		}
		default:
			std::cout << "Sorry, I do not understand you(";
			break;
		}
	}
	while (true) {

	}
	return 0;
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
		printf("InstanceThread WriteFile failed, GLE=%d.\n", GetLastError());
}

VOID Receive(char* request, HANDLE hPipe)
{
	DWORD cbBytesRead = 0;
	BOOL fSuccess = ReadFile(
		hPipe,        // handle to pipe 
		request,    // buffer to receive data 
		BUFSIZE, // size of buffer 
		&cbBytesRead, // number of bytes read 
		NULL);        // not overlapped I/O 

	if (!fSuccess || cbBytesRead == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
			std::cout << "InstanceThread: client disconnected.\n";
		else
			std::cout << "InstanceThread ReadFile failed, GLE = " << GetLastError() << "\n";
	}
}