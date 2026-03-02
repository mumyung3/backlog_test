#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <locale.h>
#define SERVERPORT 9000
#define BUFSIZE 512

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
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	WCHAR wbuf[BUFSIZE + 1];

	while (1) {
		//accept
	ACCEPT_AGAIN:
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)	goto ACCEPT_AGAIN;

			g_errorcode = WSAGetLastError();
			__debugbreak();
			break;
		}

		WCHAR temp[101] = { 0 };
		// 접속한 클라이언트 정보 출력
		wprintf(L"\n[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트 번호 = %d\n",
			InetNtop(AF_INET, &(clientaddr.sin_addr), temp, _countof(temp)), ntohs(clientaddr.sin_port)
		);

		// 클라이언트와 데이터 통신
		while (1) {

		RECEIVE_AGAIN:
			// 데이터 받기
			retval = recv(client_sock, (char*)wbuf, BUFSIZE * 2, 0);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAEWOULDBLOCK)	goto RECEIVE_AGAIN;

				g_errorcode = WSAGetLastError();
				__debugbreak();
				break;
			}
			else if (retval == 0) {

				__debugbreak();
				break;

			}


			//받은 데이터 출력
			wbuf[retval / sizeof(WCHAR)] = L'\0';
			wprintf(L"[TCP/%s:%d] %s\n",
				InetNtop(AF_INET, &(clientaddr.sin_addr), temp, _countof(temp)),
				ntohs(clientaddr.sin_port),
				wbuf);

			// 데이터 보내기
		SEND_AGAIN:
			retval = send(client_sock, (char*)wbuf, retval, 0);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAEWOULDBLOCK)	goto SEND_AGAIN;
				g_errorcode = WSAGetLastError();
				__debugbreak();
				break;
			}

		}
		// 클라 데이터 통신 끝

		// closesocket()
		closesocket(client_sock);
		wprintf(L"[TCP 서버] 클라이언트 종료: IP 주소 = %s, 포트 번호 = %d\n",
			InetNtop(AF_INET, &(clientaddr.sin_addr), temp, _countof(temp)),
			ntohs(clientaddr.sin_port));
	}

	// closesocket() 
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}