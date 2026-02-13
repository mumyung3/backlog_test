#pragma comment(lib , "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <locale.h>  

#define SERVERIP	L"127.0.0.1"
#define SERVERPORT	9000
#define BUFSIZE		512
int g_errorcode = 0;
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
	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;

	// SOMAXCONN 테스트

	for (int j = 0; ;j++) {
		SOCKADDR_IN local = {};
		local.sin_family = AF_INET;
		//j만큼 127.0.0.j로 설정
		WCHAR ipStr[20] = {};
		swprintf_s(ipStr, 20, L"127.0.0.%d", j + 1);
		InetPton(AF_INET, ipStr, &local.sin_addr);
		for (int i = 0; ;i++) {

			SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
			bind(s, (SOCKADDR*)&local, sizeof(local));
			int ret = connect(s, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (ret == SOCKET_ERROR) {
				wprintf(L"실패!\n backlog 한계 : %d\n", i);
				wprintf(L"connect 실패! 에러 %d \n", WSAGetLastError());
				break;
			}
			wprintf(L"%d번째 연결 성공\n", i);

		}
	}


	getchar();
	//retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	//if (retval == SOCKET_ERROR) {
	//	g_errorcode = WSAGetLastError();
	//	__debugbreak();
	//}
	//// 데이터 통신에 사용할 변수
	////char buf[BUFSIZE + 1];
	//WCHAR wbuf[BUFSIZE + 1];
	//int len;

	//// 서버와 데이터 통신
	//while (1) {
	//	// 데이터 입력
	//	wprintf(L"\n[보낼 데이터] ");
	//	if (fgetws(wbuf, BUFSIZE + 1, stdin) == NULL)	break;

	//	// '\n' 문자 제거
	//	len = wcslen(wbuf);

	//	if (wbuf[len - 1] == L'\n')
	//		wbuf[len - 1] = L'\0';
	//	if (wcslen(wbuf) == 0)	break;

	//	// wchar_t 배열을 바이트로 캐스팅해서 전송
	//	int byte_len = wcslen(wbuf) * sizeof(wchar_t);

	//	// 데이터 보내기
	//	retval = send(sock, (char*)wbuf, byte_len, 0);
	//	if (retval == SOCKET_ERROR) {
	//		g_errorcode = WSAGetLastError();
	//		__debugbreak();
	//		break;
	//	}
	//	wprintf(L"[TCP 클라이언트] %d바이트를 보냈습니다.\n", retval);

	//	// 데이터 받기
	//	retval = recvn(sock, (char*)wbuf, retval, 0);
	//	if (retval == SOCKET_ERROR) {
	//		g_errorcode = WSAGetLastError();
	//		__debugbreak();
	//		break;
	//	}
	//	else if (retval == 0)	break;

	//	// 받은 데이터 출력
	//	wbuf[retval / sizeof(WCHAR)] = L'\0';
	//	wprintf(L"[TCP] 클라이언트 %d바이트를 받았습니다.\n", retval);
	//	wprintf(L"[받은 데이터] %s\n", wbuf);
	//}

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;

}

