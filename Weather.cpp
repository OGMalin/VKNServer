#include "Weather.h"
#include <process.h>
#include <WinInet.h>
#include <iostream>
#include "XML.h"
#include "Utility.h"

using namespace std;

CRITICAL_SECTION WeatherCS;

Weather::Weather(HANDLE event)
{
	hThread=NULL;
	hEvent = event;

	abort = false;
	debug = false;
	InitializeCriticalSection(&WeatherCS);
};

Weather::~Weather()
{
	stop();
	DeleteCriticalSection(&WeatherCS);
};

bool Weather::start()
{
	stop();
	abort=false;
	while (!requestWeather.empty())
		requestWeather.pop();
	while (!requestAstro.empty())
		requestAstro.pop();
	hThread=(HANDLE)_beginthread(Weather::threadLoop,0,this);
	if ((int)hThread==-1)
	{
		hThread=NULL;
		abort=true;
		return false;
	}
	return true;
};

void Weather::stop()
{
  abort=true;
  if (hThread)
    if (WaitForSingleObject(hThread,5000)==WAIT_TIMEOUT)
     	TerminateThread(hThread,TRUE);
  hThread=NULL;
};

void Weather::threadLoop(void*lpv)
{
	int bufsize=1024*200;
  char* buffer = new char[bufsize];
	char szHead[]="Accept: */*\r\n\r\n";
  int   index;
	char sz[32];
	string sRequest;
  //int   msgLen;
  //char  b;
  DWORD dwRead;
  //DWORD dwError=0;
	HINTERNET hInternet;
	HINTERNET hConnect;
	WeatherRequest weatherrequest;
	AstroRequest astrorequest;
  Weather* weather=(Weather*)lpv;
	hInternet=InternetOpen("VKNServer",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if (hInternet == NULL)
	{
		cerr << "Kan ikke åpne til Internett, Feilkode:" << GetLastError() << endl;
	}
	// Meld fra at den er startet
  while(!weather->abort)
  {
		if (weather->getRequest(weatherrequest))
		{
			sRequest=weather->forecastAddress;
			sRequest+="?lat=";
			sRequest+=ftoa(weatherrequest.latitude,2,0);
			sRequest+=";lon=";
			sRequest+=ftoa(weatherrequest.longitude,2,0);
			if (weatherrequest.altitude>=0)
			{
				sRequest+=";msl=";
				_itoa_s(weatherrequest.altitude,sz,32,10);
				sRequest+=sz;
			};
			hConnect=InternetOpenUrl(hInternet,sRequest.c_str(),szHead,(DWORD)strlen(szHead),INTERNET_FLAG_DONT_CACHE,0);
			if (hConnect == NULL)
			{
				cerr << "Kan ikke kople til " << sRequest.c_str() << ", Feilkode:" << GetLastError() << endl;
				continue;
			}
			strcpy_s(buffer, 256, "<?xml version=\"1.0\"?>\n");
		  index=22;
			while (InternetReadFile(hConnect,&buffer[index],50,&dwRead))
			{
				if (weather->abort)
					break;
				if (dwRead==0)
				{
					weather->addWeatherData(buffer,index);
					break;
				}
				index+=50;
				if ((index+50)>bufsize)
					break;
			};
			InternetCloseHandle(hConnect);
		}else if (weather->getRequest(astrorequest))
		{
			sRequest=weather->astroAddress;
			sRequest+="?lat=";
			sRequest+=ftoa(astrorequest.latitude,2,0);
			sRequest+=";lon=";
			sRequest+=ftoa(astrorequest.longitude,2,0);
			if (astrorequest.altitude>=0)
			{
				sRequest+=";msl=";
				_itoa_s(astrorequest.altitude,sz,32,10);
				sRequest+=sz;
			};
			sprintf_s(sz,32,";date=%04d-%02d-%02d",astrorequest.date.wYear,astrorequest.date.wMonth,astrorequest.date.wDay);
			sRequest+=sz;
			hConnect=InternetOpenUrl(hInternet,sRequest.c_str(),szHead,(DWORD)strlen(szHead),INTERNET_FLAG_DONT_CACHE,0);
			if (hConnect == NULL)
			{
				cerr << "Kan ikke kople til " << sRequest.c_str() << ", Feilkode:" << GetLastError() << endl;
				continue;
			}
			strcpy_s(buffer,256,"<?xml version=\"1.0\"?>\n");
		  index=22;
			while (InternetReadFile(hConnect,&buffer[index],50,&dwRead))
			{
				if (weather->abort)
					break;
				if (dwRead==0)
				{
					weather->addAstroData(buffer,index);
					break;
				}
				index+=50;
				if ((index+50)>bufsize)
					break;
			};
			InternetCloseHandle(hConnect);
		}else
		{
			Sleep(250);
		}
  }
	InternetCloseHandle(hInternet);
	weather->hThread=NULL;
	delete [] buffer;
  _endthread();
};

void Weather::addRequest(const WeatherRequest& request)
{
	
	EnterCriticalSection(&WeatherCS);
	if (!checkData(request))
		requestWeather.push(request);
	LeaveCriticalSection(&WeatherCS);
};

void Weather::addRequest(const AstroRequest& request)
{
	EnterCriticalSection(&WeatherCS);
	if (!checkData(request))
		requestAstro.push(request);
	LeaveCriticalSection(&WeatherCS);
};

bool Weather::getRequest(WeatherRequest& request)
{
	bool ret;
	EnterCriticalSection(&WeatherCS);
	ret=requestWeather.empty()?false:true;
	if (ret)
	{
		request=requestWeather.front();
		requestWeather.pop();
	}
	LeaveCriticalSection(&WeatherCS);
	return ret;
};

bool Weather::getRequest(AstroRequest& request)
{
	bool ret;
	EnterCriticalSection(&WeatherCS);
	ret=requestAstro.empty()?false:true;
	if (ret)
	{
		request=requestAstro.front();
		requestAstro.pop();
	}
	LeaveCriticalSection(&WeatherCS);
	return ret;
};

void Weather::addWeatherData(char* data, int len)
{
	XML xml;
	bool ret;
	try {
		ret = xml.read(data);
	}
	catch (char const* e)
	{
		cerr << "Feil i XML forecast data\n" << e << endl;
		return;
	}
	if (ret)
	{
		MetWeatherData mwd;
		MetWeatherModel mm;
		MetWeatherTime mt;
		vector<XMLElement>::iterator it1;
		vector<XMLElement>::iterator it2;
		vector<XMLElement>::iterator it3;
		vector<XMLElement>::iterator it4;
		map<std::string,std::string>::iterator ait;
		mwd.clear();
		ait=xml.root.attribute.begin();
		while (ait!=xml.root.attribute.end())
		{
			if (ait->first=="created")
				mwd.created=ZuluTimeToLocalFileTime(ait->second);
			++ait;
		};
		it1=xml.root.element.begin();
		while (it1!=xml.root.element.end())
		{
			if (it1->name=="meta")
			{
				it2=it1->element.begin();
				while (it2!=it1->element.end())
				{
					if (it2->name=="model")
					{
						mm.clear();
						ait=it2->attribute.begin();
						while (ait!=it2->attribute.end())
						{
							if (ait->first=="to")
								mm.to=ZuluTimeToLocalFileTime(ait->second);
							else if (ait->first=="from")
								mm.from=ZuluTimeToLocalFileTime(ait->second);
							else if (ait->first=="nextrun")
								mm.nextrun=ZuluTimeToLocalFileTime(ait->second);
							else if (ait->first=="runended")
								mm.runended=ZuluTimeToLocalFileTime(ait->second);
							else if (ait->first=="termin")
								mm.termin=ZuluTimeToLocalFileTime(ait->second);
							else if (ait->first=="name")
								mm.name=ait->second;
							++ait;
						};
						mwd.model.push_back(mm);
					};
					++it2;
				};
			}else if (it1->name=="product")
			{
				it2=it1->element.begin();
				while (it2!=it1->element.end())
				{
					if (it2->name=="time")
					{
						mt.clear();
						ait=it2->attribute.begin();
						while (ait!=it2->attribute.end())
						{
							if (ait->first=="to")
								mt.to=ZuluTimeToLocalFileTime(ait->second);
							else if (ait->first=="from")
								mt.from=ZuluTimeToLocalFileTime(ait->second);
							++ait;
						}

						if (debug)
							cerr << "W> time: to=" << timeToString(mt.to) << ", from=" << timeToString(mt.from) << endl;
						it3 = it2->element.begin();
						while (it3!=it2->element.end())
						{
							if (it3->name=="location")
							{
								ait=it3->attribute.begin();
								while (ait!=it3->attribute.end())
								{
									if (ait->first=="longitude")
										mt.location.longitude=atof(ait->second.c_str());
									else if (ait->first=="latitude")
										mt.location.latitude=atof(ait->second.c_str());
									else if (ait->first=="altitude")
										mt.location.altitude=atoi(ait->second.c_str());
									++ait;
								}
								if (debug)
									cerr << "W> Location: latitude=" << mt.location.latitude << ", longitude=" << mt.location.longitude << ", altitude=" << mt.location.altitude << endl;
								it4 = it3->element.begin();
								while (it4!=it3->element.end())
								{
									if (it4->name=="temperature")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.temperature=atof(ait->second.c_str());
												mt.location.bTemperature = true;
												if (debug)
													cerr << "W> temperature=" << mt.location.temperature << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "windDirection")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "deg")
											{
												mt.location.windDirection=atof(ait->second.c_str());
												mt.location.bWindDirection = true;
												if (debug)
													cerr << "W> windDirection=" << mt.location.windDirection << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "windSpeed")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "mps")
											{
												mt.location.windSpeed=atof(ait->second.c_str());
												mt.location.bWindSpeed = true;
												if (debug)
													cerr << "W> windSpeed=" << mt.location.windSpeed << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "humidity")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.humidity=atof(ait->second.c_str());
												mt.location.bHumidity = true;
												if (debug)
													cerr << "W> humidity=" << mt.location.humidity << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "pressure")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.pressure=atof(ait->second.c_str());
												mt.location.bPressure = true;
												if (debug)
													cerr << "W> pressure=" << mt.location.pressure << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "cloudiness")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "percent")
											{
												mt.location.cloudiness=atof(ait->second.c_str());
												mt.location.bCloudiness = true;
												if (debug)
													cerr << "W> cloudiness=" << mt.location.cloudiness << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "fog")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "percent")
											{
												mt.location.fog=atof(ait->second.c_str());
												mt.location.bFog = true;
												if (debug)
													cerr << "W> fog=" << mt.location.fog << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "lowClouds")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "percent")
											{
												mt.location.lowClouds=atof(ait->second.c_str());
												mt.location.bLowClouds = true;
												if (debug)
													cerr << "W> lowClouds=" << mt.location.lowClouds << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "meduimClouds")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "percent")
											{
												mt.location.mediumClouds=atof(ait->second.c_str());
												mt.location.bMediumClouds = true;
												if (debug)
													cerr << "W> mediumClouds=" << mt.location.mediumClouds << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "highClouds")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "percent")
											{
												mt.location.highClouds=atof(ait->second.c_str());
												mt.location.bHighClouds = true;
												if (debug)
													cerr << "W> highClouds=" << mt.location.highClouds << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "dewpointTemperature")
									{
										ait = it4->attribute.begin();
										while (ait != it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.dewpointTemperature = atof(ait->second.c_str());
												mt.location.bDewpointTemperature = true;
												if (debug)
													cerr << "W> dewpointTemperature=" << mt.location.dewpointTemperature << endl;
												break;
											}
											++ait;
										}
									}									
									else if (it4->name == "temperaturProbability")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.temperatureProbability=atoi(ait->second.c_str());
												mt.location.bTemperature = true;
												if (debug)
													cerr << "W> temperatureProbability=" << mt.location.temperatureProbability << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "windProbability")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.windProbability=atoi(ait->second.c_str());
												mt.location.bWindProbability = true;
												if (debug)
													cerr << "W> windProbability=" << mt.location.windProbability << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "symbolProbability")
									{
										ait = it4->attribute.begin();
										while (ait != it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.symbolProbability = atoi(ait->second.c_str());
												mt.location.bSymbolProbability = true;
												if (debug)
													cerr << "W> symbolProbability=" << mt.location.symbolProbability << endl;
												break;
											}
											++ait;
										}
									}
									else if (it4->name == "precipitation")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "value")
											{
												mt.location.precipitation = atof(ait->second.c_str());
												mt.location.bPrecipitation = true;
											}
											else if (ait->first == "maxvalue")
											{
												mt.location.precipitationMax=atof(ait->second.c_str());
												mt.location.bPrecipitationMax = true;
											}
											else if (ait->first == "minvalue")
											{
												mt.location.precipitationMin=atof(ait->second.c_str());
												mt.location.bPrecipitationMin = true;
											}
											++ait;
										}
										if (debug)
										{
											cerr << "W> ";
											if (mt.location.bPrecipitation)
												cerr << "precipitation=" << mt.location.precipitation;
											if (mt.location.bPrecipitationMax)
												cerr << "precipitation=" << mt.location.precipitationMax;
											if (mt.location.bPrecipitationMin)
												cerr << "precipitation=" << mt.location.precipitationMin;
											cerr << endl;
										}
									}
									else if (it4->name == "symbol")
									{
										ait=it4->attribute.begin();
										while (ait!=it4->attribute.end())
										{
											if (ait->first == "number")
											{
												mt.location.symbol=atoi(ait->second.c_str());
												mt.location.bSymbol = true;
												if (debug)
													cerr << "W> symbol=" << mt.location.symbol << endl;
												break;
											}
											++ait;
										}
									};
									++it4;
								};
							};
							++it3;
						};
					};
					mwd.time.push_back(mt);
					++it2;
				};
			};
			++it1;
		};
		EnterCriticalSection(&WeatherCS);
		setData(mwd);
		weatherQue.push(mwd);
		LeaveCriticalSection(&WeatherCS);
		SetEvent(hEvent);
		//		PostMessage(hWnd,WM_COMMAND,ID_WEATHERMSG,WEATHER_DATA);
	};
};

