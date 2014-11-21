#include "APDU.h"

APDU::APDU()
{
}

APDU::~APDU()
{
}

void APDU::clear()
{
	apci.clear();
	asdu.clear();
}

int APDU::get(BYTE* data)
{
	if (!apci.valid())
		return 0;
	apci.get(data);
	if (apci.format==I_FORMAT)
		asdu.get(&data[6]);
	return apci.length+2;
}

bool APDU::set(const BYTE* data)
{
	apci.set(data);
	if (!apci.valid())
		return false;
	if (apci.format==I_FORMAT)
		asdu.set(&data[6]);
	return true;
}

bool APDU::valid()
{
	if (!apci.valid())
		return false;
	return asdu.valid();
}


void APDU::addIO(int address)
{
	InformationObject io;
	io.address=address;
	int len=asdu.addIO(io);
	apci.length+=len;
}

void APDU::addIO(InformationObject& inf)
{
	int len=asdu.addIO(inf);
	apci.length+=len;
}

void APDU::setAPCI(int format)
{
	apci.format=format;
}

void APDU::setDUI(int common, int ident, int cause)
{
	asdu.dui.ident=ident;
	asdu.dui.cause.cause=cause;
	asdu.dui.common=common;
	apci.length+=6;
}
