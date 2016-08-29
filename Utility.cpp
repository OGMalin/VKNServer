/**
	* Utility.cpp
	* 13.09.2011  Init
**/

#include <Windows.h>
#include <string>
#include <time.h>

using namespace std;

char* WhiteSpace = " \t\n\r\v\f";

bool booleanString(const std::string& s)
{
  string cs;
  if (s.length()<1)
    return false;
  unsigned int i;
  cs="";
  for (i=0;i<s.length();i++)
    cs+=toupper(s.at(i));
  if (cs.at(0)=='T')
    return true;
  if (cs.at(0)=='F')
    return false;
  if (cs=="ON")
    return true;
  if (cs=="OFF")
    return false;
  if (isdigit(cs.at(0)))
  {
    if (cs.at(0)=='0')
      return false;
    return true;
  }
  return false;
}

char* ftoa(double d, char* buffer, int bufsize, int dec, int min)
{
  char sz[20];
  sprintf_s(sz,20,"%%%d.%df",min,dec);
  sprintf_s(buffer,bufsize,sz,d);
  return buffer;
}

const std::string ftoa(double d, int dec, int min)
{
	char buffer[256];
	ftoa(d,buffer,256,dec,min);
  return string(buffer);
}

const std::string getLastError()
{
/*
  char buf[256];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),0,buf,256,NULL);
  return (string(buf));
*/
  return (string(""));
}

const std::string getProgramPath()
{
/*
	string s;
	char sz[MAX_PATH];
	if (!GetModuleFileName(NULL,sz,MAX_PATH))
		return string("");
	string::size_type size;
	s=sz;
	size=s.find_last_of('\\');
	if (size!=string::npos)
		return s.substr(0,size+1);
*/
	return string("");
};

const std::string getQuote(const string& input)
{
	std::string::size_type i1, i2;
  i1=input.find_first_of("\"\'",0);
  if (i1==string::npos)
    return "";
  if (input.at(i1)=='\'')
    i2=input.find_first_of("\'",i1+1);
  else
    i2=input.find_first_of("\"",i1+1);
  if (i2==string::npos)
    return "";
  return input.substr(i1+1,i2-i1-1);
};

const std::string getWord(const string& input, int n, char* separator)
{
	std::string::size_type i1, i2;
  int j=1;
  char* sep;
  if (!separator)
    sep=WhiteSpace;
  else
    sep=separator;
  i1=input.find_first_not_of(sep,0);
  while (j<n)
  {
     i1=input.find_first_of(sep,i1);
     if (i1==string::npos)
       return "";
     i1=input.find_first_not_of(sep,i1);
     if (i1==string::npos)
       return "";
     j++;
  };
  if (i1==string::npos)
    return "";
  i2=input.find_first_of(sep,i1);
  if (i2==string::npos)
    i2=input.length();
  return input.substr(i1,i2-i1);
}

const std::string getLine(const string& input, int n)
{
	return getWord(input,n,"\r\n");
}

const std::string lowercase(const std::string& s)
{
  string line;
	std::string::size_type i, len;
  i=0;
  len=s.length();
  while (i<len)
    line+=tolower(s.at(i++));
  return line;
}

const std::string trim(const string& s) 
{
  if(s.length() == 0)
    return s;
	std::string::size_type b = s.find_first_not_of(WhiteSpace);
	std::string::size_type e = s.find_last_not_of(WhiteSpace);
  if(b == string::npos) // No non-spaces
    return "";
  return string(s, b, e - b + 1);
}

const std::string uppercase(const std::string& s)
{
  string line;
	std::string::size_type i, len;
  i=0;
  len=s.length();
  while (i<len)
    line+=toupper(s.at(i++));
  return line;
}

bool isNumber(const string& s, int type)
{
  string::size_type e;
  if (!s.length())
    return false;
  switch (type)
  {
    case 8:  e=s.find_first_not_of("01234567"); break;
    case 10: e=s.find_first_not_of("0123456789"); break;
    case 16: e=s.find_first_not_of("0123456789abcdefABCDEF"); break;
    default: e=s.find_first_not_of("0123456789.+- "); break;
  }
  if (e==string::npos)
    return true;
  return false;
}

const std::string makeHexString(BYTE* data, int len)
{
	string s="";
	char buffer[16];
	for (int i=0;i<len;i++)
	{
		sprintf_s(buffer,16,"%02X ",data[i]);
		s+=buffer;
	}
	return trim(s);
}

char* iptoa(unsigned long ip, char* buffer, int bufsize)
{
	union 
	{
		unsigned long nip;
		struct {unsigned char a,b,c,d;} cip;
	};
	
	nip=(unsigned long) ip;
	sprintf_s(buffer,bufsize,"%u.%u.%u.%u",cip.d,cip.c,cip.b,cip.a);
	return buffer;
}

const std::string iptoa(unsigned long ip)
{
	char buffer[256];
	iptoa(ip,buffer,256);
	return string(buffer);
}

DWORD atoip(std::string s)
{
	union 
	{
		unsigned long nip;
		struct {char a,b,c,d;} cip;
	};
	std::string w;
	cip.d=atoi(getWord(s,1,".").c_str());
	cip.c=atoi(getWord(s,2,".").c_str());
	cip.b=atoi(getWord(s,3,".").c_str());
	cip.a=atoi(getWord(s,4,".").c_str());
	return nip;
}

DWORD atoip(char* sz)
{
	return atoip(string(sz));
}

bool threadRunning(HANDLE hThread)
{
	if (!hThread)
		return false;
	DWORD res=0;
	if (GetExitCodeThread(hThread, &res))
	{
		if (res==STILL_ACTIVE)
			return true;
	}
	return false;
}

std::string timeToString(const SYSTEMTIME& t)
{
	char sz[256];
	sprintf_s(sz, 256, "%02d.%02d.%d %02d:%02d:%02d", t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond);
	return sz;
}

std::string timeToString(const FILETIME& t)
{
	SYSTEMTIME st;
	if (!FileTimeToSystemTime(&t, &st))
		return "";
	return timeToString(st);
}

void localFileTime(FILETIME& ft)
{

	SYSTEMTIME st;
	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
}

const SYSTEMTIME ZuluTimeToSystemTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	SYSTEMTIME st;
	memset(&st, 0, sizeof(SYSTEMTIME));
	if (z.length() == 20)
	{
		st.wYear = atoi(z.substr(0, 4).c_str());
		st.wMonth = atoi(z.substr(5, 2).c_str());
		st.wDay = atoi(z.substr(8, 2).c_str());
		st.wHour = atoi(z.substr(11, 2).c_str());
		st.wMinute = atoi(z.substr(14, 2).c_str());
		st.wSecond = atoi(z.substr(17, 2).c_str());
	};
	return st;
};

const SYSTEMTIME ZuluTimeToLocalSystemTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	TIME_ZONE_INFORMATION tzi;
	SYSTEMTIME lst;
	GetTimeZoneInformation(&tzi);
	SYSTEMTIME st = ZuluTimeToSystemTime(z);
	SystemTimeToTzSpecificLocalTime(&tzi, &st, &lst);
	return lst;
};

const FILETIME ZuluTimeToFileTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	FILETIME ft;
	SYSTEMTIME st = ZuluTimeToSystemTime(z);
	SystemTimeToFileTime(&st, &ft);
	return ft;
};

const FILETIME ZuluTimeToLocalFileTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	FILETIME ft;
	SYSTEMTIME st = ZuluTimeToLocalSystemTime(z);
	SystemTimeToFileTime(&st, &ft);
	return ft;
};

