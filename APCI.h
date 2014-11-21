#pragma once
#include <Windows.h>

#define I_FORMAT		0x00
#define S_FORMAT		0x01
#define U_FORMAT		0x03
#define STARTDTACT	0x04
#define STARTDTCON	0x08
#define STOPDTACT		0x10
#define STOPDTCON		0x20
#define TESTFRACT		0x40
#define TESTFRCON		0x80

/**
 Første del av en 104 melding (APDU=APCI+ASDU)
*/
class APCI
{
public:
	BYTE start;
	int length;
	int format;
	int func;
	int ssn;
	int rsn;
	APCI();
	virtual ~APCI();
	void clear();
	int get(BYTE* data);
	void set(const BYTE* data);
	bool valid();
};