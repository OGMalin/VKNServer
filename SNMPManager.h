#pragma once
#include <Windows.h>
#include <queue>

struct SNMPTelegram
{
	int type;
	DWORD ip;
	int genericTrap;
	int specificTrap;
	std::string object;
	std::string value;
	int error_status;
};

class SNMPManager
{
public:
	int requestId;
	bool debug;
	bool abort;
	bool running;
	int trapPort;
	DWORD trapIp;
	int responsePort;
	SOCKET responseSocket;
	SOCKET trapSocket;
	DWORD responseIp;
	HANDLE hEvent;
	HANDLE hTrap;
	HANDLE hResponse;
	std::queue<SNMPTelegram> inQue;

	SNMPManager(HANDLE event);
	virtual SNMPManager::~SNMPManager();

	bool start();
	void stop();
	bool read(SNMPTelegram& t);
	void getRequest(DWORD ip, std::string object, std::string value="NULL", int version=1, std::string community="public");
	void setRequest(DWORD ip, std::string object, std::string value = "NULL", int version = 1, std::string community = "public");
	void send(DWORD ip, BYTE* data, int len);
	static void trapLoop(void* lpv);
	static void responseLoop(void* lpv);
	void input(const BYTE* data, int len, unsigned long addr);
	bool get(DWORD ip, std::string obj);
	bool set(DWORD ip, std::string obj, std::string val);
};