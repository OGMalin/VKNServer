#include <Windows.h>
#include <process.h>
#include <string>
#include <iostream>

#include "Console.h"

using namespace std;

CRITICAL_SECTION consoleCS;

void Console::threadLoop(void* lpv)
{
  char   c;
  Console* con=(Console*)lpv;
  string line="";
  DWORD dwRead;
  DWORD dwError;
	while (!con->abort)
  {
    if (con->hRead==NULL)
      break;
    if (!ReadFile(con->hRead, &c, 1, &dwRead, NULL))
    {
      dwError=GetLastError();
      if ((dwError==ERROR_BROKEN_PIPE) || (dwError==ERROR_HANDLE_EOF))
			{
        cout << "ReadFile fra console feilet" << endl;
        break;
      }
    }
    if (dwRead)
    {
      if (c=='\n')
      {
				con->input(line);
        line="";
      }else if (c!='\r')
      {
        line+=c;
      }
    }
  };
  _endthread();
}

Console::Console(HANDLE event)
{
	abort=false;
	hRead=GetStdHandle(STD_INPUT_HANDLE);
	hWrite=GetStdHandle(STD_OUTPUT_HANDLE);
	hEvent = event;
	InitializeCriticalSection(&consoleCS);

  hThread=(HANDLE)_beginthread(Console::threadLoop,0,this);
  if ((int)hThread==-1)
	{
		cout << "Får ikke startet console thread." << endl;
		abort=true;
		hThread=NULL;
	}
}

Console::~Console()
{
	if (hThread)
	{
		abort = true;
		CancelIoEx(hRead,NULL);
		if (WaitForSingleObject(hThread, 1000) == WAIT_TIMEOUT)
		{
			cerr << "Hard terminering" << endl;
			TerminateThread(hThread, 0);
		}
	}
	DeleteCriticalSection(&consoleCS);
}

void Console::input(const std::string s)
{
	EnterCriticalSection(&consoleCS);
	inQue.push(s);
	LeaveCriticalSection(&consoleCS);
	if (hEvent)
		SetEvent(hEvent);
}

bool Console::read(std::string &s)
{
	bool ret = false;
	EnterCriticalSection(&consoleCS);
	if (!inQue.empty())
	{
		s=inQue.front();
		inQue.pop();
		ret = true;
	}
	LeaveCriticalSection(&consoleCS);
	return ret;
}

void Console::write(std::string s)
{
	DWORD dwWrite;
	EnterCriticalSection(&consoleCS);
	WriteFile(hWrite,s.c_str(),(DWORD)s.length(),&dwWrite,NULL);
	WriteFile(hWrite,"\n",1,&dwWrite,NULL);
	LeaveCriticalSection(&consoleCS);
}
