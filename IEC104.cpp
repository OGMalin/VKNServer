#include "IEC104.h"
#include <memory.h>
//using namespace std;

IEC104Telegram::IEC104Telegram()
{
}

IEC104Telegram::~IEC104Telegram()
{
}

void IEC104Telegram::clear()
{
	memset(&apci,0,sizeof(apci));
}

void IEC104Telegram::set(BYTE* rec)
{
	clear();
	if (rec[0]!=0x68)
		return;
	apci.startbyte=0x68;
	apci.length=rec[1];
	if ((rec[2]&0x01)==0)
		apci.cf.i.cf=I_TYPE;
	else
		apci.cf.i.cf=rec[2]&0x03;

	if (apci.cf.i.cf==I_TYPE)
	{
		apci.cf.i.NS=(rec[2]>>1)+128*rec[3];
		apci.cf.i.NR=(rec[4]>>1)+128*rec[5];
	}else if (apci.cf.s.cf==S_TYPE)
	{
		apci.cf.s.NR=(rec[4]>>1)+128*rec[5];
	}else if (apci.cf.u.cf==U_TYPE)
	{
		apci.cf.u.startdt=(rec[2]>>2)&0x03;
		apci.cf.u.stopdt=(rec[2]>>4)&0x03;
		apci.cf.u.testfr=(rec[2]>>6)&0x03;
	}
}

int IEC104Telegram::getFormat()
{
	if (apci.cf.i.cf==I_TYPE)
		return I_TYPE;
	if (apci.cf.s.cf==S_TYPE)
		return S_TYPE;
	if (apci.cf.s.cf==U_TYPE)
		return U_TYPE;
	return -1;
}

int IEC104Telegram::getRaw(BYTE* data)
{
	if (apci.cf.u.cf==U_TYPE)
	{
		data[0]=apci.startbyte;
		data[1]=apci.length;
		data[2]=apci.cf.u.cf+(apci.cf.u.startdt<<2)+(apci.cf.u.stopdt<<4)+(apci.cf.u.testfr<<6);
		data[3]=0;
		data[4]=0;
		data[5]=0;
		return 6;
	}
	return 0;
}

IEC104Slave::IEC104Slave()
{
}
	
IEC104Slave::~IEC104Slave()
{
}

	
Telegram IEC104Slave::getResponse(Telegram& data)
{
	Telegram res;
	IEC104Telegram rec;
	IEC104Telegram send;
	send.clear();
	rec.set(data.data);
	int f=rec.getFormat();
	if (f==U_TYPE)
	{
		send.apci.cf.u.cf=U_TYPE;
		send.apci.length=4;
		send.apci.startbyte=0x68;
		if (rec.apci.cf.u.startdt==1)
			send.apci.cf.u.startdt=2;
		else if (rec.apci.cf.u.stopdt==1)
			send.apci.cf.u.stopdt=2;
		else if (rec.apci.cf.u.testfr==1)
			send.apci.cf.u.testfr=2;
		res.length=send.getRaw(&res.data[0]);
	}else
	{
		res.length=0;
	}
	return res;	
}
