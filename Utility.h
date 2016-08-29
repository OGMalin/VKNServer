#pragma once
/**
	* Utility.h
	* 13.09.2011  Init
**/

#include <Windows.h>
#include <string>

extern bool booleanString(const std::string& s);

// Coverting a double to a character string
extern char* ftoa(double d, char* buffer, int bufsize, int dec=2, int min=0);
extern const std::string ftoa(double d, int dec=2, int min=0);

// Get the last systemerror as string
extern const std::string getLastError();

extern const std::string getProgramPath();
extern const std::string getQuote(const std::string& s);

// Returns a word from a string where words are separated with whitespace.
// First word is n = 1
extern const std::string getWord(const std::string& s, int n, char* separator=NULL);

// Returns a line from a string where line are separated with \r or \n.
// First line is n = 1
extern const std::string getLine(const std::string& s, int n);

// Force all char to lowercase.
extern const std::string lowercase(const std::string& s);

// Removes whitespace in front and after a string.
extern const std::string trim(const std::string& s);

// Force all char to uppercase.
extern const std::string uppercase(const std::string& s);

// Checking to see if a string is a number.
// type = 0 (decimal plus '.+- ', 8 (octal), 10 (decimal) or 16 (hexadecimal)
extern bool isNumber(const std::string& s, int type=0);

// Make a string widt hex number for debuging
extern const std::string makeHexString(BYTE* data, int len);

// ip address to characterstring
extern char* iptoa(unsigned long ip, char* buffer, int bufsize);
extern const std::string iptoa(unsigned long ip);

// Characterstring to ip
extern DWORD atoip(std::string s);
extern DWORD atoip(char* sz);

// Check to see if a thread is running
extern bool threadRunning(HANDLE hThread);

// Tidsfunksjoner
extern std::string timeToString(const FILETIME& t);
extern std::string timeToString(const SYSTEMTIME& t);

extern void localFileTime(FILETIME& ft);

extern const FILETIME ZuluTimeToFileTime(const std::string& z);
extern const FILETIME ZuluTimeToLocalFileTime(const std::string& z);
extern const SYSTEMTIME ZuluTimeToSystemTime(const std::string& z);
extern const SYSTEMTIME ZuluTimeToLocalSystemTime(const std::string& z);
