#pragma once

#include <Windows.h>
#include <vector>

// Start byte
#define FIXEDFRAME		0x10
#define VARIABLEFRAME	0x68

#define MAXFRAMELEN		20

// End byte
#define END						0x16

// Function code
// Primary
#define FC_P_RESETLINK				0		// Expect confirm
#define FC_P_RESETUSER				1		// Expect confirm
#define FC_P_TEST							2		// Expect confirm
#define FC_P_USERDATA1				3		// Expect confirm
#define FC_P_USERDATA2				4		// No reply
#define FC_P_REQACCESSDEMAND	8		// Expect respond
#define FC_P_REQSTATUS				9		// Expect respond
#define FC_P_REQCLASS1				10	// Expect respond
#define FC_P_REQCLASS2				11	// Expect respond
// Secondary
#define FC_S_ACK							0		// Confirm
#define FC_S_NOTACCEPTED			1		// Confirm
#define FC_S_LINKBUSY					1		// Confirm
#define FC_S_USERDATA					8		// Respond
#define FC_S_NODATA						9		// Respond
#define FC_S_STATUSOFLINK			11	// Respond
#define FC_S_ACCESSDEMAND			11	// Respond
#define FC_S_NOTFUNCTIONING		14
#define	FC_S_NOTIMPLEMETED		15

// Cause of transmition
#define COT_PERIODIC							1
#define COT_BACKGROUND						2
#define COT_SPONTANEOUS						3
#define COT_INITIALISED						4
#define COT_REQUEST								5
#define COT_ACTIVATION					  6
#define COT_ACTIVATIONCONFIRM			7
#define COT_DEACTIVATION					8
#define COT_DEACTIVATIONCONFIRM		9
#define COT_ACTIVATIONTERMINATION	10
#define COT_REMOTECOMMAND					11
#define COT_LOCALCOMMAND					12
#define COT_FILETRANSFER					13
#define COT_INTERROGATION					20
#define COT_UNKNOWNTYPE						44
#define COT_UNKNOWNCAUSE					45
#define COT_UNKNOWNCOMMON					46
#define COT_UNKNOWNINFORMATION		47 


// Qualifier of interrogation
#define QOI_STATION_INTERROGATION	20

#define BROADCAST									0xff

