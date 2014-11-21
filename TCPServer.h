#pragma once

#include <Windows.h>
#include <list>

#define TCPTELEGRAMMAX	512

struct TPCTelegram
{
	int length;
	u_long ipv4;
	BYTE data[TCPTELEGRAMMAX];
};

class TCPServer
{
public:
	int serverSocket;
	int clientSocket;
	sockaddr_in serverAddr;
	sockaddr_in clientAddr;
	unsigned short port;
	HANDLE hEvent;
	HANDLE hThread;
	std::list<TPCTelegram> inQue;
	static void threadLoop(void* lpv);
	bool abort;
	TCPServer();
	virtual ~TCPServer();
	void start();
	void stop();
	void input(const TPCTelegram data);
	int read(TPCTelegram& data);
	void write(TPCTelegram& data);
};

