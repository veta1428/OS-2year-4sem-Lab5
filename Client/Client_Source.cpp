#include <iostream>
#include <string>
#include <Windows.h>

#define BUFSIZE 200

const wchar_t shared_pipe_name[] = TEXT("\\\\.\\pipe\\SERVER_REY_PIPE_%d");

int main(int argc, char** argv)
{
    std::cout << "Hi, I am client #" << std::stoi(argv[1]);

    HANDLE hPipe;
    const char* text = "Default message from client.";

    TCHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead = 0, cbToWrite = 0, cbWritten = 0, dwMode = 0;
    wchar_t buffer[1000];
    wsprintf(buffer, shared_pipe_name, std::stoi(argv[1]));

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

        // Exit if an error other than ERROR_PIPE_BUSY occurs. 

        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            std::cout << "Could not open pipe." << GetLastError();
            return -1;
        }

        // All pipe instances are busy, so wait for 20 seconds. 

        if (!WaitNamedPipe(buffer, 20000))
        {
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }
    }

    printf("Sending %d byte message: \"%s\"\n", strlen(text), text);

    fSuccess = WriteFile(
        hPipe,                  // pipe handle 
        text,             // message 
        strlen(text),              // message length 
        &cbWritten,             // bytes written 
        NULL);                  // not overlapped 

    if (!fSuccess)
    {
        printf("WriteFile to pipe failed. GLE=%d\n", GetLastError());
        return -1;
    }

    printf("\nMessage sent to server, receiving reply as follows:\n");

    do
    {
        // Read from the pipe. 

        fSuccess = ReadFile(
            hPipe,    // pipe handle 
            chBuf,    // buffer to receive reply 
            BUFSIZE * sizeof(TCHAR),  // size of buffer 
            &cbRead,  // number of bytes read 
            NULL);    // not overlapped 

        if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
            break;

        printf("\"%s\"\n", chBuf);
    } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

    if (!fSuccess)
    {
        printf("ReadFile from pipe failed. GLE=%d\n", GetLastError());
        return -1;
    }


    while (true)
    {

    }

    return 0;
}