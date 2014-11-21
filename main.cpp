// Serverprogram

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <map>
#include <stdlib.h>
#include "Utility.h"
#include "Console.h"
#include "IEC104Slave.h"
#include "SNMPManager.h"
#include "SNMP.h"
#include "weather.h"

using namespace std;

enum {WAIT_CONSOLE=WAIT_OBJECT_0,WAIT_104,WAIT_SNMP,WAIT_WEATHER};
enum {CMAIN=0, C104, CSNMP, CWEATHER };


struct ScheduleEntry
{
	std::string command;
	DWORD stime;
	DWORD tick;
};

struct SNMPCommand
{
	std::string station;
	int pdu;
	std::string object;
	std::string cmd;
	int generic_trap;
	int spesific_trap;
	std::string value;
};

struct SNMPResponse
{
	std::string name;
	DWORD stime;
	DWORD id;
	DWORD tick;
	DWORD last; // 1=feil, 0=ok, 2=oppstart
};

struct WeatherCommand
{
	std::string type;
	std::string location;
	std::string cmd;
};

struct StationMap
{
	std::string name;
	std::list<DWORD> ip;
};

struct LocationMap
{
	std::string name;
	double latitude;
	double longitude;
};

Console con;
IEC104Slave* iec104;
SNMPManager* snmp;
Weather* weather;
HANDLE hEvent[4];

int cmdsection;
bool logging;
bool useScheduler;
std::string logfile;
std::list<ScheduleEntry> scheduleQue;
std::list<SNMPCommand> snmpQue;
std::list<SNMPResponse> responseQue;
std::list<StationMap> stationList;
std::list<LocationMap> locationList;
std::list<WeatherCommand> weatherQue;

DWORD snmpPort;
bool snmpDebug;
bool i104Debug;
DWORD i104CA;
bool weatherDebug;
std::string forecastUrl;
std::string astroUrl;

// Liste over ip'er som krever response (alternativ til pinging)
void response(std::string name, DWORD tick, DWORD id)
{
	SNMPResponse sr;
	sr.name = name;
	sr.stime = tick*10;
	sr.id = id;
	sr.tick = sr.stime;
	sr.last = 2;
	responseQue.push_back(sr);
}

void scheduler(std::string t, std::string command)
{
	static DWORD tick=1;
	ScheduleEntry se;
	if (command.empty())
		return;
	if (t.empty())
		return;
	if (!isNumber(t,10))
		return;
	se.command=command;
	se.stime=atoi(t.c_str())*10;
	se.tick = 200 + tick;
	tick += 5;
	scheduleQue.push_back(se);
}

