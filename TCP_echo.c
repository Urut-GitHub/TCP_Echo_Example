#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_PORT "666"
#define DEFAULT_IP "127.0.0.1"

#define DEFAULT_BUFLEN 4096

#define MSG_DELIM "?"

int i_global_thr_cnt = 0;

/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/
DWORD listener_func(LPVOID *ptr) {
  int iRes;
  int iSendRes;
  int iDutyFlag = 0;

  char cResponse[DEFAULT_BUFLEN];
  char *cMsg;

  char* cParseStr = malloc(DEFAULT_BUFLEN);

  char recvbuf[DEFAULT_BUFLEN];
  int  recvbuflen = DEFAULT_BUFLEN;

  SOCKET AcSock = (SOCKET)ptr;

  printf("Listener Thread Start. Thread Cnt: [%d]\n", i_global_thr_cnt);

  while (1) {
    printf("Start listening on socket.\n");

    iRes = recv(AcSock, recvbuf, recvbuflen, 0);

    if (iRes > 0) {
      printf("Bytes received: [%d] String: [%s]\n", iRes, recvbuf);

      strcpy(cParseStr, recvbuf);

      while (1) {
        cMsg = strtok(cParseStr, MSG_DELIM);

        if(strlen(cMsg) > 0) {
          cParseStr = cParseStr + strlen(cMsg) + 1;
          printf("Got message: [%s]\n", cMsg);
        }

        if (strcmp(cMsg, "status") == 0) {
          sprintf(cResponse, "Number of active connections: [%d]", i_global_thr_cnt);

          printf("Got status command. Response: [%s]\n", cResponse);
        }
        else if (strcmp(cMsg, "duty") == 0) {
          printf("Got duty command.\n");
          strcpy(cResponse, "For the Emperor!");
          iDutyFlag = 1;
        }
        else {
          // Echo back to the sender
          printf("Echo back to client.\n");
          strcpy(cResponse, cMsg);
        }

        printf("Sending to client: [%s]\n", cResponse);
        iSendRes = send(AcSock, cResponse, strlen(cResponse), 0);

        if (iSendRes == SOCKET_ERROR) {
          printf("Send failed with error: [%d]\n", WSAGetLastError());
          closesocket(AcSock);
          WSACleanup();
          break;
        }

        printf("Bytes sent: %d\n", iSendRes);

        if (iDutyFlag != 0) {
          printf("Duty fullied.\n");
          i_global_thr_cnt--;

          return 0;
        }

        if (strlen(cParseStr) < 1) {
          printf("No more messages. Keep listening.\n");
          break;
        }
      }
    }
    else if (iRes == 0) {
      printf("Connection closing...\n");
      break;
    }
    else {
      printf("Recv failed with error: %d\n", WSAGetLastError());
      closesocket(AcSock);
      WSACleanup();

      break;
    }
  }

  closesocket(AcSock);
  WSACleanup();

  i_global_thr_cnt--;

  printf("Thread finish. Thread Cntr: [%d]\n", i_global_thr_cnt);
}
/***************************************************************/
int get_listen_socket(const char* ip, SOCKET* SrvSock) {
  int iRes;
  struct addrinfo* result = NULL;
  struct addrinfo hints;
  WSADATA wsaData;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  iRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iRes != 0) {
    printf("WSAStartup failed with error: [%d]\n", iRes);

    return 1;
  }

  // Resolve the server address and port
  iRes = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
  if (iRes != 0) {
    printf("getaddrinfo failed with error: [%d]\n", iRes);

    return 1;
  }

  // Create a SOCKET for the server to listen for client connections.
  *SrvSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (*SrvSock == INVALID_SOCKET) {
    printf("socket failed with error: [%ld]\n", WSAGetLastError());
    freeaddrinfo(result);

    return 1;
  }

  // Setup the TCP listening socket
  iRes = bind(*SrvSock, result->ai_addr, (int)result->ai_addrlen);
  if (iRes == SOCKET_ERROR) {
    printf("bind failed with error: [%d]\n", WSAGetLastError());

    freeaddrinfo(result);
    closesocket(*SrvSock);
    WSACleanup();

    return 1;
  }

  return 0;
}
/***************************************************************/
int main() {
  SOCKET LSock = INVALID_SOCKET;
  SOCKET AcSock = INVALID_SOCKET;
  HANDLE hThrHand;
  DWORD iThrId;

  int iRes;
  int iSendRes;

  printf("Hello World!\n");

  iRes = get_listen_socket(DEFAULT_IP, &LSock);

  if (iRes != 0) {
    printf("Failed to initialize listen socket. Return.\n");

    return 1;
  }

  iRes = listen(LSock, SOMAXCONN);

  if (iRes != 0) {
    printf("Listen function failed with error: [%d]\n", WSAGetLastError());
    closesocket(LSock);
    WSACleanup();

    return 1;
  }


  while (1) {
    printf("Waiting for client connection.\n");

    if (i_global_thr_cnt < 0) {
      break;
    }

    AcSock = accept(LSock, NULL, NULL);
    if (AcSock == INVALID_SOCKET) {
      printf("Accept failed with error: [%d]\n", WSAGetLastError());
      closesocket(AcSock);
      WSACleanup();

      return 1;
    }

    printf("Accepted connection.\n");

    i_global_thr_cnt++;

    hThrHand = CreateThread(
      NULL,                                    // default security attributes
      0,                                       // use default stack size  
      (LPTHREAD_START_ROUTINE)listener_func,   // thread function name
      (LPVOID)AcSock,                          // argument to thread function 
      0,                                       // use default creation flags 
      &iThrId);                                // returns the thread identifier 

    printf("Thread done.\n");

    if (hThrHand == NULL) {
      printf("Failed to create thread. Error Code: [%d]\n", WSAGetLastError());
      return 1;
    }
    else {
      printf("New thread created.\n");
      hThrHand = NULL;
    }
  }

  printf("Farwell cruel world.\n");

  return 0;
}