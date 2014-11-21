#pragma once
#include <windows.h>
#include "TCPServer.h"

//#define IEC104MAXLEN	255
#define I_TYPE				0
#define S_TYPE				1
#define U_TYPE				3

struct I_FORMAT
{
	unsigned int cf;
	unsigned int NS;
	unsigned int NR;
};

struct S_FORMAT
{
	unsigned int cf;
	unsigned int NR;
};

struct U_FORMAT
{
	unsigned int cf;
	unsigned int startdt;
	unsigned int stopdt;
	unsigned int testfr;
};

union ControlField
{
	I_FORMAT i;
	S_FORMAT s;
	U_FORMAT u;
};

struct APCI
{ // Lengde 6:
	unsigned int startbyte; // Alltid 68h
	unsigned int length;    // Maks lengde er 253 eg. hele telegrammet er maks 255
	ControlField cf;
};

class IEC104Telegram
{
public:
	APCI apci;
	IEC104Telegram();
	virtual ~IEC104Telegram();
	void clear();
	void set(BYTE* rec);
	int getFormat();
	int getRaw(BYTE* data);
};

class IEC104Slave
{
public:
	IEC104Slave();
	virtual ~IEC104Slave();
	Telegram getResponse(Telegram& data);
};