void doCommand(std::string s)
{
	int next=1;
	int tmpsection=cmdsection;
	string arg1,arg2,arg3;
	string cmd = lowercase(getWord(s,next++));
	if (cmd=="cd")
	{
		arg1 = lowercase(getWord(s,next++));
		if (arg1=="i104")
			cmdsection=C104;
		else if (arg1=="main")
			cmdsection=CMAIN;
		else if (arg1=="snmp")
			cmdsection=CSNMP;
		else if (arg1 == "weather")
			cmdsection = CWEATHER;
		else if (arg1 == "..")
			cmdsection=CMAIN;
		return;
	}
	if (cmd=="main")
	{
		tmpsection=CMAIN;
		cmd = lowercase(getWord(s,next++));
	}else if (cmd=="i104")
	{
		tmpsection=C104;
		cmd = lowercase(getWord(s,next++));
	}else if (cmd=="snmp")
	{
		tmpsection=CSNMP;
		cmd = lowercase(getWord(s,next++));
	}
	else if (cmd == "weather")
	{
		tmpsection = CWEATHER;
		cmd = lowercase(getWord(s, next++));
	}

	if (tmpsection==CMAIN)
	{
		if (cmd=="log")
		{
			logging=booleanString(getWord(s,next++));
			return;
		}
		if (cmd=="logfile")
		{
			logfile=getWord(s,next++);
			return;
		}
		if (cmd=="auto")
		{
			arg1 = getWord(s,next++);
			arg2.clear();
			string w = getWord(s,next++);
			while (w!="")
			{
				arg2 += w + " ";
				w = getWord(s,next++);
			}
			scheduler(arg1,arg2);
			return;
		}
		if (cmd=="scheduler")
		{
			useScheduler=booleanString(getWord(s,next++));
			return;
		}
		if (cmd == "station")
		{
			StationMap st;
			st.name = getWord(s, next++);
			while ((arg1 = getWord(s, next++)) != "")
				st.ip.push_back(atoip(arg1));
			stationList.push_back(st);
			return;
		}
		if (cmd == "location")
		{
			LocationMap lt;
			lt.name = getWord(s, next++);
			lt.latitude=atof(getWord(s, next++).c_str());
			lt.longitude = atof(getWord(s, next++).c_str());
			locationList.push_back(lt);
			return;
		}
	}else if (tmpsection == C104)
	{
		if (cmd=="start")
		{
			if (!iec104)
			{
				iec104 = new IEC104Slave(hEvent[C104]);
				iec104->debug = i104Debug;
				iec104->commonaddress = i104CA;
				iec104->start();
			}
			return;
		}
		if (cmd=="stop")
		{
			if (iec104)
				delete iec104;
			iec104 = NULL;
			return;
		}
		if (cmd=="debug")
		{
			i104Debug = booleanString(getWord(s, next++));
			if (iec104)
				iec104->debug = i104Debug;
			return;
		}
		if (cmd=="commonaddress")
		{
			i104CA = atoi(getWord(s, next++).c_str());
			if (iec104)
				iec104->commonaddress = i104CA;
			return;
		}
		if (cmd=="set")
		{
			if (!iec104)
				return;
			int ident,addr;
			arg1=getWord(s,next++);
			arg2=getWord(s,next++);
			arg3=getWord(s,next++);
			ident=atoi(arg1.c_str());
			addr=atoi(arg2.c_str());
			switch (ident)
			{
				case M_SP_NA_1:
				case M_SP_TB_1:
					iec104->setSpi(ident,addr,atoi(arg3.c_str()));
					break;
				case M_DP_NA_1:
				case M_DP_TB_1:
					iec104->setDpi(ident,addr,atoi(arg3.c_str()));
					break;
				case M_ME_NA_1:
				case M_ME_TD_1:
					iec104->setNva(ident,addr,atoi(arg3.c_str()));
					break;
				case M_ME_NC_1:
				case M_ME_TF_1:
					iec104->setVal(ident,addr,(float)atof(arg3.c_str()));
					break;
				default:
					cerr << "Ukjent ASDU.\n";
					break;
			}
			return;
		}
	}else if (tmpsection==CSNMP)
	{
		if (cmd=="start")
		{
			if (!snmp)
			{
				snmp = new SNMPManager(hEvent[CSNMP]);
				snmp->responsePort = snmpPort;
				snmp->debug = snmpDebug;
				snmp->start();
			}
			return;
		}
		if (cmd == "stop")
		{
			if (snmp)
				delete snmp;
			snmp = NULL;
			return;
		}
		if (cmd=="debug")
		{
			snmpDebug = booleanString(getWord(s, next++));
			if (snmp)
				snmp->debug = snmpDebug;
			return;
		}
		if (cmd=="port")
		{
			arg1 = getWord(s,next++);
			if (isNumber(arg1, 10))
			{
				snmpPort = atoi(arg1.c_str());
				if (snmp)
				{
					cout << "Restarter SNMP Manager for å skifte port." << endl;
					doCommand("snmp stop");
					doCommand("snmp start");
				}
			}
			return;
		}
		if (cmd=="get")
		{
			if (!snmp)
			{
				cout << "SNMP Manager kjørier ikke." << endl;
				return;
			}
			DWORD ip=0;
			arg1 = getWord(s,next++);
			arg2 = getWord(s,next++);
			if (isNumber(arg1))
			{
				ip = atoip(arg1);
			}else
			{
				list<StationMap>::iterator it=stationList.begin();
				while (it != stationList.end())
				{
					if (it->name == arg1)
					{
						if (!it->ip.empty())
							ip = it->ip.front();
						break;
					}
					++it;
				}
			}
			if (!snmp->get(ip, arg2))
				cerr << "Syntax: get <ip|station> <objekt>" << endl;
			return;
		}
		if (cmd == "response")
		{
			arg1 = getWord(s, next++);
			arg2 = getWord(s, next++);
			arg3 = getWord(s, next++);
			response(arg1, atoi(arg2.c_str()), atoi(arg3.c_str()));
			return;
		}
		if (cmd=="rec")
		{
			SNMPCommand sc;
			sc.pdu = atoi(getWord(s,next++).c_str());
			sc.station = getWord(s, next++);
			arg1 = getWord(s,next++);
			switch (sc.pdu)
			{
				case PDU_TRAP1:
					sc.object = getWord(arg1, 1, ":");
					sc.generic_trap = atoi(getWord(arg1, 2, ":").c_str());
					sc.spesific_trap = atoi(getWord(arg1, 3, ":").c_str());
					break;
				case PDU_GETRESPONSE:
				case PDU_INFORMREQUEST:
				case PDU_TRAP2:
					sc.object = getWord(arg1, 1, ":");
					sc.value = getWord(arg1, 2, ":");
					break;
				default:
					sc.object = arg1;
					break;
			}
			sc.cmd = getQuote(s);
			snmpQue.push_back(sc);
			return;
		}
	}else if (tmpsection == CWEATHER)
	{
		if (cmd == "start")
		{
			if (!weather)
			{
				weather = new Weather(hEvent[CWEATHER]);
				weather->debug = weatherDebug;
				weather->astroAddress = astroUrl;
				weather->forecastAddress = forecastUrl;
				weather->start();
			}
			return;
		}
		if (cmd == "stop")
		{
			if (weather)
				delete weather;
			weather = NULL;
			return;
		}
		if (cmd == "debug")
		{
			weatherDebug = booleanString(getWord(s, next++));
			if (weather)
				weather->debug = weatherDebug;
			return;
		}
		if (cmd == "astrourl")
		{
			astroUrl = getWord(s, next++);
			if (weather)
				weather->astroAddress = astroUrl;
			return;
		}
		if (cmd == "forecasturl")
		{
			forecastUrl = getWord(s, next++);
			if (weather)
				weather->forecastAddress = forecastUrl;
			return;
		}
		if (cmd == "get")
		{
			if (!weather)
			{
				cout << "Vær agent kjørier ikke." << endl;
				return;
			}
			arg1 = getWord(s, next++);
			arg2 = getWord(s, next++);
			list<LocationMap>::iterator it = locationList.begin();
			double lat = 0.0;
			double lon = 0.0;
			while (it != locationList.end())
			{
				if (it->name == arg2)
				{
					lat = it->latitude;
					lon = it->longitude;
					break;
				}
				++it;
			}
			if ((lat == 0.0) || (lon == 0.0))
			{
				cerr << "Latitude/Longitude er ikke angitt." << endl;
				return;
			}
			if (arg1 == "astro")
			{
				AstroRequest aReq;
				aReq.altitude = -1;
				aReq.latitude = lat;
				aReq.longitude = lon;
				GetLocalTime(&aReq.date);
				weather->addRequest(aReq);
			}else if (arg1 == "forecast")
			{
				WeatherRequest wReq;
				wReq.altitude = 0;
				wReq.latitude = lat;
				wReq.longitude = lon;
				weather->addRequest(wReq);
			}
			return;
		}
		if (cmd == "rec")
		{
			WeatherCommand wc;
			wc.location = getWord(s, next++);
			wc.type = getWord(s, next++);
			wc.cmd = getQuote(s);
			weatherQue.push_back(wc);
			return;
		}
	}
}

