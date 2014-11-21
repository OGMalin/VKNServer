/**
	* XML.h
	* 13.09.2011  Init
**/

#ifndef __XML_h
#define __XML_h

#include <string>
#include <vector>
#include <map>

class XMLElement
{
public:
	std::string name;
  std::string text;
  std::map<std::string,std::string> attribute;
  std::vector<XMLElement> element;
	XMLElement();
	virtual ~XMLElement();
	virtual void clear();
  void interprete(const std::string& s);
};

class XML
{
  bool getElement(int n, int& current, std::vector<XMLElement>& cel, XMLElement& el);
  void writeBlock(XMLElement& xe, std::string& s,std::string tab);
public:
  std::string	version;
  std::string	encoding;
  XMLElement	root;
	XML();
	virtual ~XML();
	virtual bool read(const std::string& xmlstring);
	virtual const std::string write();
	virtual bool readfile(const std::string& filename);
	virtual bool writefile(const std::string& filename);
	const std::string expandSingleBlock(const std::string& s);
  static const std::string getFirstBlock(const std::string& s);
  static const std::string stripentities(const std::string& s);
  bool getElement(int n, XMLElement& el);
};

#endif // __XML_h