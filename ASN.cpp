#include "ASN.h"
#include "Utility.h"

using namespace std;

char* classString[] = {"Universal", "Application", "Context-specific", "Private"};
char* pcString[] = {"Primitive", "Constructed"};
char* tagString[] = {"EOC", "BOOLEAN", "INTEGER", "BITSTRING", "OCTETSTRING", "NULL", "OBJECTIDENTIFIER",
			"OBJECTDESCRIPTOR", "EXTERNAL", "REAL", "ENUMERATED", "EMBEDDEDPDV", "UTF8STRING", "RELATIVEOID",
			"Undefined", "Undefined",
			"SEQUENCE", "SET", "NUMERICSTRING", "PRINTABLESTRING", "T61STRING", "VIDEOTEXSTRING", "IA5STRING",
			"UTCTIME", "GENERALIZEDTIME", "GRAPHICSTRING", "VISIBLESTRING", "GENERALSTRING", "UNIVERSALSTRING",
			"CHARACTERSTRING", "BMPSTRING"};

int ASN::getASNLength(const BYTE* data, int& start)
{
	int length=0;

	if (data[1]>0x80)
	{
		switch (data[1]&0x7f)
		{
			case 1:
				length=data[2];
				start=3;
				break;
			case 2:
				length=0x100*data[2]+data[3];
				start=4;
				break;
			case 3:
				length=0x10000*data[2]+0x100*data[3]+data[4];
				start=5;
				break;
			case 4:
				length=0x1000000*data[2]+0x10000*data[3]+0x100*data[4]+data[5];
				start=6;
				break;
			default:
				start=2;
				break;
		};
	}else if (data[1]==0x80)
	{
		int i=2;
		while (i<512)
		{
			if ((data[i]==0) && (data[i+1]==0))
			{
				length=i-2;
				break;
			}
			++i;
		}
		start=2;
	}else
	{
		length=data[1];
		start=2;
	};
	return length;
};

ASN::ASN()
{
	clear();
}

void ASN::clear()
{
	Class=0;
	PC=0;
	tag=0;
	length=0;
	asn.clear();
}

bool ASN::set(int objectname, int value)
{
	int v0,v1,v2,v3,v4;
	switch (objectname)
	{
		case ASN_NULL:
			set(ASN_UNIVERSAL, ASN_PRIMITIVE, ASN_NULL);
			break;
		case ASN_INTEGER:
			set(ASN_UNIVERSAL, ASN_PRIMITIVE, ASN_INTEGER);
			v0=value&0x7f;
			v1=(value>>7)&0x7f;
			v2=(value>>14)&0x7f;
			v3=(value>>21)&0x7f;
			v4=(value>>28)&0x0f;
			if (v4)
				content[length++]=v4|0x80;
			if (v3)
				content[length++]=v3|0x80;
			if (v2)
				content[length++]=v2|0x80;
			if (v1)
				content[length++]=v1|0x80;
			content[length++]=v0;
			break;
		default:
			return false;
	}
	return true;
}

bool ASN::set(int objectname)
{
	switch (objectname)
	{
		case ASN_SEQUENCE:
			set(ASN_UNIVERSAL, ASN_CONSTRUCTED, ASN_SEQUENCE);
			break;
		default:
			return false;
	}
	return true;
}

bool ASN::add(ASN& a)
{
	if (PC!=ASN_CONSTRUCTED)
		return false;
	BYTE data[512];
	int len=a.get(data);
	memcpy(&content[length],data,len);
	length+=len;
	return true;
}

bool ASN::set(int objectname, std::string value)
{
	switch (objectname)
	{
		case ASN_OCTETSTRING:
			set(ASN_UNIVERSAL, ASN_PRIMITIVE, ASN_OCTETSTRING);
			length=(int)value.length();
			memcpy(content,value.c_str(),length);
			break;
		case ASN_INTEGER:
			return set(ASN_INTEGER,atoi(value.c_str()));
		case ASN_OBJECTIDENTIFIER:
			set(ASN_UNIVERSAL, ASN_PRIMITIVE, ASN_OBJECTIDENTIFIER);
			length=makeObjectIdentifier(value,content);
			break;
		default:
			return false;
	}
	return true;
}

bool ASN::set(int c, int p, int t)
{
	if (c>3)
		return false;
	if (p>1)
		return false;
	if (t>31)
		return false;
	Class=c;
	PC=p;
	tag=t;
	length=0;
	return true;
}