void scheduler()
{
	if (!useScheduler)
		return;
	std::list<ScheduleEntry>::iterator it=scheduleQue.begin();
	while (it!=scheduleQue.end())
	{
		--it->tick;
		if (it->tick==0)
		{
			it->tick=it->stime;
			if (logging)
				cout << it->command << endl;
			doCommand(it->command);
		}
		++it;
	}
}

void response(std::string station)
{
	std::string s;
	char sz[256];
	std::list<SNMPResponse>::iterator it = responseQue.begin();
	while (it != responseQue.end())
	{
		if (it->name == station)
		{
			it->tick = it->stime;
			if (it->last != 0)
			{
				_itoa_s(it->id, sz, 256, 10);
				s = "i104 set 30 ";
				s += sz;
				s += " 0";
				if (logging)
					cout << s << endl;
				doCommand(s);
				it->last = 0;
			}
		}
		++it;
	}
}

void response()
{
	std::string s;
	char sz[256];
	std::list<SNMPResponse>::iterator it = responseQue.begin();
	while (it != responseQue.end())
	{
		--it->tick;
		if (it->tick == 0)
		{
			it->tick = it->stime;
			if (it->last != 1)
			{
				_itoa_s(it->id, sz, 256, 10);
				s = "i104 set 30 ";
				s += sz;
				s += " 1";
				if (logging)
					cout << s << endl;
				doCommand(s);
				it->last = 1;
			}
		}
		++it;
	}
}

void handleIEC104Event(APDU& apdu)
{
}

