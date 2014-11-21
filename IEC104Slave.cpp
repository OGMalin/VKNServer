#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <iostream>
#include "Utility.h"
#include "IEC104Slave.h"

CRITICAL_SECTION IEC104SlaveCSWrite;
CRITICAL_SECTION IEC104SlaveCSRead;
CRITICAL_SECTION IEC104SlaveCSSpool;
CRITICAL_SECTION IEC104SlaveCSValue;

using namespace std;

IEC104Slave::IEC104Slave(HANDLE event)
{
	debug=false;
	abort=false;
	OkToSend=false;
	commonaddress=1;
	port=2404; // Standard port for IEC 104 kommunikasjon
	slaveIp=INADDR_ANY; // Velg hvilken egen ip som skal brukes, default er alle definerte.
	hThread=NULL;
	currentSSN=currentRSN=0;
	// Lag en event som 'main' kan lytte på når noe skal sendes til andre funksjoner (kommando).
	hEvent=event;
	InitializeCriticalSection(&IEC104SlaveCSWrite);
	InitializeCriticalSection(&IEC104SlaveCSRead);
	InitializeCriticalSection(&IEC104SlaveCSSpool);
	InitializeCriticalSection(&IEC104SlaveCSValue);
}

IEC104Slave::~IEC104Slave()
{
	stop();
	DeleteCriticalSection(&IEC104SlaveCSWrite);
	DeleteCriticalSection(&IEC104SlaveCSRead);
	DeleteCriticalSection(&IEC104SlaveCSSpool);
	DeleteCriticalSection(&IEC104SlaveCSValue);
}