int ASN::makeObjectIdentifier(std::string s, BYTE* data)
{
	int i=1,x,y,index=0,x0,x1,x2,x3,x4;
	string w;
	// Første byte er spesiell (X*40)+Y
	w=getWord(s,i++,".");
	x=atoi(w.c_str());
	w=getWord(s,i++,".");
	y=atoi(w.c_str());
	data[index++]=(x*40)+y;

	w=getWord(s,i++,".");
	while (w.length())
	{
		x=atoi(w.c_str());
		x0=x&0x7f;
		x1=(x>>7)&0x7f;
		x2=(x>>14)&0x7f;
		x3=(x>>21)&0x7f;
		x4=(x>>28)&0x0f;
		if (x4)
			data[index++]=x4|0x80;
		if (x3)
			data[index++]=x3|0x80;
		if (x2)
			data[index++]=x2|0x80;
		if (x1)
			data[index++]=x1|0x80;
		data[index++]=x0;
		w=getWord(s,i++,".");
	}
	return index;
}

int ASN::get(BYTE* data)
{
	int index = 0;
	data[index++]=(Class<<6)|(PC<<5)|tag;
	if (length>=0x80)
	{
		data[index++]=length>>7;
		data[index++]=length&0x7f;
	}else
	{
		data[index++]=length;
	}
	memcpy(&data[index],content,length);
	return (length+index);
}

bool ASN::set(const BYTE* data)
{
	ASN a;
	int index;
	int i;
	int len;

	Class=data[0]>>6;
	PC=(data[0]>>5)&0x01;
	tag=data[0]&0x1f;
	length=getASNLength(data, index);

	if (length>512)
		return false;

	memcpy(content,&data[index],length);

	if (PC==ASN_PRIMITIVE)
		return true;

	i=0;
	while (i<length)
	{
		a.clear();
		len=getASNLength(&content[i], index);
		if (a.set(&content[i]))
			asn.push_back(a);
		i+=len+index;
	}
	return true;
}

int ASN::Integer()
{
	int	val;
	switch (length)
	{
		case 1:
			val=content[0]&0x7f;
			if (content[0]&0x80)
				val=0x80-val;
			break;
		case 2:
			val=(0x100*(content[0]&0x7f))+content[1];
			if (content[0]&0x80)
				val=0x8000-val;
			break;
		case 3:
			val=(0x10000*(content[0]&0x7f))+0x100*content[1]+content[2];
			if (content[0]&0x80)
				val=0x800000-val;
			break;
		case 4:
			val=0x1000000*content[0]+0x10000*content[1]+0x100*content[2]+content[3];
			break;
		default:
			val=0;
			break;
	}

	return val;
}

std::string ASN::String()
{
	string s;
	int val;
	int i,j;
	char sz[512];
	switch (tag)
	{
		case ASN_BOOLEAN:
			if (content[0]==0)
				s="FALSE";
			else
				s="TRUE";
			break;
		case ASN_INTEGER:
		case ASN_ENUMERATED:
			val=Integer();
			sprintf_s(sz,512,"%i",val);
			s=sz;
			break;
		case ASN_OCTETSTRING:
		case ASN_UTF8STRING:
		case ASN_NUMERICSTRING:
		case ASN_PRINTABLESTRING:
		case ASN_T61STRING:
		case ASN_VIDEOTEXSTRING:
		case ASN_IA5STRING:
		case ASN_GRAPHICSTRING:
		case ASN_VISIBLESTRING:
		case ASN_GENERALSTRING:
		case ASN_UNIVERSALSTRING:
		case ASN_CHARACTERSTRING:
			if (length<512)
				content[length]=0;
			else
				content[511]=0;
			s=(char*)content;
			break;
		case ASN_OBJECTIDENTIFIER:
			s="";
			if (length>0)
			{
				sprintf_s(sz,512,"%i.%i",content[0]/40,content[0]%40);
				s=sz;
				i=1;
				while (i<length)
				{
					s+=".";
					j=0;
					while (content[i+j]>=0x80)
						++j;
					++j;
					switch (j)
					{
						case 1:
							val=content[i];
							break;
						case 2:
							val=(content[i]&0x7f)<<7;
							val+=content[i+1];
							break;
						case 3:
							val=(content[i]&0x7f)<<14;
							val+=(content[i+1]&0x7f)<<7;
							val+=content[i+2];
						case 4:
							val=(content[i]&0x7f)<<21;
							val+=(content[i+1]&0x7f)<<14;
							val+=(content[i+2]&0x7f)<<7;
							val+=content[i+3];
							break;
						default:
							val=0;
							break;
					}
					i+=j;
					sprintf_s(sz,512,"%i",val);
					s+=sz;
				};
			};

			break;
		default:
			s="";
			break;
	};
	return s;
}

std::string ASN::toString(std::string prefix)
{
	string s="";
	int i;

	s+=prefix+"------\n";
	s+=prefix+"Class="+classString[Class]+"\n";
	s+=prefix+"PC="+pcString[PC]+"\n";
	s+=prefix+"Tag="+tagString[tag]+"\n";


	if (PC==ASN_PRIMITIVE)
	{
		s+=prefix+"Verdi="+String()+"\n";
		return s;
	}

	i=0;
	std::vector<ASN>::iterator it=asn.begin();
	while (it!=asn.end())
	{
		s+=it->toString(prefix+"\t");
		++it;
	}
	return s;
}