void checkSNMPEvent(std::string station, SNMPTelegram& t)
{
	std::list<SNMPCommand>::iterator it=snmpQue.begin();
	std::string cmd;
  string::size_type e;
	while (it!=snmpQue.end())
	{
		if ((it->station==station) && (it->object==t.object) && (it->pdu==t.type))
		{
			if (it->pdu==PDU_TRAP1)
			{
				if (it->generic_trap!=t.genericTrap)
				{
					++it;
					continue;
				}
				if ((it->generic_trap==TRAP_ENTERPRISESPECIFIC) && (it->spesific_trap!=t.specificTrap))
				{
					++it;
					continue;
				}
			}else if ((it->value!="") && (it->value!=t.value))
			{
				++it;
				continue;
			}
			cmd=it->cmd;
			e=cmd.find("<value>");
			if (e!=string::npos)
				cmd.replace(e,7,t.value);
			if (logging)
				cout << cmd << endl;
			doCommand(cmd);
			return;
		}
		++it;
	}
}

void handleSNMPEvent(std::string station, SNMPTelegram& t)
{
	response(station);
	if (logging)
	{
		char sz[16];
		string s="SNMP> ";
		s += station;
		if (t.type==PDU_TRAP1)
		{
			_itoa_s(t.genericTrap,sz,16,10);
			s+= " : " + string(sz);
			_itoa_s(t.specificTrap,sz,16,10);
			s+= " : " + string(sz);
		}
		if (!t.object.empty())
		{
			s+=" : " + t.object;
			s+="=" + t.value;
		}
		if (t.error_status)
			s+=" Error: " + t.error_status;
		if (!logfile.empty())
		{
			ofstream f(logfile,ios_base::app);
			f << s << endl;
		}else
		{
			cout << s << endl;
		}
	}
	if (!t.error_status)
		checkSNMPEvent(station, t);
}

std::string getStationName(DWORD ip)
{
	list<StationMap>::iterator smIt = stationList.begin();
	while (smIt != stationList.end())
	{
		list<DWORD>::iterator dwIt = smIt->ip.begin();
		while (dwIt != smIt->ip.end())
		{
			if (ip == *dwIt)
				return smIt->name;
			++dwIt;
		}
		++smIt;
	}
	return iptoa(ip);
}

std::string getLocation(MetAstroLocation mal)
{
	list<LocationMap>::iterator lmIt = locationList.begin();
	while (lmIt != locationList.end())
	{
		if ((lmIt->latitude == mal.latitude) && (lmIt->longitude == mal.longitude))
			return lmIt->name;
		++lmIt;
	}
	return "";
}

std::string getLocation(MetWeatherLocation mwl)
{
	list<LocationMap>::iterator lmIt = locationList.begin();
	while (lmIt != locationList.end())
	{
		if ((lmIt->latitude == mwl.latitude) && (lmIt->longitude == mwl.longitude))
			return lmIt->name;
		++lmIt;
	}
	return "";
}

void checkWeatherEvent(std::string location, std::string type, std::string value)
{
	std::list<WeatherCommand>::iterator wcIt = weatherQue.begin();
	std::string cmd;
	string::size_type e;
	while (wcIt != weatherQue.end())
	{
		if ((wcIt->location == location) && (wcIt->type == type))
		{
			cmd = wcIt->cmd;
			e = cmd.find("<value>");
			if (e != string::npos)
				cmd.replace(e, 7, value);
			if (logging)
				cout << cmd << endl;
			doCommand(cmd);
			return;
		}
		++wcIt;
	}
}

void handleAstroEvent(MetAstroData mad)
{
	MetAstroLocation ml;
	std::string location;
	if (!mad.getCurrentLocation(ml))
		return;
	location = getLocation(ml);
	if (location == "")
		return;

	checkWeatherEvent(location, "sun", (mad.isSunNow(ml) ? "1" : "0"));
}

