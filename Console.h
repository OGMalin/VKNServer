#pragma once

#include <string>
#include <queue>

class Console
{
public:
	HANDLE hRead;
	HANDLE hWrite;
	HANDLE hEvent;
	HANDLE hThread;
	static void threadLoop(void* lpv);
	bool abort;
	std::queue<std::string> inQue;
	Console(HANDLE event=NULL);
	virtual ~Console();
	void input(const std::string s);
	bool read(std::string &s);
	void write(std::string s);
};

