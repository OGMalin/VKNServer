#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <iostream>
#include <stdlib.h>
#include "TCPServer.h"
using namespace std;

CRITICAL_SECTION TCPServerCS;

void TCPServer::threadLoop(void* lpv)
{
	int clientLen;
	TPCTelegram data;
  TCPServer* server=(TCPServer*)lpv;
	server->clientSocket=-1;
	while (!server->abort)
	{
		clientLen=sizeof(server->clientAddr);
		if ((server->clientSocket = accept(server->serverSocket, (struct sockaddr*) & server->clientAddr, (int*)&clientLen)) < 0)
		{
			cout << "accept() feilet." << endl;
			break;
		}

		cout << "Koplet til " << inet_ntoa(server->clientAddr.sin_addr) << endl;
		data.ipv4=server->clientAddr.sin_addr.S_un.S_addr;
		data.length=1;
		while (data.length>0)
		{
			if ((data.length=recv(server->clientSocket, (char*)data.data, TCPTELEGRAMMAX, 0)) < 0)
			{
				cout << "recv() feilet." << endl;
				break;
			}
			if (data.length)
				server->input(data);
		}
		closesocket(server->clientSocket);
		server->clientSocket=-1;
	}
  _endthread();
}

TCPServer::TCPServer()
{
	port=2404;
	hThread=NULL;
	hEvent=CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&TCPServerCS);
//	start();
}

TCPServer::~TCPServer()
{
	stop();
	DeleteCriticalSection(&TCPServerCS);
	CloseHandle(hEvent);
}

void TCPServer::start()
{
	WSADATA wsaData;
	if (hThread)
		return;
	abort=false;

	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
		cout << "WSAStartup() feilet." << endl;
		return;
	}

	if ((serverSocket=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		cout << "socket() feilet." << endl;
		return;
	}

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	if (bind(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
	{
		cout << "bind() feilet." << endl;
		return;
	}

	if (listen(serverSocket,5) <0)
	{
		cout << "listen() feilet." << endl;
		return;
	}

	cout << "IEC104Slave startet på " << inet_ntoa(serverAddr.sin_addr) << " Port: " << port << endl;

  hThread=(HANDLE)_beginthread(TCPServer::threadLoop,0,this);
  if ((int)hThread==-1)
	{
		cout << "Får ikke startet console thread." << endl;
		abort=true;
		hThread=NULL;
	}
}

void TCPServer::stop()
{
	if (hThread)
	{
		abort=true;
		if (WaitForSingleObject(hThread,1000)==WAIT_TIMEOUT)
			TerminateThread(hThread,0);
		hThread=NULL;
	}
}

void TCPServer::input(const TPCTelegram data)
{
	EnterCriticalSection(&TCPServerCS);
	inQue.push_back(data);
	LeaveCriticalSection(&TCPServerCS);
	SetEvent(hEvent);
}

int TCPServer::read(TPCTelegram& data)
{
	int ret=-1;
	EnterCriticalSection(&TCPServerCS);
	if (inQue.size()>0)
	{
		data=inQue.front();
		inQue.pop_front();
		ret=inQue.size();
	}
	LeaveCriticalSection(&TCPServerCS);
	return ret;
}

void TCPServer::write(TPCTelegram& data)
{
	if (hThread && clientSocket>=0)
		send(clientSocket,(char*)data.data,data.length,0);
}

