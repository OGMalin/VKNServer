#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <WS2tcpip.h>
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
	port="2404"; // Standard port for IEC 104 kommunikasjon
	slaveIp=INADDR_ANY; // Velg hvilken egen ip som skal brukes, default er alle definerte.
	hThread=NULL;
	slaveSN=masterSN=0;
	// Lag en event som 'main' kan lytte på når noe skal sendes til andre funksjoner (kommando).
	hEvent=event;
	InitializeCriticalSection(&IEC104SlaveCSWrite);
	InitializeCriticalSection(&IEC104SlaveCSRead);
	InitializeCriticalSection(&IEC104SlaveCSSpool);
	InitializeCriticalSection(&IEC104SlaveCSValue);
	clientSocket = INVALID_SOCKET;
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

//	sockaddr_in slaveAddr;
	abort=false;
	OkToSend = false;

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
		if (clientSocket!=INVALID_SOCKET)
			closesocket(clientSocket);

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
			if (masterSN != apdu.apci.ssn)
				cerr << "IEC: Feil master sequencenumber." << " Expected: " << masterSN << " got: " << apdu.apci.ssn << endl;
			masterSN++;
			switch (apdu.asdu.dui.ident)
			{
				case C_SC_NA_1: // Enkel kommando
				case C_DC_NA_1: // Dobbel kommando
				case C_RC_NA_1: // Regulering
				case C_SC_TA_1: // Enkel kommando med tid
				case C_DC_TA_1: // Dobbel kommando med tid
				case C_RC_TA_1: // Regulerering med tid
					if (apdu.asdu.io.size() != 1)
						break;
					EnterCriticalSection(&IEC104SlaveCSRead);
					inQue.push(apdu);
					LeaveCriticalSection(&IEC104SlaveCSRead);
					SetEvent(hEvent);
					res.setAPCI(I_FORMAT);
					res.setDUI(commonaddress, apdu.asdu.dui.ident, COT_ACTIVATIONCONFIRM);
					inf = apdu.asdu.io[0];
					res.addIO(inf);
					OkToSend ? write(res) : spool(res);
					res.clear();
					res.setAPCI(I_FORMAT);
					res.setDUI(commonaddress, apdu.asdu.dui.ident, COT_ACTIVATIONTERMINATION);
					inf = apdu.asdu.io[0];
					res.addIO(inf);
					OkToSend ? write(res) : spool(res);
					break;
				case C_IC_NA_1: // Interrogation
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
				case C_CI_NA_1: // Counter interrogation
					break;
				case C_CS_NA_1: // Klokke synkronisering.
					res.setAPCI(I_FORMAT);
					res.setDUI(commonaddress, C_CS_NA_1, COT_ACTIVATIONCONFIRM);
					inf.address = 0;
					inf.cp56time = currentTime();
					res.addIO(inf);
					OkToSend ? write(res) : spool(res);
					break;
			}
			break;
		case S_FORMAT:
			if (((int)slaveSN > apdu.apci.rsn + 2) || ((int)slaveSN < apdu.apci.rsn))
				cerr << "IEC: Feil slave sequencenumber." << " Expected: " << slaveSN << " got: " << apdu.apci.rsn << endl;
//			masterSN++;
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
	if (!clientSocket)
		return false;
	if (apdu.apci.format==I_FORMAT)
	{
		apdu.apci.ssn = slaveSN++;
		apdu.apci.rsn = masterSN;
		if (slaveSN > 32767)
			slaveSN = 0;
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
		send(clientSocket, (char*)data, len, 0);
		LeaveCriticalSection(&IEC104SlaveCSWrite);
		Sleep(30); // Venter 30 msec slik at meldingene ikke kommer for tett. (spessielt svar på IC)
	}
	return true;
}


void IEC104Slave::threadLoop(void* lpv)
{
	int iResult;
	SOCKET listenSocket=INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	char recvbuf[512];
	int recvbuflen = 512;

  IEC104Slave* slave=(IEC104Slave*)lpv;

	while (!slave->abort)
	{
		slave->emptyQues();
		slave->OkToSend = false;
		slave->masterSN = slave->slaveSN = 0;
		slave->clientSocket = INVALID_SOCKET;
		listenSocket = INVALID_SOCKET;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		iResult = getaddrinfo(NULL, slave->port.c_str(), &hints, &result);
		if (iResult != 0)
		{
			cerr << "IEC104: getaddrinfo feilet." << endl;
			break;
		}

		listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (listenSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			cerr << "IEC104: Feilet i listenSocket = socket(...)" << endl;
			break;
		}

		iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			cerr << "IEC104: Feilet med bind:" << endl;
			freeaddrinfo(result);
			closesocket(listenSocket);
			break;
		}

		freeaddrinfo(result);

		iResult = listen(listenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			cerr << "IEC104: Feilet med listen:" << endl;
			closesocket(listenSocket);
			break;
		}

		// Vent på anrop
		slave->clientSocket = accept(listenSocket, NULL, NULL);
		if (slave->clientSocket==INVALID_SOCKET)
		{
			cerr << "IEC104: accept() feilet." << endl;
			closesocket(listenSocket);
			break;
		}

		closesocket(listenSocket);

		cout << "IEC104 tilkoplet. " << endl;

		do
		{
			iResult = recv(slave->clientSocket, recvbuf, recvbuflen, 0);
			if (iResult>0)
			{
				if (slave->debug)
					cerr << makeHexString((BYTE*)recvbuf, iResult) << endl;
				slave->input((BYTE*)recvbuf);
			}
			else if (iResult == 0)
			{
				cerr << "IEC104: Lukke forbindelsen." << endl;
			}else
			{
				cerr << "IEC104: recv() feilet." << endl;
				closesocket(slave->clientSocket);
				break;
			}

		} while (iResult > 0);

		iResult = shutdown(slave->clientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			cerr << "IEC104: shutdown feilet." << endl;
			closesocket(slave->clientSocket);
			break;
		}
		closesocket(slave->clientSocket);
		slave->clientSocket = INVALID_SOCKET;
	}
	slave->OkToSend = false;
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
		v.type=M_DP_NA_1;
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

void IEC104Slave::emptyQues()
{
	EnterCriticalSection(&IEC104SlaveCSSpool);
	outQue.clear();
	LeaveCriticalSection(&IEC104SlaveCSSpool);
	EnterCriticalSection(&IEC104SlaveCSWrite);
	sendQue.clear();
	LeaveCriticalSection(&IEC104SlaveCSWrite);
}
