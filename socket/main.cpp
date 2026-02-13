#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "stdio.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <locale.h>  
#pragma comment(lib, "ws2_32.lib")

BOOL DomainToIP(WCHAR* szDomain, IN_ADDR* pAddr);


// 도메인 연결
int main() {
	//wprint 콘솔 출력 위한 초기화
	_wsetlocale(LC_ALL, L"");  // 시스템 로케일 사용
	// *** Winsock 초기화 (필수!) ***
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("WSAStartup 실패\n");
		return 0;
	}

	// 1. 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("소켓 생성 실패\n");
		return 0;
	}

	// 2. 서버 주소 설정 (위 코드)
	SOCKADDR_IN SockAddr;
	IN_ADDR Addr;
	memset(&SockAddr, 0, sizeof(SockAddr));

	WCHAR domain[256] = L"google.com";

	// 도메인 string -> ip 숫자 함수
	DomainToIP(domain, &Addr);

	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr = Addr;

	// ip string -> 숫자 함수
	//InetPton(AF_INET, L"142.250.198.46", &SockAddr.sin_addr); //dns 서버에서 계속 ip 바꾸는듯함.

	WCHAR szClientIP[16] = { 0 };
	// ip 숫자 -> string 함수
	InetNtop(AF_INET, &SockAddr.sin_addr, szClientIP, 16);

	// ip string -> 숫자 함수
	SOCKADDR_IN TestAddr;
	TestAddr.sin_addr.s_addr = inet_addr("147.46.114.70"); // 이제 inetpton 함수를 사용할것.

	// ip 숫자 -> string 함수
	printf("IP 주소 =%s\n", inet_ntoa(TestAddr.sin_addr));

	// WSAStringToAddress() 함수 연습
	// ipv6 버전
	const wchar_t* ipv6test = L"2001:0230:abcd:ffab:0023:eb00:ffff:1111";
	wprintf(L"IPv6 주소(변환 전) = %s\n", ipv6test);

	SOCKADDR_IN6 ipv6num;
	int addrlen = sizeof(ipv6num);
	WSAStringToAddress((LPWSTR)ipv6test, AF_INET6, NULL,
		(SOCKADDR*)&ipv6num, &addrlen);
	printf("IPv6 주소(변환 후) = 0x");
	for (int i = 0; i < 16; i++) {
		printf("%02x", ipv6num.sin6_addr.u.Byte[i]);
	}
	printf("\n");

	// WSAAddressToString() 함수 연습
	wchar_t ipaddr[50];
	DWORD ipaddrlen = 50; // sizeof 안됨. 유니코드는 문자 수로 넣어줘야함. 실제론 100바이트임.
	WSAAddressToString((SOCKADDR*)&ipv6num, sizeof(ipv6num), NULL, ipaddr, &ipaddrlen);
	wprintf(L"IPv6 주소(다시 변환후) = %s\n", ipaddr);


	SockAddr.sin_port = htons(80);  // HTTP 포트
	// 타임아웃 설정 추가
	DWORD timeout = 3000;  // 3초
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

	printf("연결 시도 중... (최대 3초)\n");
	if (connect(sock, (SOCKADDR*)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR) {
		printf("연결 실패: %d\n", WSAGetLastError());
		// 10060 = 타임아웃
		// 10061 = 연결 거부
		closesocket(sock);
		WSACleanup();
		getchar();
		return 0;
	}

	printf("구글 연결 성공!\n");

	// 4. 데이터 송수신
	send(sock, "GET / HTTP/1.1\r\n\r\n", 18, 0);
	char buf[1024];
	recv(sock, buf, sizeof(buf), 0);

	// 5. 종료
	closesocket(sock);
	WSACleanup();


	return 0;
}
BOOL DomainToIP(WCHAR* szDomain, IN_ADDR* pAddr)
{
	ADDRINFOW* pAddrInfo;
	SOCKADDR_IN* pSockAddr;
	if (GetAddrInfo(szDomain, L"0", NULL, &pAddrInfo) != 0)
	{
		return FALSE;
	}
	pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	*pAddr = pSockAddr->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return TRUE;
}