// ASDU typer i bruk av NUC pluss 104 typer
// Prosessinformasjon fra RTU
#define M_SP_NA_1				1		// Single-point information
#define M_DP_NA_1				3		// Double-point information
#define M_ST_NA_1				5		// Step position information
#define M_BO_NA_1				7		// Bitstring of 32 bit
#define M_ME_NA_1				9		// Measured value, normalized value
#define M_ME_NB_1				11	// Measured value, scaled value
#define M_ME_NC_1				13	// Measured value, short floating point value		
#define M_IT_NA_1				15	// Integrated totals
#define M_PS_NA_1				20	// Packed single-point information with status change detection
#define M_ME_ND_1				21	// Measured value, normalized value without quality descriptor
#define M_SP_TB_1				30	// Single-point information with time tag CP56Time2a
#define M_DP_TB_1				31	// Double-point information with time tag CP56Time2a
#define M_ST_TB_1				32	// Step position information with time tag CP56Time2a
#define M_BO_TB_1				33	// Bitstring of 32 bit with time tag CP56Time2a
#define M_ME_TD_1				34	// Measured value, normalized value with time tag CP56Time2a
#define M_ME_TE_1				35	// Measured value, scaled value with time tag CP56Time2a
#define M_ME_TF_1				36	// Measured value, short floating point value with time tag CP56Time2a
#define M_IT_TB_1				37	// Integrated totals value with time tag CP56Time2a
#define M_EP_TD_1				38	// Event of protection equipment with time tag CP56Time2a
#define M_EP_TE_1				39	// Packed start events of protection equipment with time tag CP56Time2a
#define M_EP_TF_1				40	// Packed output circuit information of protection equipment with time tag CP56Time2a
// Prosessinformasjon til RTU
#define C_SC_NA_1				45	// Single command
#define C_DC_NA_1				46	// Double command
#define C_RC_NA_1				47	// Regulating step command
#define C_SE_NA_1				48	// Set point command, normalised value
#define C_SE_NB_1				49	// Set point command, scaled value
#define C_SE_NC_1				50	// Set point command, short floating point value
#define C_BO_NA_1				51	// Bitstring of 32 bit
#define C_SC_TA_1				58	// Single command with time tag CP56Time2a
#define C_DC_TA_1				59	// Double command with time tag CP56Time2a
#define C_RC_TA_1				60	// Regulating step command with time tag CP56Time2a
#define C_SE_TA_1				61	// Set point command, normalised value with time tag CP56Time2a
#define C_SE_TB_1				62	// Set point command, scaled value with time tag CP56Time2a
#define C_SE_TC_1				63	// Set point command, short floating point value with time tag CP56Time2a
#define C_BO_TA_1				64	// Bitstring of 32 bit with time tag CP56Time2a
// Systeminformasjon fra RTU
#define M_EI_NA_1				70	// End of initialization
// Systeminformasjon til RTU
#define C_IC_NA_1				100	// Interrrogation command
#define C_CI_NA_1				101	// Counter interrrogation command
#define C_RD_NA_1				102	// Read command
#define C_CS_NA_1				103	// Clock syncronization command
#define C_TS_NB_1				104	// Test command
#define C_RP_NC_1				105	// Reset process command
#define C_CD_NA_1				106	// Delayed acquistion command
#define C_TS_TA_1				107	// Test command with time tag CP56Time2a
// Parameter til RTU
#define P_ME_NA_1				110	// Parameter of measured value, normalized value
#define P_ME_NB_1				111	// Parameter of measured value, scaled value
#define P_ME_NC_1				112	// Parameter of measured value, short floating-point number
#define P_AC_NA_1				113	// Parameter activation
// Filoverføring
#define F_FR_NA_1				120	// File ready
#define F_SR_NA_1				121	// Section ready
#define F_SC_NA_1				122	// Call directory, select file, call file, call section
#define F_LS_NA_1				123	// Last section, last segment
#define F_AF_NA_1				124	// Ack file, ack section
#define F_SG_NA_1				125	// Segment
#define F_DR_TA_1				126	// Directory
#define F_SC_NB_1				127	// Query Log - Request archive file

#define DPI_INTERMEDIATE					0
#define DPI_OFF										1
#define DPI_ON										2
#define DPI_ERROR									3

#define SPI_OFF										0
#define SPI_ON										1

#define DCS_OFF										1
#define DCS_ON										2

#define SCS_OFF										0
#define SCS_ON										1


// Fixed test pattern
#define FBP												0x55AA

struct CP56Time2a
{
	unsigned int	ms		: 16; // 0..59 999 ms
	unsigned int	min		: 6;  // 0..59 min.
	unsigned int	res1	: 1;
	unsigned int	IV		: 1;  // Invalid time
	unsigned int	h			: 5;  // 0..23 h
	unsigned int	res2	: 2;
	unsigned int	SU		:	1;
	unsigned int	d			: 5;  // 0..31 Dag i måneden
	unsigned int	wd		: 3;  // 1..7 Dag i uka
	unsigned int	m			: 4;  // 1..12 Måned
	unsigned int	res3	: 4;
	unsigned int	y			: 7;  // 0..99 År
	unsigned int	res4	: 1;
};

struct VariableStructureQualifier
{
	unsigned int Number : 7;
	unsigned int SQ			: 1;
};

struct CauseOfTransmission
{
	unsigned int cause			: 6;
	unsigned int confirm		: 1;  // 0=positive, 1= negative
	unsigned int test				: 1;  // 0=no test, 1=test
	unsigned int originator	: 8;  // Optional originator address
};

// Single-Point Information
struct SIQ
{
	unsigned int SPI	: 1; // 0=OFF, 1=ON
	unsigned int res	: 3;
	unsigned int BL		: 1; // Blocked
	unsigned int SB		: 1; // Substituted
	unsigned int NT		: 1; // Not topical
	unsigned int IV		: 1; // Invalid
};

