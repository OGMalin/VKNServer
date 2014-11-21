#include <Windows.h>
#include <iostream>
#include <time.h>
#include "SNMP.h"
#include "ASN.h"
#include "Utility.h"

using namespace std;

char* pduString[] = {"Get request", "Get next request", "Get response", "Set request", "Trap v1", "Get bulk request", "Inform request", "Trap v2", "Report"};
char* trapString[] = {"Cold start", "Warm start", "Link down", "Link up", "Authentication failure", "EGP neighbor loss", "Enterprise specific"};

void SNMP::clear()
{
	version=0;
	community.clear();
	pdu=0;
	enterprise.clear();
	agentaddr=0;
	generictrap=0;
	specifictrap=0;
	timestamp=0;
	requestid=0;
	errorstatus=0;
	errorindex=0;
	variable.clear();
}

bool SNMP::set(const BYTE* data, int len, unsigned long addr)
{
	ASN asn;
	length=len;
	ip=addr;
	if (!asn.set(data))
		return false;
	if (asn.asn.size()!=3)
		return false;
//	cout << asn.toString() << endl; // Kun for debuging
	clear();
	version=asn.asn.at(0).Integer()+1;
	community=asn.asn.at(1).String();
	pdu=asn.asn.at(2).tag;
	switch (pdu)
	{
		case PDU_TRAP1:
			enterprise=asn.asn.at(2).asn.at(0).String();
			agentaddr=asn.asn.at(2).asn.at(1).Integer();
			generictrap=asn.asn.at(2).asn.at(2).Integer();
			specifictrap=asn.asn.at(2).asn.at(3).Integer();
			timestamp=asn.asn.at(2).asn.at(4).Integer();
			setVariables(asn.asn.at(2).asn.at(5));
			break;
		case PDU_GETRESPONSE:
		case PDU_TRAP2:
		case PDU_INFORMREQUEST:
			requestid=asn.asn.at(2).asn.at(0).Integer();
			errorstatus=asn.asn.at(2).asn.at(1).Integer();
			if (asn.asn.at(2).asn.at(1).PC==ASN_PRIMITIVE)
			{
				errorindex=asn.asn.at(2).asn.at(2).Integer();
				setVariables(asn.asn.at(2).asn.at(3));
			}else
			{
				setVariables(asn.asn.at(2).asn.at(2));
			}
			break;
		default:
			break;
	}
	return true;
}

void SNMP::setVariables(ASN &asn)
{
	std::vector<ASN>::iterator it=asn.asn.begin();
	while (it!=asn.asn.end())
	{
		if (it->PC==ASN_PRIMITIVE)
		{
			if (it->tag==ASN_OBJECTIDENTIFIER)
			{
				SNMPVariable v;
				v.id=it->String();
				++it;
				if (it!=asn.asn.end())
				{
					v.value=it->String();
					variable.push_back(v);
				}else
				{
					v.value="";
					variable.push_back(v);
					return;
				}
			}
		}else
		{
			setVariables(*it);
		}
		++it;
	}
}

std::string SNMP::toString()
{
	char sz[256];
	string s="";
	s += "Sender: " + iptoa(ip);
	_itoa_s(version,sz,256,10);
	s += "\nversjon: "  + string(sz);
	s += "\nCommunity: " + community;
	s += "\nPdu: " + string(pduString[pdu]);
	s += "\nEnterprise: " + enterprise;
	s += "\nAgent-adresse: " + iptoa(agentaddr);
	s += "\nGeneric trap: " + string(trapString[generictrap]);
	_itoa_s(specifictrap,sz,256,10);
	s += "\nSpecific trap: " + string(sz);
	_itoa_s(timestamp,sz,256,10);
	s += "\nTime stamp: " + string(sz);
	std::vector<SNMPVariable>::iterator vit=variable.begin();
	while (vit!=variable.end())
	{
		s += "\nObjekt: " + string(vit->id);
		s += ", Verdi: " + string(vit->value);
		++vit;
	}
	return s;
}