void Weather::addAstroData(char* data, int len)
{
	SYSTEMTIME st;
	SYSTEMTIME lst;
	TIME_ZONE_INFORMATION tzi;
	XML xml;
	bool ret;
	try {
		ret = xml.read(data);
	}
	catch (char const* e)
	{
		cerr << "Feil i XML astro data\n" << e << endl;
		return;
	}
	if (ret)
	{
		MetAstroData mad;
		MetAstroTime mt;
		vector<XMLElement>::iterator it1;
		vector<XMLElement>::iterator it2;
		vector<XMLElement>::iterator it3;
		vector<XMLElement>::iterator it4;
		map<std::string,std::string>::iterator ait;
		mad.clear();
		it1=xml.root.element.begin();
		while (it1!=xml.root.element.end())
		{
			if (it1->name=="time")
			{
				mt.clear();
				ait=it1->attribute.begin();
				while (ait!=it1->attribute.end())
				{
					if (ait->first=="date")
					{
						memset(&st,0,sizeof(SYSTEMTIME));
						if (ait->second.length()>=10)
						{
							st.wYear=atoi(ait->second.substr(0,4).c_str());
							st.wMonth=atoi(ait->second.substr(5,2).c_str());
							st.wDay=atoi(ait->second.substr(8,2).c_str());
							GetTimeZoneInformation(&tzi);
							SystemTimeToTzSpecificLocalTime(&tzi,&st,&lst);
							SystemTimeToFileTime(&st,&mt.date);
						}
					}
					++ait;
				}
				if (debug)
					cerr << "W> time: " << timeToString(mt.date) << endl;
				it2 = it1->element.begin();
				while (it2!=it1->element.end())
				{
					if (it2->name=="location")
					{
						ait=it2->attribute.begin();
						while (ait!=it2->attribute.end())
						{
							if (ait->first=="longitude")
								mt.location.longitude=atof(ait->second.c_str());
							else if (ait->first=="latitude")
								mt.location.latitude=atof(ait->second.c_str());
							else if (ait->first=="altitude")
								mt.location.altitude=atoi(ait->second.c_str());
							++ait;
						}
						if (debug)
							cerr << "W> Location: latitude=" << mt.location.latitude << ", longitude=" << mt.location.longitude << ", altitude=" << mt.location.altitude << endl;
						it3 = it2->element.begin();
						while (it3!=it2->element.end())
						{
							if (it3->name=="sun")
							{
								ait=it3->attribute.begin();
								while (ait!=it3->attribute.end())
								{
									if (ait->first=="set")
										mt.location.sunset=ZuluTimeToLocalFileTime(ait->second);
									else if (ait->first=="rise")
										mt.location.sunrise=ZuluTimeToLocalFileTime(ait->second);
									else if (ait->first=="never_rise")
										mt.location.never_rise=booleanString(ait->second);
									else if (ait->first=="never_set")
										mt.location.never_set=booleanString(ait->second);
									++ait;
								};
							};
							++it3;
						}
						if (debug)
							cerr << "W> sun: rise=" << (mt.location.never_rise ? "Aldri" : timeToString(mt.location.sunrise)) << ", set=" << (mt.location.never_set ? "Aldri" : timeToString(mt.location.sunset)) << endl;

					};
					mad.time.push_back(mt);
					++it2;
				};
			};
			++it1;
		};
		EnterCriticalSection(&WeatherCS);
		setData(mad);
		astroQue.push(mad);
		LeaveCriticalSection(&WeatherCS);
		SetEvent(hEvent);
		//		PostMessage(hWnd,WM_COMMAND,ID_WEATHERMSG,ASTRO_DATA);
	};
};

