#include "ASDU.h"
#include "Utility.h"

using namespace std;

ASDU::ASDU()
{
	clear();
};

ASDU::~ASDU()
{
}

void ASDU::clear()
{
	dui.clear();
	io.clear();
};

int ASDU::get(BYTE* data)
{
	int index=0;
	// Lag DUI
	data[index++]=dui.ident;
	data[index++]=dui.qualifier.Number|(dui.qualifier.SQ<<7);
	data[index++]=dui.cause.cause|(dui.cause.confirm<<6)|(dui.cause.test<<7);
	data[index++]=dui.cause.originator;
	data[index++]=dui.common&0xff;
	data[index++]=(dui.common>>8);
	vector<InformationObject>::iterator it;
	it=io.begin();
	while (it!=io.end())
	{
		if ((it==io.begin()) || !dui.qualifier.SQ)
		{
			memcpy(&data[index],&it->address,3);
			index+=3;
		}
		switch (dui.ident)
		{
			case M_SP_NA_1:
				memcpy(&data[index++],&it->siq,1);
				break;
			case M_DP_NA_1:
				memcpy(&data[index++],&it->diq,1);
				break;
			case M_ME_NA_1:
				memcpy(&data[index],&it->nva,2);
				index+=2;
				memcpy(&data[index++],&it->qds,1);
				break;
			case M_ME_NC_1:
				memcpy(&data[index],&it->value,4);
				index+=4;
				memcpy(&data[index++],&it->qds,1);
				break;
			case M_SP_TB_1:
				memcpy(&data[index++],&it->siq,1);
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
			case M_DP_TB_1:
				memcpy(&data[index++],&it->diq,1);
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
			case M_ME_TD_1:
				memcpy(&data[index],&it->nva,2);
				index+=2;
				memcpy(&data[index++],&it->qds,1);
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
			case M_ME_TF_1:
				memcpy(&data[index],&it->value,4);
				index+=4;
				memcpy(&data[index++],&it->qds,1);
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
			case C_SC_NA_1:
				memcpy(&data[index++],&it->sco,1);
				break;
			case C_DC_NA_1:
				memcpy(&data[index++],&it->dco,1);
				break;
			case C_SC_TA_1:
				memcpy(&data[index++],&it->sco,1);
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
			case C_DC_TA_1:
				memcpy(&data[index++],&it->dco,1);
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
			case M_EI_NA_1:
				memcpy(&data[index++],&it->coi,1);
				break;
			case C_IC_NA_1:
				data[index++]=it->qoi;
				break;
			case C_CI_NA_1:
				data[index++]=it->qcc;
				break;
			case C_CS_NA_1:
				memcpy(&data[index],&it->cp56time,7);
				index+=7;
				break;
		};
		++it;
	}
	return index;
}

void ASDU::set(const BYTE* data)
{
	dui.ident=data[0];
	dui.qualifier.Number=data[1]&0x7f;
	dui.qualifier.SQ=data[1]>>7;
	dui.cause.cause=data[2]&0x3f;
	dui.cause.confirm=(data[2]>>6)&0x01;
	dui.cause.test=data[2]>>7;
	dui.cause.originator=data[3];
	dui.common=data[4]+data[5]*256;
	DWORD i;
	int index=6;
	InformationObject inf;
	for (i=0;i<dui.qualifier.Number;i++)
	{
		if ((i==0) ||(dui.qualifier.SQ==0))
		{
			inf.address=data[index++];
			inf.address+=data[index++]*0x100;
			inf.address+=data[index++]*0x10000;
		}else
		{
			inf.address=io[0].address;
		}

		switch (dui.ident)
		{
			case C_SC_NA_1:
				memset(&inf.sco,data[index++],1);
				break;
			case C_DC_NA_1:
				memset(&inf.dco,data[index++],1);
				break;
			case C_SC_TA_1:
				memset(&inf.sco,data[index++],1);
				memcpy(&inf.cp56time,&data[index],7);
				index+=7;
				break;
			case C_DC_TA_1:
				memset(&inf.dco,data[index++],1);
				memcpy(&inf.cp56time,&data[index],7);
				index+=7;
				break;
			case C_IC_NA_1:
				inf.qoi=data[index++];
				break;
			case C_CI_NA_1:
				inf.qcc=data[index++];
				break;
			case C_CS_NA_1:
				memcpy(&inf.cp56time,&data[index],7);
				index+=7;
				break;
			default:
				// Ikke støttet ident
				return;
		}
		io.push_back(inf);
	}
}

bool ASDU::valid()
{
	return true;
}

int ASDU::addIO(InformationObject& i)
{
	int len=0;
	io.push_back(i);
	dui.qualifier.Number++;
	switch (dui.ident)
	{
		case M_SP_NA_1:
		case M_DP_NA_1:
		case C_SC_NA_1:
		case C_DC_NA_1:
		case M_EI_NA_1:
		case C_IC_NA_1:
		case C_CI_NA_1:
			len=4;
			break;
		case M_ME_NA_1:
			len=6;
			break;
		case M_ME_NC_1:
			len=8;
			break;
		case C_CS_NA_1:
			len=10;
			break;
		case M_SP_TB_1:
		case M_DP_TB_1:
		case C_SC_TA_1:
		case C_DC_TA_1:
			len=11;
			break;
		case M_ME_TD_1:
			len=13;
			break;
		case M_ME_TF_1:
			len=15;
			break;
	};
	if ((dui.qualifier.SQ==1) && (dui.qualifier.Number>1))
		len-=3;
	return len;
}

const SYSTEMTIME cp56time2systemtime(const CP56Time2a& cp)
{
	SYSTEMTIME st;
	st.wYear=cp.y+2000;
	st.wMonth=cp.m;
	st.wDay=cp.d;
	st.wHour=cp.h;
	st.wMinute=cp.min;
	st.wSecond=(WORD)(cp.ms/1000);
	st.wMilliseconds=cp.ms-(st.wSecond*1000);
	st.wDayOfWeek=cp.wd-1;
	return st;
};

const CP56Time2a currentTime()
{
	CP56Time2a cp;
	SYSTEMTIME st;
	GetLocalTime(&st);
	cp.y=st.wYear-2000;
	cp.m=st.wMonth;
	cp.d=st.wDay;
	cp.h=st.wHour;
	cp.min=st.wMinute;
	cp.ms=st.wSecond*1000+st.wMilliseconds;
	cp.wd=st.wDayOfWeek+1;
	cp.IV=0;
	TIME_ZONE_INFORMATION timezone;
	if (GetTimeZoneInformation(&timezone)==TIME_ZONE_ID_DAYLIGHT)
		cp.SU=1;
	else
		cp.SU=0;
	return cp;
};

int unstructured(const std::string& s)
{
	string::size_type i=s.find('.');
	if (i==string::npos)
		return atoi(s.c_str());
	return (atoi(s.substr(0,i).c_str())*256+atoi(s.substr(i+1).c_str()));
};

char* structured(WORD address, char* buffer, int size)
{
	sprintf_s(buffer,size,"%03d.%03d", HIBYTE(address),LOBYTE(address));
	return buffer;
};

