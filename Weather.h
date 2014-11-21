#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <queue>

extern const FILETIME ZuluTimeToFileTime(const std::string& z);
extern const FILETIME ZuluTimeToLocalFileTime(const std::string& z);
extern const SYSTEMTIME ZuluTimeToSystemTime(const std::string& z);
extern const SYSTEMTIME ZuluTimeToLocalSystemTime(const std::string& z);

class MetWeatherLocation
{
public:
	bool bTemperature;
	bool bWindDirection;
	bool bWindSpeed;
	bool bHumidity;
	bool bPressure;
	bool bCloudiness;
	bool bFog;
	bool bLowClouds;
	bool bMediumClouds;
	bool bHighClouds;
	bool bTemperatureProbability;
	bool bWindProbability;
	bool bPrecipitation;
	bool bPrecipitationMax;
	bool bPrecipitationMin;
	bool bSymbol;
	bool bDewpointTemperature;
	bool bSymbolProbability;

	double longitude;
	double latitude;
	int altitude;
	double temperature;
	double windDirection;
	double windSpeed;
	double humidity;
	double pressure;
	double cloudiness;
	double fog;
	double lowClouds;
	double mediumClouds;
	double highClouds;
	int temperatureProbability;
	int windProbability;
	double precipitation;
	double precipitationMax;
	double precipitationMin;
	int symbol;
	double dewpointTemperature;
	int symbolProbability;
	void clear()
	{
		longitude = latitude = 0.0;
		altitude = 0;
		temperature = windDirection = windSpeed = humidity = pressure = cloudiness = fog = lowClouds = mediumClouds = highClouds = precipitation = precipitationMax = precipitationMin = dewpointTemperature = 0.0;
		symbol = temperatureProbability = windProbability = symbolProbability= 0;
		bTemperature = bWindDirection = bWindSpeed = bHumidity = bPressure = bCloudiness = bFog = bLowClouds = bMediumClouds = bHighClouds = bPrecipitation = bPrecipitationMax = bPrecipitationMin = bTemperatureProbability = bWindProbability = bSymbol = bDewpointTemperature = bSymbolProbability = false;
	};
};

class MetAstroLocation
{
public:
	double longitude;
	double latitude;
	int altitude;
	FILETIME sunrise;
	FILETIME sunset;
	bool never_rise;
	bool never_set;
	void clear()
	{
		longitude=latitude=0.0;
		altitude=0;
		never_rise=never_set=false;
		memset(&sunrise,0,sizeof(FILETIME));
		memset(&sunset,0,sizeof(FILETIME));
	};
};

class MetWeatherTime
{
public:
	FILETIME to;
	FILETIME from;
	std::string datatype;
	MetWeatherLocation location;
	void clear()
	{
		memset(&to,0,sizeof(FILETIME));
		memset(&from,0,sizeof(FILETIME));
		datatype.erase();
		location.clear();
	};
};

class MetAstroTime
{
public:
	FILETIME date;
	MetAstroLocation location;
	void clear()
	{
		memset(&date,0,sizeof(FILETIME));
		location.clear();
	};
};


class MetWeatherModel
{
public:
	FILETIME to;
	FILETIME from;
	FILETIME nextrun;
	FILETIME runended;
	FILETIME termin;
	std::string name;
	void clear()
	{
		memset(&to,0,sizeof(FILETIME));
		memset(&from,0,sizeof(FILETIME));
		memset(&nextrun,0,sizeof(FILETIME));
		memset(&runended,0,sizeof(FILETIME));
		memset(&termin,0,sizeof(FILETIME));
		name.erase();
	};
};

class MetWeatherData
{
public:
	FILETIME created;
	std::vector<MetWeatherTime> time;
	std::vector<MetWeatherModel> model;
	void clear()
	{
		memset(&created,0,sizeof(FILETIME));
		time.clear();
		model.clear();
	};
	bool getCurrentLocation(MetWeatherLocation& ml);
};

class MetAstroData
{
public:
	std::vector<MetAstroTime> time;
	void clear()
	{
		time.clear();
	}
	bool getCurrentLocation(MetAstroLocation& ml);
	bool isSunNow(MetAstroLocation& ml);
};

class WeatherRequest
{
public:
	double longitude;
	double latitude;
	int altitude;
};

class AstroRequest
{
public:
	double longitude;
	double latitude;
	int altitude;
	SYSTEMTIME date;
};

class Weather
{
public:
	bool abort;
	bool debug;
	HANDLE hThread;
	HANDLE hEvent;
	std::string forecastAddress;
	std::string astroAddress;
	std::queue<WeatherRequest> requestWeather;
	std::queue<AstroRequest> requestAstro;
	std::queue<MetWeatherData> weatherQue;
	std::queue<MetAstroData> astroQue;
	std::vector<MetWeatherData> weatherData;
	std::vector<MetAstroData> astroData;
	Weather(HANDLE event);
	virtual ~Weather();

	static void threadLoop(void* lpv);

	bool start();
	void stop();
	void setWeatherData(MetWeatherData& mwd);
	void setAstroData(MetAstroData& mad);
	void addRequest(const WeatherRequest& request);
	void addRequest(const AstroRequest& request);
	bool getRequest(WeatherRequest& request);
	bool getRequest(AstroRequest& request);
	void addWeatherData(char* data, int len);
	void addAstroData(char* data, int len);
	bool read(MetWeatherData& mwd);
	bool read(MetAstroData& mad);
};