bool Weather::read(MetWeatherData& mwd)
{
	bool ret=false;
	EnterCriticalSection(&WeatherCS);
	if (!weatherQue.empty())
	{
		mwd=weatherQue.front();
		weatherQue.pop();
		ret = true;
	}
	LeaveCriticalSection(&WeatherCS);
	return ret;
};

bool Weather::read(MetAstroData& mad)
{
	bool ret=false;
	EnterCriticalSection(&WeatherCS);
	if (!astroQue.empty())
	{
		mad=astroQue.front();
		astroQue.pop();
		ret = true;
	};
	LeaveCriticalSection(&WeatherCS);
	return ret;
};

bool Weather::checkData(const WeatherRequest& request)
{
	size_t size = weatherData.size();
	for (size_t i = 0; i < size; i++)
	{
		if (weatherData[i].time.size())
		{
			if ((weatherData[i].time[0].location.latitude == request.latitude) && (weatherData[i].time[0].location.longitude = request.longitude))
			{
				if (weatherData[i].model.size())
				{
					FILETIME now;
					localFileTime(now);
					if (weatherData[i].model[0].nextrun.dwHighDateTime>now.dwHighDateTime)
					{
						weatherQue.push(weatherData[i]);
						SetEvent(hEvent);
						return true;
					}
					else if (weatherData[i].model[0].nextrun.dwHighDateTime == now.dwHighDateTime)
					{
						if (weatherData[i].model[0].nextrun.dwLowDateTime >= now.dwLowDateTime)
						{
							weatherQue.push(weatherData[i]);
							SetEvent(hEvent);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool Weather::checkData(const AstroRequest& request)
{
	size_t size = astroData.size();
	for (size_t i = 0; i < size; i++)
	{
		if (astroData[i].time.size())
		{
			if ((astroData[i].time[0].location.latitude == request.latitude) && (astroData[i].time[0].location.longitude = request.longitude))
			{
				FILETIME now;
				localFileTime(now);

				//if (astroData[i].model.size())
				//{
				//	FILETIME now;
				//	localFileTime(now);
				//	if (weatherData[i].model[0].nextrun.dwHighDateTime>now.dwHighDateTime)
				//	{
				//		weatherQue.push(weatherData[i]);
				//		SetEvent(hEvent);
				//		return true;
				//	}
				//	else if (weatherData[i].model[0].nextrun.dwHighDateTime == now.dwHighDateTime)
				//	{
				//		if (weatherData[i].model[0].nextrun.dwLowDateTime >= now.dwLowDateTime)
				//		{
				//			weatherQue.push(weatherData[i]);
				//			SetEvent(hEvent);
				//			return true;
				//		}
				//	}
				//}
			}
		}
	}
	return false;
}

void Weather::setData(MetWeatherData& mwd)
{
	MetWeatherLocation ml1,ml2;
	size_t size;
	if (!mwd.getCurrentLocation(ml1))
		return;
	size=weatherData.size();
	for (size_t i = 0; i < size;i++)
	{
		if (weatherData[i].getCurrentLocation(ml2))
		{
			if ((ml1.latitude == ml2.latitude) && (ml1.longitude == ml2.longitude))
			{
				weatherData[i] = mwd;
				return;
			}
		}
	}
	weatherData.push_back(mwd);
}

void Weather::setData(MetAstroData& mad)
{
	MetAstroLocation ml1, ml2;
	size_t size;
	if (!mad.getCurrentLocation(ml1))
		return;
	size = astroData.size();
	for (size_t i = 0; i < size; i++)
	{
		if (astroData[i].getCurrentLocation(ml2))
		{
			if ((ml1.latitude == ml2.latitude) && (ml1.longitude == ml2.longitude))
			{
				astroData[i] = mad;
				return;
			}
		}
	}
	astroData.push_back(mad);
}

bool MetWeatherData::getCurrentLocation(MetWeatherLocation& ml)
{
	// I time filetime = 1*10*1000*1000*60*60 = 36.000.000.000;
	SYSTEMTIME st;
	FILETIME ft;
	GetLocalTime(&st);
	st.wMilliseconds=0;
	SystemTimeToFileTime(&st,&ft);
	if (time.size()<2)
		return false;
	ml=time[0].location;
	ml.precipitation=time[1].location.precipitation;
	ml.precipitationMax=time[1].location.precipitationMax;
	ml.precipitationMin=time[1].location.precipitationMin;
	ml.symbol=time[1].location.symbol;
	return true;
};

bool MetAstroData::getCurrentLocation(MetAstroLocation& ml)
{
	// I time filetime = 1*10*1000*1000*60*60 = 36.000.000.000;
	SYSTEMTIME st;
	FILETIME ft;
	GetLocalTime(&st);
	st.wMilliseconds=0;
	SystemTimeToFileTime(&st,&ft);
	if (time.size()<1)
		return false;
	ml=time[0].location;
	return true;
};

bool MetAstroData::isSunNow(MetAstroLocation& mal)
{
//	if (!getAstro(mal))
//		return false;
	if (mal.never_rise)
		return false;
	if (mal.never_set)
		return true;
	SYSTEMTIME set, rise, now;
	FileTimeToSystemTime(&mal.sunset, &set);
	FileTimeToSystemTime(&mal.sunrise, &rise);
	GetLocalTime(&now);
	if (now.wHour<rise.wHour)
		return false;
	if (now.wHour == rise.wHour)
	{
		if (now.wMinute<rise.wMinute)
			return false;
		if (now.wMinute == rise.wMinute)
		{
			if (now.wSecond<rise.wSecond)
				return false;
			if (now.wSecond == rise.wSecond)
				return true;
		};
	};
	if (now.wHour>set.wHour)
		return false;
	if (now.wHour == set.wHour)
	{
		if (now.wMinute>set.wMinute)
			return false;
		if (now.wMinute == set.wMinute)
		{
			if (now.wSecond>set.wSecond)
				return false;
		};
	};
	return true;
};

const FILETIME ZuluTimeToFileTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	FILETIME ft;
	SYSTEMTIME st=ZuluTimeToSystemTime(z);
	SystemTimeToFileTime(&st,&ft);
	return ft;
};

const FILETIME ZuluTimeToLocalFileTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	FILETIME ft;
	SYSTEMTIME st=ZuluTimeToLocalSystemTime(z);
	SystemTimeToFileTime(&st,&ft);
	return ft;
};


const SYSTEMTIME ZuluTimeToSystemTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	SYSTEMTIME st;
	memset(&st,0,sizeof(SYSTEMTIME));
	if (z.length()==20)
	{
		st.wYear=atoi(z.substr(0,4).c_str());
		st.wMonth=atoi(z.substr(5,2).c_str());
		st.wDay=atoi(z.substr(8,2).c_str());
		st.wHour=atoi(z.substr(11,2).c_str());
		st.wMinute=atoi(z.substr(14,2).c_str());
		st.wSecond=atoi(z.substr(17,2).c_str());
	};
	return st;
};

const SYSTEMTIME ZuluTimeToLocalSystemTime(const std::string& z)
{ // 01234567890123456789
	// CCYY-MM-DDTHH:MM:SSZ
	TIME_ZONE_INFORMATION tzi;
	SYSTEMTIME lst;
	GetTimeZoneInformation(&tzi);
	SYSTEMTIME st=ZuluTimeToSystemTime(z);
	SystemTimeToTzSpecificLocalTime(&tzi,&st,&lst);
	return lst;
};