void handleWeatherEvent(MetWeatherData mwd)
{
	MetWeatherLocation wl;
	std::string location;
	std::string val;
	if (!mwd.getCurrentLocation(wl))
		return;
	location = getLocation(wl);
	if (location == "")
		return;
	if (wl.bCloudiness)
		checkWeatherEvent(location, "cloudiness", ftoa(float(wl.cloudiness)));
	if (wl.bFog)
		checkWeatherEvent(location, "fog", ftoa(float(wl.fog)));
	if (wl.bHighClouds)
		checkWeatherEvent(location, "highClouds", ftoa(float(wl.highClouds)));
	if (wl.bHumidity)
		checkWeatherEvent(location, "humidity", ftoa(float(wl.humidity)));
	if (wl.bLowClouds)
		checkWeatherEvent(location, "lowClouds", ftoa(float(wl.lowClouds)));
	if (wl.bMediumClouds)
		checkWeatherEvent(location, "mediumClouds", ftoa(float(wl.mediumClouds)));
	if (wl.bPrecipitation)
		checkWeatherEvent(location, "precipitation", ftoa(float(wl.precipitation)));
	if (wl.bPressure)
		checkWeatherEvent(location, "pressure", ftoa(float(wl.pressure)));
	if (wl.bSymbol)
		checkWeatherEvent(location, "symbol", ftoa(float(wl.symbol)));
	if (wl.bTemperature)
		checkWeatherEvent(location, "temperature", ftoa(float(wl.temperature)));
	if (wl.bWindDirection)
		checkWeatherEvent(location, "windDirection", ftoa(float(wl.windDirection)));
	if (wl.bWindSpeed)
		checkWeatherEvent(location, "windSpeed", ftoa(float(wl.windSpeed)));
	if (wl.bDewpointTemperature)
		checkWeatherEvent(location, "dewpointTemperature", ftoa(float(wl.dewpointTemperature)));
	if (wl.bTemperatureProbability)
		checkWeatherEvent(location, "temperatureProbability", ftoa(float(wl.temperatureProbability)));
	if (wl.bWindProbability)
		checkWeatherEvent(location, "windProbability", ftoa(float(wl.windProbability)));
	if (wl.bPrecipitationMax)
		checkWeatherEvent(location, "precipitationMax", ftoa(float(wl.precipitationMax)));
	if (wl.bPrecipitationMin)
		checkWeatherEvent(location, "precipitationMin", ftoa(float(wl.precipitationMin)));
	if (wl.bSymbolProbability)
		checkWeatherEvent(location, "symbolProbability", ftoa(float(wl.symbolProbability)));
}

void start(std::string ini)
{

	int i;
	char sz[256];
	string s;

	cmdsection = CMAIN;
	logging = false;
	useScheduler = false;
	logfile.clear();
	scheduleQue.clear();
	snmpQue.clear();
	responseQue.clear();
	stationList.clear();
	locationList.clear();
	weatherQue.clear();

	snmpPort = 161;
	snmpDebug = false;
	i104Debug = false;
	i104CA = 1;
	weatherDebug = false;
	forecastUrl.clear();
	astroUrl.clear();

	// Sett opp Events som lyttes på i hovedløkka (run()).
	for (i = 1; i < 4; i++)
		hEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Initier WinSock
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Les oppstartsfil
	if (ini!="")
	{
		ifstream inifile(ini);
		if (inifile)
		{
			while (inifile.getline(sz, 256))
			{
				s = trim(sz);
				if ((s.length()>0) && (s.at(0) != ';'))
					doCommand(s);
			}
		}else
		{
			cerr << "Finner ikke " << ini << endl;
		}
	}

	cmdsection = CMAIN;
};

void stop()
{
	int i;
	if (weather)
		delete weather;
	if (snmp)
		delete snmp;
	if(iec104)
		delete iec104;
	weather = NULL;
	snmp = NULL;
	iec104 = NULL;
	for (i = 1; i < 4; i++)
		CloseHandle(hEvent[i]);
	WSACleanup();
};

// Returnerer true for restart og false for exit
bool run()
{
	DWORD dw;

	string s;
	APDU apdu;
	SNMPTelegram snmpT;
	MetAstroData mad;
	MetWeatherData mwd;

	while (1)
	{
		dw = WaitForMultipleObjects(4, hEvent, FALSE, 100);
		switch (dw)
		{
			case WAIT_CONSOLE: // Kommandoer fra konsolle
				while (con.read(s))
				{
					if (s == "quit")
						return false;
					else if (s == "restart")
						return true;
					doCommand(s);
			}
			break;
		case WAIT_104: // Meldinger/kommandoer fra IEC104 master
			while (iec104->read(apdu))
			{
				handleIEC104Event(apdu);
			}
			break;
		case WAIT_SNMP:
			while (snmp->read(snmpT))
			{
				// Sjekk om stasjonen er definert
				s = getStationName(snmpT.ip);
				handleSNMPEvent(s, snmpT);
			}
			break;
		case WAIT_WEATHER:
			while (weather->read(mad))
			{
				handleAstroEvent(mad);
			}
			while (weather->read(mwd))
			{
				handleWeatherEvent(mwd);
			}
			break;
		case WAIT_TIMEOUT:
			scheduler();
			response();
			break;
		default:
			cout << "Ukjent event." << endl;
			break;
		}
	}
	cerr << "Skal aldri komme hit." << endl;
	return false;
}

int main(int argc, char* argv[])
{
	string inifile;

	if (argc > 1)
		inifile = argv[1];
	else
		inifile="config.ini";

	// Sett console Event
	hEvent[0] = con.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	start(inifile);
	while (run())
	{
		stop();
		start(inifile);
	}

	CloseHandle(hEvent[0]);
}