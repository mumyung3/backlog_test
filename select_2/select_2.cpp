#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <locale.h>
#define SERVERPORT 9000
#define BUFSIZE 512

struct SOCKETINFO
{
	SOCKET sock;
	WCHAR buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;

};

int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[FD_SETSIZE];

BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int nIndex);

int g_errorcode = 0;
int main(int argc, char* argv[]) {
	//wprint 콘솔 출력 위한 초기화
	_wsetlocale(LC_ALL, L"");  // 시스템 로케일 사용

	int retval;
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)	return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) {
		g_errorcode = WSAGetLastError();
		__debugbreak();
		return 1;
	}

	// 넌블로킹 소켓으로 전환
	u_long on = 1;
	int retval1 = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval1 == SOCKET_ERROR) {
		g_errorcode = WSAGetLastError();
		__debugbreak();
		return 1;
	}

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {

		g_errorcode = WSAGetLastError();
		__debugbreak();
	}

	// listen()
	retval = listen(listen_sock, SOMAXCONN_HINT(65535));
	if (retval == SOCKET_ERROR) {
		g_errorcode = WSAGetLastError();
		__debugbreak();
	}

	// 데이터 통신에 사용할 변수
	FD_SET rset, wset;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, i;
	WCHAR wbuf[BUFSIZE + 1];

	while (1) {

		// 소켓 셋 초기화
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for (i = 0; i < nTotalSockets; i++) {
			if (SocketInfoArray[i]->recvbytes > SocketInfoArray[i]->sendbytes)
				FD_SET(SocketInfoArray[i]->sock, &wset);
			else
				FD_SET(SocketInfoArray[i]->sock, &rset);
		}

		// select()
		int retval2 = select(0, &rset, &wset, NULL, NULL);
		if (retval2 == SOCKET_ERROR) {
			g_errorcode = WSAGetLastError();
			__debugbreak();
		}

		//accept
		if (FD_ISSET(listen_sock, &rset)) {
			addrlen = sizeof(clientaddr);
			client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				g_errorcode = WSAGetLastError();
				__debugbreak();
			}
			else {
				WCHAR temp[101] = { 0 };
				// 접속한 클라이언트 정보 출력
				wprintf(L"\n[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트 번호 = %d\n",
					InetNtop(AF_INET, &(clientaddr.sin_addr), temp, _countof(temp)), ntohs(clientaddr.sin_port)
				);
				AddSocketInfo(client_sock);
			}
		}

		// 소켓 셋 검사  : 데이터 통신

		for (i = 0; i < nTotalSockets; i++) {
			SOCKETINFO* ptr = SocketInfoArray[i];

			// 데이터 받기
			if (FD_ISSET(ptr->sock, &rset)) {
				int retval3 = recv(ptr->sock, (char*)(ptr->buf), BUFSIZE * 2, 0);
				if (retval3 == SOCKET_ERROR) {
					g_errorcode = WSAGetLastError();
					//__debugbreak();
					RemoveSocketInfo(i);
					// rst 종료시 
					continue;
				}
				else if (retval3 == 0) {
					RemoveSocketInfo(i);
					// fin 종료시
					continue;
				}
				ptr->recvbytes = retval3;

				//받은 데이터 출력
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

				WCHAR temp[101] = { 0 };
				ptr->buf[retval3 / sizeof(WCHAR)] = L'\0';
				wprintf(L"NONBLOCK recv [TCP/%s:%d] 채팅 내역 : %s\n",
					InetNtop(AF_INET, &(clientaddr.sin_addr), temp, _countof(temp)),
					ntohs(clientaddr.sin_port),
					ptr->buf);
			}

			// 데이터 보내기
			if (FD_ISSET(ptr->sock, &wset)) {
				int retval4 = send(ptr->sock, (char*)(ptr->buf + ptr->sendbytes),
					(ptr->recvbytes - ptr->sendbytes), 0);
				if (retval4 == SOCKET_ERROR) {
					g_errorcode = WSAGetLastError();
					//__debugbreak();
					RemoveSocketInfo(i);
					continue;
				}
				ptr->sendbytes += retval4;
				if (ptr->recvbytes == ptr->sendbytes) {
					ptr->recvbytes = ptr->sendbytes = 0;
				}
			}
		}

	}

	// 윈속 종료
	WSACleanup();
	return 0;
}

BOOL AddSocketInfo(SOCKET sock) {

	if (nTotalSockets >= FD_SETSIZE) {
		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		return FALSE;
	}

	ptr->sock = sock;
	ptr->recvbytes = 0;
	ptr->sendbytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr;

	return TRUE;
}

void RemoveSocketInfo(int nIndex) {
	SOCKETINFO* ptr = SocketInfoArray[nIndex];

	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

	WCHAR temp[101] = { 0 };
	wprintf(L"[TCP 서버] 클라이언트 종료: IP 주소 = %s, 포트 번호 = %d\n",
		InetNtop(AF_INET, &(clientaddr.sin_addr), temp, _countof(temp)),
		ntohs(clientaddr.sin_port));

	closesocket(ptr->sock);
	delete ptr;
	if (nIndex != (nTotalSockets - 1))
		SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];

	--nTotalSockets;
}