// Double-point Information
struct DIQ
{
	unsigned int DPI	: 2; // 0=Intermediate, 1=OFF, 2=ON, 3=Intermediate
	unsigned int res	: 2;
	unsigned int BL		: 1; // Blocked
	unsigned int SB		: 1; // Substituted
	unsigned int NT		: 1; // Not topical
	unsigned int IV		: 1; // Invalid
};

// Measured value
struct QDS
{
	unsigned int OV		: 1; // Overflow
	unsigned int res	: 3;
	unsigned int BL		: 1; // Blocked
	unsigned int SB		: 1; // Substituted
	unsigned int NT		: 1; // Not topical
	unsigned int IV		: 1; // Invalid
};

// Value with transient state indication
struct VTI
{
	unsigned int Value			: 7;
	unsigned int Transient	: 1;
};

struct BCR
{
	_int32	counter	: 32;
	unsigned int SQ	: 5; // Sequence number
	unsigned int CY	: 1; // Counter overflow
	unsigned int CA	: 1; // Counter adjusted
	unsigned int IV	: 1; // Invalid
};

struct SCO
{
	unsigned int SCS	: 1; // 0=OFF, 1=ON
	unsigned int res	: 1; // Fast 0
	unsigned int QU		: 5;
	unsigned int ES		: 1; // 0=Execute, 1=Select
};

struct DCO
{
	unsigned int DCS	: 2; // 0= Not permitted, 1=OFF, 2=ON, 3=Not permitted
	unsigned int QU		: 5;
	unsigned int ES		: 1; // 0=Execute, 1=Select
};

struct RCO
{
	unsigned int RCS	: 2; // 0= Not permitted, 1=Next LOWER, 2=Next HIGHER, 3=Not permitted
	unsigned int QU		: 5;
	unsigned int ES		: 1; // 0=Execute, 1=Select
};

struct COI
{
	unsigned int cause	: 7; // 
	unsigned int change	: 1; // Initialisation after change of local parameters
};

struct QPM
{
	unsigned int KPA	: 6;
	unsigned int LPC	:	1;
	unsigned int POP	: 1;
};

/*
struct QOS
{
	unsigned int QL	: 7;
	unsigned int ES	: 1;
};
*/
class DataUnitIdentifier
{
public:
	BYTE ident;
	VariableStructureQualifier qualifier;
	CauseOfTransmission cause;
	WORD common;
	DataUnitIdentifier(){clear();};
	void clear()
	{
		memset(&qualifier,0,1);
		ident=0;
		memset(&cause,0,2);
		common=0;
	};
};

class InformationObject
{
public:
	DWORD address;
	union // 1 byte verdier
	{
		SIQ siq;					// 1, 30
		DIQ diq;					// 3, 31
		QDS qds;					// 5, 7, 9, 13, 32, 34, 36
		SCO sco;					// 45, 58
		DCO dco;					// 46, 59
		RCO rco;					// 47, 60
//		QOS qos;					// 48, 61
		COI coi;					// 70
		BYTE qoi;         // 100
		BYTE qcc;					// 101
		BYTE qrp;					// 105
		QPM qpm;					// 110, 112
	};
	union // Lengste verdi er 5 byte
	{
		VTI vti;					// 5, 32
		DWORD bsi;				// 7, 51
		WORD nva;					// 9, 34, 48, 61
		float value;			// 13, 36, 112
		BCR bcr;					// 37
		WORD fbp;
	};

	CP56Time2a cp56time; // 30, 31, 32, 34, 36, 103

	InformationObject(){clear();};
	void clear()
	{
		address=0;
		qoi=0;
		memset(&bcr,0,5);
		memset(&cp56time,0,7);
	};
};

class ASDU
{
public:
	DataUnitIdentifier dui;
	std::vector<InformationObject> io;
	ASDU();
	virtual ~ASDU();
	virtual void clear();
	int get(BYTE* data);
	void set(const BYTE* data);
	bool valid();
	// Returnerer lengden på denne io.
	int addIO(InformationObject& i);
};

extern const SYSTEMTIME cp56time2systemtime(const CP56Time2a& cp);
extern const CP56Time2a currentTime();
extern int unstructured(const std::string& s);
extern char* structured(WORD address, char* buffer, int size);
