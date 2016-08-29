#pragma once

#include <Windows.h>
#include <list>
#include <queue>
#include "APDU.h"

struct IECValue
{
	int id;
	int type; // ASDU type
	union
	{
		float fVal;
		int		nVal;
	};
};

class IEC104Slave
{
public:
	HANDLE hThread;
	HANDLE hEvent;
	bool debug;
	bool abort;
//	SOCKET slaveSocket;
	SOCKET clientSocket;
	std::string port;
	bool OkToSend;
	unsigned int slaveIp;
	int commonaddress;
	std::queue<APDU> inQue;		// Meldinger som venter p� � bli lest.
	std::list<APDU> outQue;		// Meldinger som venter p� � bli sendt.
	std::list<APDU> sendQue;	// De siste sendte meldinger.

	std::vector<IECValue> stored;		// Sist registrerte verdier for meldinger/m�linger

	DWORD slaveSN; // Next sequencenumber for message to master
	DWORD masterSN; // Number of seqences from master
	IEC104Slave(HANDLE event);
	virtual ~IEC104Slave();

	static void threadLoop(void* lpv);
	void input(BYTE* data);
//	const TPCTelegram getResponse(const TPCTelegram& data);

	bool start();
	void stop();
	bool read(APDU& apdu);
	// Sender melding til master
	bool write(APDU& apdu);
	void setSpi(int ident, int addr, int spi);
	void setDpi(int ident, int addr, int dpi);
	void setNva(int ident, int addr, int nva);
	void setVal(int ident, int addr, float val);
	// Pr�ver � sende meldingene i utk�en
	void spool();
	// Legger en melding i utk�en
	void spool(APDU& apdu);
	void sendInterrogation();
	void emptyQues();
};