#include <iostream>
#include <string>
#include <Windows.h>

#define BUFSIZE 200

const wchar_t shared_pipe_name[] = TEXT("\\\\.\\pipe\\SERVER_REY_PIPE_%d");

VOID Send(char* response, HANDLE hPipe);
VOID Receive(char* request, HANDLE hPipe);

//requests
const char get_to_modify[] = "read %d \0";
const char modify[] = "modw %d %s %f \0";
char exit_cycle[] = "exit %d \0";
char release_read[] = "relr %d \0";
char release_write[] = "relw %d \0";

//responses
char record_data[] = "data %d %s %f \0";
char modif_data_resp[] = "updt %d \0";
char not_found[] = "404_ \0";
char bad_request[] = "400_ \0";

int main(int argc, char** argv)
{
    int processId = std::stoi(argv[1]);
    std::cout << "Hi, I am client #" << processId;
    HANDLE hPipe;
    const char* text = "Default message from client.";

    char chBuf[BUFSIZE];
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

    while (true) 
    {
        std::cout << "\nOptions:\n1. Get record by ID\n2. Modify record by ID\n3. Exit\n4. Free reading\n5. Free writing\n";
        char* selected = new char[100];
        std::cin >> selected;

        if (strcmp(selected, "1") == 0)
        {
            Send((char*)get_to_modify, hPipe);
        }
        else if (strcmp(selected, "2") == 0)
        {
            Send((char*)modify, hPipe);
        }
        else if (strcmp(selected, "3") == 0)
        {
            char buf[100];
            snprintf(buf, strlen(buf), exit_cycle, processId);
            Send((char*)buf, hPipe);

            /*fSuccess = WriteFile(
                hPipe,
                buf,
                strlen(buf) + 1,
                &cbWritten,
                NULL);

            if (!fSuccess)
            {
                printf("WriteFile to pipe failed. GLE=%d\n", GetLastError());
                return -1;
            }*/

            return 0;
        }
        else if (strcmp(selected, "4"))
        {
            Send((char*)release_read, hPipe);
        }
        else if (strcmp(selected, "5"))
        {
            Send((char*)release_write, hPipe);
        }
        else 
        {
            std::cout << "Sorry cannot understand your option!\n";
            break;
        }

        fSuccess = ReadFile(
            hPipe,    // pipe handle 
            chBuf,    // buffer to receive reply 
            BUFSIZE * sizeof(char),  // size of buffer 
            &cbRead,  // number of bytes read 
            NULL);    // not overlapped 

        std::cout << chBuf;
        delete[] selected;
    }

    //printf("Sending %d byte message: \"%s\"\n", strlen(text), text);

    //fSuccess = WriteFile(
    //    hPipe,                  // pipe handle 
    //    text,             // message 
    //    strlen(text),              // message length 
    //    &cbWritten,             // bytes written 
    //    NULL);                  // not overlapped 

    //if (!fSuccess)
    //{
    //    printf("WriteFile to pipe failed. GLE=%d\n", GetLastError());
    //    return -1;
    //}

    //printf("\nMessage sent to server, receiving reply as follows:\n");

    //do
    //{
    //    // Read from the pipe. 

    //    fSuccess = ReadFile(
    //        hPipe,    // pipe handle 
    //        chBuf,    // buffer to receive reply 
    //        BUFSIZE * sizeof(TCHAR),  // size of buffer 
    //        &cbRead,  // number of bytes read 
    //        NULL);    // not overlapped 

    //    if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
    //        break;

    //    printf("\"%s\"\n", chBuf);
    //} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

    //if (!fSuccess)
    //{
    //    printf("ReadFile from pipe failed. GLE=%d\n", GetLastError());
    //    return -1;
    //}


    //while (true)
    //{

    //}

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