bool IEC104Slave::start()
{
	// Kjører allerede
	if (hThread)
		return false;

	sockaddr_in slaveAddr;
	abort=false;

	// Lag en nettverk socket med ipv4 og tcp
	if ((slaveSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		cout << "socket() feilet." << endl;
		return false;
	}

	// Velg hvilken IP og port som skal brukes.
	memset(&slaveAddr, 0, sizeof(slaveAddr));
	slaveAddr.sin_family = AF_INET;
	slaveAddr.sin_addr.s_addr = htonl(slaveIp);
	slaveAddr.sin_port = htons(port);

	if (bind(slaveSocket, (struct sockaddr*) &slaveAddr, sizeof(slaveAddr)) < 0)
	{
		cout << "bind() feilet." << endl;
		return false;
	}

	// Forbered for innkommende anrop
	if (listen(slaveSocket,5) <0)
	{
		cout << "listen() feilet." << endl;
		return false;
	}

	cout << "IEC104Slave startet, Ip: " << inet_ntoa(slaveAddr.sin_addr) << " Port: " << port << endl;

  hThread=(HANDLE)_beginthread(IEC104Slave::threadLoop,0,this);
  if ((int)hThread==-1)
	{
		cout << "Får ikke startet tråd for å lytte etter master." << endl;
		abort=true;
		hThread=NULL;
		return false;
	}

	// Rapporter 'End of initialization'
	APDU apdu;
	apdu.setAPCI(I_FORMAT);
	apdu.setDUI(commonaddress, M_EI_NA_1,COT_INITIALISED);
	apdu.addIO(0);
	OkToSend?write(apdu):spool(apdu);
	return true;
}

void IEC104Slave::stop()
{
	if (threadRunning(hThread))
	{
		APDU apdu;
		apdu.apci.format=U_FORMAT;
		apdu.apci.func=STOPDTACT;
		write(apdu);
		OkToSend=false;

		abort=true;
		if (WaitForSingleObject(hThread, 1000) == WAIT_TIMEOUT)
			TerminateThread(hThread,0);
		hThread=NULL;
		if (masterSocket!=-1)
			closesocket(masterSocket);
		if (slaveSocket != -1)
			closesocket(slaveSocket);

	}
}

void IEC104Slave::input(BYTE* data)
{
	// Iec 104 telegram?
	if (data[0]!=0x68)
		return;

	APDU apdu,res;
	InformationObject inf;
	apdu.set(data);
	if (!apdu.valid())
		return;

	switch (apdu.apci.format)
	{
		case I_FORMAT:
			switch (apdu.asdu.dui.ident)
			{
				case C_SC_NA_1:
					break;
				case C_DC_NA_1:
					break;
				case C_SC_TA_1:
					break;
				case C_DC_TA_1:
					break;
				case C_IC_NA_1:
					res.setAPCI(I_FORMAT);
					res.setDUI(commonaddress, C_IC_NA_1,COT_ACTIVATIONCONFIRM);
					inf.address=0;
					inf.qoi=QOI_STATION_INTERROGATION;
					res.addIO(inf);
					OkToSend?write(res):spool(res);
					sendInterrogation();
					res.clear();
					res.setAPCI(I_FORMAT);
					res.setDUI(commonaddress, C_IC_NA_1,COT_ACTIVATIONTERMINATION);
					inf.clear();
					inf.address=0;
					inf.qoi=QOI_STATION_INTERROGATION;
					res.addIO(inf);
					OkToSend?write(res):spool(res);
					break;
				case C_CI_NA_1:
					break;
				case C_CS_NA_1:
					// Klokke synkronisering.
					break;
			}
			currentRSN=apdu.apci.ssn+1;
			break;
		case S_FORMAT:
//			if (currentSSN!=apdu.apci.rsn)
//			{
//				
//			}
			break;
		case U_FORMAT:
			res.apci.format=U_FORMAT;
			switch (apdu.apci.func)
			{
				case STARTDTACT:
					res.apci.func=STARTDTCON;
					OkToSend=true;
					write(res);
					spool();
					break;
				case STOPDTACT:
					res.apci.func=STOPDTCON;
					write(res);
					OkToSend=false;
					break;
				case TESTFRACT:
					res.apci.func=TESTFRCON;
					write(res);
					break;
			}
			break;
	}
	/* For kommandoer etc. 
	EnterCriticalSection(&IEC104SlaveCS);
	inQue.push_back(apdu);
	LeaveCriticalSection(&IEC104SlaveCS);
	SetEvent(hEvent);
	*/

}

bool IEC104Slave::read(APDU& apdu)
{
	bool ret=false;
	EnterCriticalSection(&IEC104SlaveCSRead);
	if (!inQue.empty())
	{
		apdu=inQue.front();
		inQue.pop();
		ret=true;
	}
	LeaveCriticalSection(&IEC104SlaveCSRead);
	return ret;
}

bool IEC104Slave::write(APDU& apdu)
{
	BYTE data[255];
	int len;
	if (!masterSocket)
		return false;
	if (apdu.apci.format==I_FORMAT)
	{
		apdu.apci.ssn=currentSSN++;
		apdu.apci.rsn=currentRSN;
	}
	len=apdu.get(data);
	if (len)
	{
		EnterCriticalSection(&IEC104SlaveCSWrite);
		
		// Ta vare på de 255 siste sendte meldinger i tilfelle kommunikasjonsfeil.
		if (apdu.apci.format==I_FORMAT)
		{
			sendQue.push_back(apdu);
			if (sendQue.size()>255)
				sendQue.pop_front();
		}

		if (debug)
			cerr << makeHexString(data,len) << endl;
		send(masterSocket,(char*)data,len,0);
		LeaveCriticalSection(&IEC104SlaveCSWrite);
	}
	return true;
}


void IEC104Slave::threadLoop(void* lpv)
{
	int masterLen;
	int recvLen;
	sockaddr_in masterAddr;
	BYTE data[255];
  IEC104Slave* slave=(IEC104Slave*)lpv;
	slave->masterSocket=-1;
	while (!slave->abort)
	{
		masterLen=sizeof(masterAddr);
		// Vent på anrop
		if ((slave->masterSocket = accept(slave->slaveSocket, (struct sockaddr*) & masterAddr, (int*)&masterLen)) < 0)
		{
			cerr << "IEC104: accept() feilet." << endl;
			break;
		}

		cout << "IEC104 koplet til " << inet_ntoa(masterAddr.sin_addr) << endl;
		recvLen=1;
		while (recvLen>0)
		{
			if ((recvLen=recv(slave->masterSocket, (char*)data, 255, 0)) < 0)
			{
				cerr << "IEC104: recv() feilet." << endl;
				break;
			}

			if (slave->debug)
				cerr << makeHexString(data,recvLen) << endl;
			slave->input(data);
		}
		closesocket(slave->masterSocket);
		slave->masterSocket=-1;
	}
  _endthread();
}

void IEC104Slave::setVal(int ident, int addr, float val)
{
	vector<IECValue>::iterator sit;
	list<APDU>::iterator oit;
	APDU apdu;
	ASDU asdu;
	InformationObject inf;

	// Lagre verdien
	EnterCriticalSection(&IEC104SlaveCSValue);
	sit=stored.begin();
	while (sit!=stored.end())
	{
		if (sit->id==addr)
		{
			if (sit->fVal==val)
			{
				LeaveCriticalSection(&IEC104SlaveCSValue);
				return;
			}
			sit->fVal=val;
			break;
		}
		++sit;
	}
	if (sit==stored.end())
	{
		IECValue v;
		v.id=addr;
		v.type=M_ME_NC_1;
		v.fVal=val;
		stored.push_back(v);
	}
	LeaveCriticalSection(&IEC104SlaveCSValue);

	// Lag meldingen
	apdu.setAPCI(I_FORMAT);
	apdu.setDUI(commonaddress,ident,COT_SPONTANEOUS);
	inf.address=addr;
	inf.value=val;
	inf.cp56time=currentTime();
	apdu.addIO(inf);
	
	// Sjekk om samme måling er i køen for sending til frontend
	EnterCriticalSection(&IEC104SlaveCSSpool);
	oit=outQue.begin();
	while (oit!=outQue.end())
	{
		if ((oit->asdu.dui.ident==ident) && oit->asdu.dui.qualifier.Number==1)
		{
			if (oit->asdu.io[0].address==addr)
			{
				outQue.erase(oit);
				break;
			}
		}
		++oit;
	}
	LeaveCriticalSection(&IEC104SlaveCSSpool);

	// Send meldingen
	OkToSend?write(apdu):spool(apdu);
}

void IEC104Slave::setNva(int ident, int addr, int nva)
{
	vector<IECValue>::iterator sit;
	list<APDU>::iterator oit;
	APDU apdu;
	ASDU asdu;
	InformationObject inf;

	// Lagre verdien
	EnterCriticalSection(&IEC104SlaveCSValue);
	sit=stored.begin();
	while (sit!=stored.end())
	{
		if (sit->id==addr)
		{
			if (sit->nVal==nva)
			{
				LeaveCriticalSection(&IEC104SlaveCSValue);
				return;
			}
			sit->nVal=nva;
			break;
		}
		++sit;
	}
	if (sit==stored.end())
	{
		IECValue v;
		v.id=addr;
		v.type=M_ME_NA_1;
		v.nVal=nva;
		stored.push_back(v);
	}
	LeaveCriticalSection(&IEC104SlaveCSValue);

	// Lag meldingen
	apdu.setAPCI(I_FORMAT);
	apdu.setDUI(commonaddress,ident,COT_SPONTANEOUS);
	inf.address=addr;
	inf.nva=nva;
	inf.cp56time=currentTime();
	apdu.addIO(inf);
	
	// Sjekk om samme måling er i køen for sending til frontend
	EnterCriticalSection(&IEC104SlaveCSSpool);
	oit=outQue.begin();
	while (oit!=outQue.end())
	{
		if ((oit->asdu.dui.ident==ident) && oit->asdu.dui.qualifier.Number==1)
		{
			if (oit->asdu.io[0].address==addr)
			{
				outQue.erase(oit);
				break;
			}
		}
		++oit;
	}
	LeaveCriticalSection(&IEC104SlaveCSSpool);

	// Send meldingen
	OkToSend?write(apdu):spool(apdu);
}

void IEC104Slave::setSpi(int ident, int addr, int spi)
{
	bool exist=false;
	vector<IECValue>::iterator sit;
	list<APDU>::iterator oit;
	APDU apdu;
	ASDU asdu;
	InformationObject inf;

	// Lagre verdien
	EnterCriticalSection(&IEC104SlaveCSValue);
	sit=stored.begin();
	while (sit!=stored.end())
	{
		if (sit->id==addr)
		{
			if (sit->nVal==spi)
			{
				LeaveCriticalSection(&IEC104SlaveCSValue);
				return;
			}
			sit->nVal=spi;
			break;
		}
		++sit;
	}
	if (sit==stored.end())
	{
		IECValue v;
		v.id=addr;
		v.type=M_SP_NA_1;
		v.nVal=spi;
		stored.push_back(v);
	}
	LeaveCriticalSection(&IEC104SlaveCSValue);

	// Lag meldingen
	apdu.setAPCI(I_FORMAT);
	apdu.setDUI(commonaddress,ident,COT_SPONTANEOUS);
	inf.address=addr;
	inf.siq.SPI=spi;
	inf.cp56time=currentTime();
	apdu.addIO(inf);
	
	// Send meldingen
	OkToSend?write(apdu):spool(apdu);
}

void IEC104Slave::setDpi(int ident, int addr, int dpi)
{
	bool exist=false;
	vector<IECValue>::iterator sit;
	list<APDU>::iterator oit;
	APDU apdu;
	ASDU asdu;
	InformationObject inf;

	// Lagre verdien
	EnterCriticalSection(&IEC104SlaveCSValue);
	sit=stored.begin();
	while (sit!=stored.end())
	{
		if (sit->id==addr)
		{
			if (sit->nVal==dpi)
			{
				LeaveCriticalSection(&IEC104SlaveCSValue);
				return;
			}
			sit->nVal=dpi;
			break;
		}
		++sit;
	}
	if (sit==stored.end())
	{
		IECValue v;
		v.id=addr;
		v.type=M_SP_NA_1;
		v.nVal=dpi;
		stored.push_back(v);
	}
	LeaveCriticalSection(&IEC104SlaveCSValue);

	// Lag meldingen
	apdu.setAPCI(I_FORMAT);
	apdu.setDUI(commonaddress,ident,COT_SPONTANEOUS);
	inf.address=addr;
	inf.diq.DPI=dpi;
	inf.cp56time=currentTime();
	apdu.addIO(inf);
	
	// Send meldingen
	OkToSend?write(apdu):spool(apdu);
}

void IEC104Slave::spool()
{
	if (!OkToSend)
		return;

	EnterCriticalSection(&IEC104SlaveCSSpool);
	list<APDU>::iterator it = outQue.begin();
	while (it!=outQue.end())
	{
		write(*it);
		++it;
	}
	outQue.clear();
	LeaveCriticalSection(&IEC104SlaveCSSpool);
}

void IEC104Slave::spool(APDU& apdu)
{
	EnterCriticalSection(&IEC104SlaveCSSpool);
	outQue.push_back(apdu);
	LeaveCriticalSection(&IEC104SlaveCSSpool);
}

void IEC104Slave::sendInterrogation()
{
	int n;
	APDU apdu;
	vector<IECValue>::iterator it;
	InformationObject inf;

	EnterCriticalSection(&IEC104SlaveCSValue);

	// Send enkelt meldinger
	it=stored.begin();
	n=0;
	while (it!=stored.end())
	{
		if (it->type==M_SP_NA_1)
		{
			if (n==0)
			{
				apdu.clear();
				apdu.setAPCI(I_FORMAT);
				apdu.setDUI(commonaddress, M_SP_NA_1,COT_INTERROGATION);
			}
			inf.clear();
			inf.address=it->id;
			inf.siq.SPI=it->nVal;
			apdu.addIO(inf);
			n++;
			if (n==10)
			{
				write(apdu);
				n=0;
			}
		}
		++it;
	}
	if ((n>0) && (n<10))
				write(apdu);

	// Send doble meldinger
	it=stored.begin();
	n=0;
	while (it!=stored.end())
	{
		if (it->type==M_DP_NA_1)
		{
			if (n==0)
			{
				apdu.clear();
				apdu.setAPCI(I_FORMAT);
				apdu.setDUI(commonaddress, M_DP_NA_1,COT_INTERROGATION);
			}
			inf.clear();
			inf.address=it->id;
			inf.diq.DPI=it->nVal;
			apdu.addIO(inf);
			n++;
			if (n==10)
			{
				write(apdu);
				n=0;
			}
		}
		++it;
	}
	if ((n>0) && (n<10))
				write(apdu);

	// Send normalised målinger
	it=stored.begin();
	n=0;
	while (it!=stored.end())
	{
		if (it->type==M_ME_NA_1)
		{
			if (n==0)
			{
				apdu.clear();
				apdu.setAPCI(I_FORMAT);
				apdu.setDUI(commonaddress, M_ME_NA_1,COT_INTERROGATION);
			}
			inf.clear();
			inf.address=it->id;
			inf.nva=it->nVal;
			apdu.addIO(inf);
			n++;
			if (n==10)
			{
				write(apdu);
				n=0;
			}
		}
		++it;
	}
	if ((n>0) && (n<10))
				write(apdu);

	// Send floating point målinger
	it=stored.begin();
	n=0;
	while (it!=stored.end())
	{
		if (it->type==M_ME_NC_1)
		{
			if (n==0)
			{
				apdu.clear();
				apdu.setAPCI(I_FORMAT);
				apdu.setDUI(commonaddress, M_ME_NC_1,COT_INTERROGATION);
			}
			inf.clear();
			inf.address=it->id;
			inf.value=it->fVal;
			apdu.addIO(inf);
			n++;
			if (n==10)
			{
				write(apdu);
				n=0;
			}
		}
		++it;
	}
	if ((n>0) && (n<10))
				write(apdu);

	LeaveCriticalSection(&IEC104SlaveCSValue);
}