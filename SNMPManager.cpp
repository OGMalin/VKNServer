#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <iostream>
#include "Utility.h"
#include "SNMPManager.h"
#include "SNMP.h"

CRITICAL_SECTION SNMPManagerCS;

using namespace std;

SNMPManager::SNMPManager(HANDLE event)
{
	debug=false;
	abort=false;
	trapPort=162; // Standard port for SNMP TRAP
	trapIp=INADDR_ANY; // Velg hvilken egen ip som skal brukes, default er alle definerte.
	responsePort=161; // Standard port for SNMP response
	responseIp=INADDR_ANY; // Velg hvilken egen ip som skal brukes, default er alle definerte.
	hResponse=NULL;
	hTrap=NULL;
	requestId=1;
	running=false;
	// Lag en event som 'main' kan lytte på når noe skal sendes til andre funksjoner (kommando).
	hEvent=event;
	InitializeCriticalSection(&SNMPManagerCS);
}

SNMPManager::~SNMPManager()
{
	stop();
	DeleteCriticalSection(&SNMPManagerCS);
}

bool SNMPManager::start()
{
	// Kjører allerede
	if (!threadRunning(hTrap))
	{
		// Lag en nettverk socket med ipv4 og tcp for trap mottaker
		if ((trapSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		{
			cout << "socket() feilet." << endl;
			return false;
		}

		abort = false;

		hTrap=(HANDLE)_beginthread(SNMPManager::trapLoop,0,this);
		if ((int)hTrap==-1)
		{
			cout << "Får ikke startet tråd for å lytte etter SNMP trap." << endl;
			abort=true;
			hTrap=NULL;
			return false;
		}
	}
	if (!threadRunning(hResponse))
	{
		// Lag en nettverk socket med ipv4 og tcp
		if ((responseSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		{
			cout << "response: socket() feilet." << endl;
			return false;
		}

		abort = false;

		hResponse=(HANDLE)_beginthread(SNMPManager::responseLoop,0,this);
		if ((int)hResponse==-1)
		{
			cout << "Får ikke startet tråd for å lytte etter SNMP response." << endl;
			abort=true;
			hTrap=NULL;
			return false;
		}
	}
	running=true;
	return true;
}

void SNMPManager::stop()
{
	if (threadRunning(hTrap))
	{
		abort=true;
		if (WaitForSingleObject(hTrap,1000)==WAIT_TIMEOUT)
			TerminateThread(hTrap,0);
		hTrap=NULL;
		if (trapSocket != -1)
			closesocket(trapSocket);
	}
	if (threadRunning(hResponse))
	{
		abort=true;
		if (WaitForSingleObject(hResponse,1000)==WAIT_TIMEOUT)
			TerminateThread(hResponse,0);
		if (responseSocket!=-1)
			closesocket(responseSocket);
		hResponse = NULL;
	}
	running=false;
}

void SNMPManager::input(const BYTE* data, int len, unsigned long addr)
{
	SNMP snmp;
	if (!snmp.set(data, len, addr))
	{
		cerr << "Feil i SNMP Trap melding fra: " << (int)snmp.cip.a << "." << (int)snmp.cip.b << "." << (int)snmp.cip.c << "." << (int)snmp.cip.d << endl;
		return;
	}
	if (debug)
		cout << snmp.toString() << endl;

	EnterCriticalSection(&SNMPManagerCS);
	std::vector<SNMPVariable>::iterator vit=snmp.variable.begin();
	SNMPTelegram t;
	t.genericTrap=snmp.generictrap;
	t.ip=snmp.ip;
	t.type=snmp.pdu;
	t.specificTrap=snmp.specifictrap;
	t.error_status=snmp.errorstatus;
	if (vit!=snmp.variable.end())
	{
		while (vit!=snmp.variable.end())
		{
			t.object=vit->id;
			t.value=vit->value;
			inQue.push(t);
			++vit;
		}
	}else
	{
		t.object.clear();
		t.value.clear();
		inQue.push(t);
	}
	LeaveCriticalSection(&SNMPManagerCS);
	SetEvent(hEvent);
}

void SNMPManager::trapLoop(void* lpv)
{
	int agentLen;
	int recvLen;
	sockaddr_in agentAddr;
	BYTE data[255];
  SNMPManager* trap=(SNMPManager*)lpv;

	sockaddr_in trapAddr;

	// Velg hvilken IP og port som skal brukes.
	memset(&trapAddr, 0, sizeof(trapAddr));
	trapAddr.sin_family = AF_INET;
	trapAddr.sin_addr.s_addr = htonl(trap->trapIp);
	trapAddr.sin_port = htons(trap->trapPort);

	if (bind(trap->trapSocket, (struct sockaddr*) &trapAddr, sizeof(trapAddr)) < 0)
	{
		cout << "bind() feilet." << endl;
		return;
	}

	cout << "SNMP trap server startet, Ip: " << inet_ntoa(trapAddr.sin_addr) << " Port: " << trap->trapPort << endl;

	while (!trap->abort)
	{
		agentLen=sizeof(agentAddr);
		if ((recvLen=recvfrom(trap->trapSocket, (char*)data, 255, 0, (struct sockaddr*)&agentAddr, &agentLen)) < 0)
		{
			cerr << "recvfrom() feilet." << endl;
		}else
		{
			if (trap->debug)
				cerr << "T> " << inet_ntoa(agentAddr.sin_addr) << " " << makeHexString(data,recvLen) << endl;
			trap->input(data, recvLen, ntohl(agentAddr.sin_addr.S_un.S_addr));
		}
	}
  _endthread();
}

void SNMPManager::responseLoop(void* lpv)
{
	int agentLen;
	int recvLen;
	sockaddr_in agentAddr;
	BYTE data[255];
  SNMPManager* response=(SNMPManager*)lpv;

	sockaddr_in responseAddr;

	// Velg hvilken IP og port som skal brukes.
	memset(&responseAddr, 0, sizeof(responseAddr));
	responseAddr.sin_family = AF_INET;
	responseAddr.sin_addr.s_addr = htonl(response->responseIp);
	responseAddr.sin_port = htons(response->responsePort);

	if (bind(response->responseSocket, (struct sockaddr*) &responseAddr, sizeof(responseAddr)) < 0)
	{
		cout << "response: bind() feilet. Errorkode: " <<  WSAGetLastError() << endl;
		return;
	}

	cout << "SNMP response server startet, Ip: " << inet_ntoa(responseAddr.sin_addr) << " Port: " << response->responsePort << endl;

	while (!response->abort)
	{
		agentLen=sizeof(agentAddr);
		if ((recvLen=recvfrom(response->responseSocket, (char*)data, 255, 0, (struct sockaddr*)&agentAddr, &agentLen)) < 0)
		{
			cerr << "response: recvfrom() feilet." << endl;
		}else
		{
			if (response->debug)
				cerr << "T> " << inet_ntoa(agentAddr.sin_addr) << " " << makeHexString(data,recvLen) << endl;
			response->input(data, recvLen, ntohl(agentAddr.sin_addr.S_un.S_addr));
		}
	}
  _endthread();
}

void SNMPManager::send(DWORD ip, BYTE* data, int len)
{
	if (!threadRunning(hResponse))
		return;

	sockaddr_in agentAddr;

	memset(&agentAddr, 0, sizeof(agentAddr));
	agentAddr.sin_addr.s_addr = htonl(ip);
	agentAddr.sin_family =AF_INET;
	agentAddr.sin_port = htons(161);
	if (sendto(responseSocket, (const char*)&data[0],len,0,(struct sockaddr*) & agentAddr, sizeof(agentAddr))==SOCKET_ERROR)
	{
		cerr << "sendto() feilet. Errorkode: " <<  WSAGetLastError() << endl;
	}
}

bool SNMPManager::read(SNMPTelegram& t)
{
	bool ret=false;
	EnterCriticalSection(&SNMPManagerCS);
	if (!inQue.empty())
	{
		t=inQue.front();
		inQue.pop();
		ret = true;
	}
	LeaveCriticalSection(&SNMPManagerCS);
	return ret;
}

void SNMPManager::getRequest(DWORD ip, std::string object, std::string value, int version, std::string community)
{
	ASN asn;
	ASN ver;
	ASN com;
	ASN pdu;
	ASN recid;
	ASN errstat;
	ASN errind;
	ASN bnd;
	ASN var;
	ASN obj;
	ASN val;

	// Sett objektet det spørres om
	obj.set(ASN_OBJECTIDENTIFIER,object);
	if (lowercase(value)=="null")
	{
		val.set(ASN_NULL,0);
	}else
	{
		if (isNumber(value))
			val.set(ASN_INTEGER,value);
		else
			val.set(ASN_OCTETSTRING,value);
	}

	// Legg det til en variabel
	var.set(ASN_SEQUENCE);
	var.add(obj);
	var.add(val);

	// Legg alle variabler inn i en binding
	bnd.set(ASN_SEQUENCE);
	bnd.add(var);

	// Errorkode
	errind.set(ASN_INTEGER,0);

	// Error status
	errstat.set(ASN_INTEGER,0);

	// Request id
	if (requestId>=MAXINT)
		requestId=1;
	else
		requestId++;
	recid.set(ASN_INTEGER,requestId++);

	// Legg det inn i en pdu
	pdu.set(ASN_CONTEXTSPECIFIC, ASN_CONSTRUCTED, PDU_GETREQUEST);
	pdu.add(recid);
	pdu.add(errstat);
	pdu.add(errind);
	pdu.add(bnd);

	// Community string
	com.set(ASN_OCTETSTRING,community);

	// Trap versjon
	ver.set(ASN_INTEGER,(version-1));

	asn.set(ASN_SEQUENCE);
	asn.add(ver);
	asn.add(com);
	asn.add(pdu);

	BYTE data[512];
	int len=asn.get(data);
	if (debug)
		cout << "SNMP< " << makeHexString(data,len) << endl;
	send(ip,data,len);
}

void SNMPManager::setRequest(DWORD ip, std::string object, std::string value, int version, std::string community)
{
	ASN asn;
	ASN ver;
	ASN com;
	ASN pdu;
	ASN recid;
	ASN errstat;
	ASN errind;
	ASN bnd;
	ASN var;
	ASN obj;
	ASN val;

	// Sett objektet det spørres om
	obj.set(ASN_OBJECTIDENTIFIER, object);
	if (lowercase(value) == "null")
	{
		val.set(ASN_NULL, 0);
	}else
	{
		if (isNumber(value))
			val.set(ASN_INTEGER, value);
		else
			val.set(ASN_OCTETSTRING, value);
	}
	// Legg det til en variabel
	var.set(ASN_SEQUENCE);
	var.add(obj);
	var.add(val);

	// Legg alle variabler inn i en binding
	bnd.set(ASN_SEQUENCE);
	bnd.add(var);

	// Errorkode
	errind.set(ASN_INTEGER, 0);

	// Error status
	errstat.set(ASN_INTEGER, 0);

	// Request id
	if (requestId >= MAXINT)
		requestId = 1;
	else
		requestId++;
	recid.set(ASN_INTEGER, requestId++);

	// Legg det inn i en pdu
	pdu.set(ASN_CONTEXTSPECIFIC, ASN_CONSTRUCTED, PDU_SETREQUEST);
	pdu.add(recid);
	pdu.add(errstat);
	pdu.add(errind);
	pdu.add(bnd);

	// Community string
	com.set(ASN_OCTETSTRING, community);

	// Trap versjon
	ver.set(ASN_INTEGER, (version - 1));

	asn.set(ASN_SEQUENCE);
	asn.add(ver);
	asn.add(com);
	asn.add(pdu);

	BYTE data[512];
	int len = asn.get(data);
	if (debug)
		cout << "SNMP< " << makeHexString(data, len) << endl;
	send(ip, data, len);
}

bool SNMPManager::get(DWORD ip, std::string obj)
{
	if (!running)
		return false;
	if (ip==0)
		return false;
	if (obj.empty())
		return false;
	if (!isNumber(obj))
		return false;
	getRequest(ip,obj);
	return true;
}

bool SNMPManager::set(DWORD ip, std::string obj, std::string val)
{
	if (!running)
		return false;
	if (ip == 0)
		return false;
	if (obj.empty())
		return false;
	if (!isNumber(obj))
		return false;
	setRequest(ip, obj, val);
	return true;
}
