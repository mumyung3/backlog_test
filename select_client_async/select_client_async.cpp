// 논블로킹 서버와 블로킹 클라 테스트

#pragma comment(lib , "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <WS2tcpip.h>
#include <locale.h>  
#include <conio.h>
#define SERVERIP	L"127.0.0.1"
#define SERVERPORT	9000
#define BUFSIZE		512
int g_errorcode = 0;

using namespace::std;

enum class ConnState {
	CONNECTING,
	CONNECTED,
	CLOSED
};

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags) {

	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)	return SOCKET_ERROR;
		else if (received == 0)	break;

		left -= received;
		ptr += received;
	}

	return (len - left);
}

int main(int argc, char* argv[]) {
	//wprint 콘솔 출력 위한 초기화
	_wsetlocale(LC_ALL, L"");  // 시스템 로케일 사용

	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)	return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		g_errorcode = WSAGetLastError();
		__debugbreak();
	}

	// 논 블로킹 설정
	u_long mode = 1;
	ioctlsocket(sock, FIONBIO, &mode);

	// 서버 주소 설정
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	// connect()
	int ret = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

	ConnState state;
	// 비동기 connect
	if (ret == SOCKET_ERROR) {

		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK) {
			state = ConnState::CONNECTING;
		}
		else {
			wprintf(L"connect 실패! 에러 %d \n", err);
			return 0;
		}
	}
	else {
		state = ConnState::CONNECTED;
	}


	// 데이터 통신에 사용할 변수
	WCHAR wbuf[BUFSIZE + 1];
	int len;

	fd_set readset, writeset;

	wstring sendBuffer; // 보낼 데이터 큐 역할
	// 이벤트 루프
	while (1) {
		FD_ZERO(&readset);
		FD_ZERO(&writeset);

		if (state == ConnState::CONNECTING) {
			FD_SET(sock, &writeset);
		}
		if (state == ConnState::CONNECTED) {
			// 상시
			FD_SET(sock, &readset);

			//보낼 데이터가 있을시에만 writeset 키기(cpu 100% 방지)
			if (!sendBuffer.empty()) {
				FD_SET(sock, &writeset);
			}
		}

		//select
		timeval timeout = { 0,0 };
		int selectret = select(0, &readset, &writeset, NULL, &timeout);
		if (selectret == SOCKET_ERROR) {
			g_errorcode = WSAGetLastError();
			__debugbreak();
		}

		// select 후 입력 체크
		if (_kbhit()) {
			wstring input;
			wcin >> input;
			sendBuffer += input;
		}

		if (state == ConnState::CONNECTING && FD_ISSET(sock, &writeset)) {
			int err;
			int len = sizeof(err);
			getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

			if (err == 0) {
				//연결 성공
				state = ConnState::CONNECTED;
				wprintf(L"connect 성공!\n");

			}
			else {
				//연결 실패
				wprintf(L"connect 실패! 에러 %d\n", err);
				break;
			}
		}

		//데이터 수신
		if (state == ConnState::CONNECTED && FD_ISSET(sock, &readset)) {
			WCHAR wbuffer[BUFSIZE + 1] = {};
			int recvRet = recv(sock, (char*)wbuffer, BUFSIZE * 2, 0);

			if (recvRet == SOCKET_ERROR) {
				g_errorcode = WSAGetLastError();
				wprintf(L"rst및 비정상 종료\n");
				__debugbreak();
				// rst 종료시 
				break;
			}
			if (recvRet == 0) {
				wprintf(L"fin 종료\n");
				break;
			}
			wbuffer[recvRet / sizeof(WCHAR)] = L'\0';
			wprintf(L"서버로부터 받은 데이터 :%s\n", wbuffer);
		}

		// 데이터 송신
		if (state == ConnState::CONNECTED && FD_ISSET(sock, &writeset) && !sendBuffer.empty()) {
			int sendRet = send(sock, (const char*)sendBuffer.c_str(), (int)(sendBuffer.size() * sizeof(WCHAR)), 0);

			if (sendRet == SOCKET_ERROR) {
				g_errorcode = WSAGetLastError();
				wprintf(L"비정상 종료\n");
				__debugbreak();
				// rst 종료시 
				break;
			}
			if (sendRet > 0) {
				sendBuffer.erase(0, sendRet / sizeof(WCHAR));
			}

		}

	}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;

}

