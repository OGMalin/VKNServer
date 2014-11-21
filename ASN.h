#pragma once
#include <Windows.h>
#include <vector>
#include <string>

enum {ASN_UNIVERSAL=0, ASN_APPLICATION, ASN_CONTEXTSPECIFIC, ASN_PRIVATE};
enum {ASN_PRIMITIVE=0, ASN_CONSTRUCTED};
enum {ASN_EOC=0, ASN_BOOLEAN, ASN_INTEGER, ASN_BITSTRING, ASN_OCTETSTRING, ASN_NULL, ASN_OBJECTIDENTIFIER,
			ASN_OBJECTDESCRIPTOR, ASN_EXTERNAL, ASN_REAL, ASN_ENUMERATED, ASN_EMBEDDEDPDV, ASN_UTF8STRING, ASN_RELATIVEOID,
			ASN_SEQUENCE=16, ASN_SET, ASN_NUMERICSTRING, ASN_PRINTABLESTRING, ASN_T61STRING, ASN_VIDEOTEXSTRING, ASN_IA5STRING,
			ASN_UTCTIME, ASN_GENERALIZEDTIME, ASN_GRAPHICSTRING, ASN_VISIBLESTRING, ASN_GENERALSTRING, ASN_UNIVERSALSTRING,
			ASN_CHARACTERSTRING, ASN_BMPSTRING};

class ASN
{
public:
	int Class;
	int PC;
	int tag;
	int length;
	BYTE content[512];
	std::vector<ASN> asn;
	ASN::ASN();
	void clear();
	bool add(ASN& a);
	bool set(const BYTE* data);
	bool set(int objectname);
	bool set(int objectname, int value);
	bool set(int objectname, std::string value);
	bool set(int c, int p, int t);
	// Converterer objectet til raw format
	int get(BYTE* data);
	// Hent integer verdi
	int Integer();
	// Hent tekst verdi
	std::string String();
	// Få hele objektet som tekst for debug.
	std::string toString(std::string prefix="");
	// Lager en objectidentifier bitstrøm. Returnerer lengden;
	int makeObjectIdentifier(std::string s, BYTE* data);
	// Finner lengden
	int getASNLength(const BYTE* data, int& start);
};

