#pragma once

#include "APCI.h"
#include "ASDU.h"

class APDU
{
public:
	APCI apci;
	ASDU asdu;
	APDU();
	virtual ~APDU();
	void clear();
	int get(BYTE* data);
	bool set(const BYTE* data);
	void addIO(int address);
	void addIO(InformationObject& inf);
	void setDUI(int common, int ident, int cause);
	void setAPCI(int format);
	bool valid();
};