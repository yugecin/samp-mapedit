/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "sockets.h"
#include "ui.h"
#include "../shared/serverlink.h"
#include <windows.h>
#include <winsock2.h>
#include <io.h>

static int inited;
static int socketsend, socketrecv;

void sockets_init()
{
	struct sockaddr_in addr;
	struct sockaddr *saddr = (struct sockaddr*) &addr;
	int flags;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) ||
		LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)
	{
		sprintf(debugstring,
			"cannot initialize winsock: %d\n", WSAGetLastError());
		ui_push_debug_string();
		inited = 0;	
		return;
	}

	inited = 1;

	socketrecv = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketrecv == -1) {
		sprintf(debugstring,
			"~r~cannot create recv socket %d", WSAGetLastError());
		ui_push_debug_string();
		return;
	}

	socketsend = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketsend == -1) {
		closesocket(socketrecv);
		socketrecv = -1;
		sprintf(debugstring,
			"~r~cannot create send socket", WSAGetLastError());
		ui_push_debug_string();
		return;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SOCKET_PORT_CLIENT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketrecv, saddr, sizeof(addr)) == -1) {
		closesocket(socketrecv);
		closesocket(socketsend);
		socketrecv = -1;
		sprintf(debugstring, "~r~socket cannot listen (port %d)",
			SOCKET_PORT_SERVER);
		ui_push_debug_string();
		return;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(SOCKET_PORT_SERVER);
	if (connect(socketsend, saddr, sizeof(struct sockaddr)) == -1) {
		closesocket(socketrecv);
		closesocket(socketsend);
		socketrecv = -1;
		sprintf(debugstring, "send socket cannot connect");
		ui_push_debug_string();
		return;
	}

	/*set sockets as non-blocking*/
	flags = 1;
	ioctlsocket(socketrecv, FIONBIO, (DWORD*) &flags);
	ioctlsocket(socketsend, FIONBIO, (DWORD*) &flags);
}

void sockets_recv()
{

}

void sockets_send(void *rpc, int len)
{
	send(socketsend, (char*) rpc, len, 0);
}

void sockets_dispose()
{
	if (socketrecv != -1) {
		closesocket(socketrecv);
		closesocket(socketsend);
	}
	if (inited) {
		WSACleanup();
	}
}
