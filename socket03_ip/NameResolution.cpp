#undef UNICODE
#undef _UNICODE

#pragma comment(lib, "ws2_32"); // 이거 하는 거와 안하는 이유가 뭘까
#include <winsock2.h>
#include <WS2tcpip.h> 
#include <stdio.h>

#define TESTNAME TEXT("www.google.com")

BOOL GetIPAddr(char* name, IN_ADDR* addr);
BOOL GetDomainName(IN_ADDR addr, char* name, int namelen);

int main(int argc, char* argv[]) {

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	printf(TEXT("도메인 이름( 변환 전) = %s \n"), TESTNAME);

	IN_ADDR addr;
	char temp[101] = TESTNAME;
	if (GetIPAddr(temp, &addr)) {

		printf("IP 주소(변환 후) = %s \n", InetNtop(AF_INET, &addr, temp, sizeof(temp))); // inet_ntoa 함수 비권장 함수 
		//printf("IP 주소(변환 후) = %s \n", inet_ntoa(addr)); // inet_ntoa 함수 비권장 함수, 

		// IP -> 도메인 
		char name[256];
		if (GetDomainName(addr, name, sizeof(name))) {

			printf("도메인 이름 (다시 변환 후) = %s \n", name);
			int a = 0;
			a++;
		}
	}
	WSACleanup();

	return 0;

}

// 도메인 -> ip 주소
BOOL GetIPAddr(char* name, IN_ADDR* addr) {
	// HOSTENT 구조체는 스레드당 하나씩 할당 된다.
	//HOSTENT* ptr = gethostbyname(name); // gethostbyname 비권장 함수
	ADDRINFO hints = { 0 };
	ADDRINFO* pAddrInfo = NULL;
	SOCKADDR_IN* pSockAddr;
	GetAddrInfo(name, NULL, &hints, &pAddrInfo); // gethostbyname 비권장 함수

	pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	memcpy(addr, &(pSockAddr->sin_addr), sizeof(pSockAddr->sin_addr));

	return TRUE;
}
// ip 주소 -> 도메인
BOOL GetDomainName(IN_ADDR addr, char* name, int namelen) {

	//HOSTENT* ptr = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
	SOCKADDR_IN sockaddr = { 0 };
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr = addr;
	getnameinfo((SOCKADDR*)&sockaddr, sizeof(sockaddr), name, namelen, NULL, 0, NI_NAMEREQD);


	return TRUE;
}