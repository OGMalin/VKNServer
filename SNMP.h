#pragma once
#include <string>
#include <vector>
#include "ASN.h"

enum {PDU_GETREQUEST=0, PDU_GETNEXTREQUEST, PDU_GETRESPONSE, PDU_SETREQUEST, PDU_TRAP1, PDU_GETBULKREQUEST, PDU_INFORMREQUEST, PDU_TRAP2, PDU_REPORT};
enum {TRAP_COLDSTART=0, TRAP_WARMSTART, TRAP_LINKDOWN, TRAP_LINKUP, TRAP_AUTHENTICATIONFAILURE, TRAP_EGPNEIGHBORLOSS, TRAP_ENTERPRISESPECIFIC};

struct SNMPVariable
{
	std::string id;
	std::string value;
};

class SNMP
{
public:
	int length;
	int version;
	int pdu;
	std::string enterprise;
	unsigned long agentaddr;
	int generictrap;
	int specifictrap;
	unsigned int timestamp;
	std::string community;
	int requestid;
	int errorstatus;
	int errorindex;
  union {
    unsigned long ip;
    struct {char a,b,c,d;} cip;
  };
	std::vector<SNMPVariable> variable;
	void clear();
	bool set(const BYTE* data, int len, unsigned long addr);
	void setVariables(ASN& asn);
	// For debug
	std::string toString();
};