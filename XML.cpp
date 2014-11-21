/**
	* XML.cpp
	* 13.09.2011  Init
**/

#include "XML.h"
#include <fstream>
#include "Utility.h"

using namespace std;

XMLElement::XMLElement()
{
};

XMLElement::~XMLElement()
{
};

void XMLElement::clear()
{
	name.erase();
	text.erase();
	attribute.clear();
	element.clear();
};

void XMLElement::interprete(const std::string &s)
{
  int count;
	size_t len,i,j;
  char quote;
  string ss,sss,attr,val,esc="\27";
  XMLElement e;

  len=s.length();

  // Find headtag
  while ((i=s.find("<",0))!=string::npos)
  {
    if (s.at(i+1)>='A')
      break;
  }
  if (i==string::npos)
    return;

  // Get name
  name=getWord(s.substr(i+1),1,"\n =>/");
  i+=name.length()+1;

  // Find end of head tag (>) and attributes
  while (i<len)
  {
    if (s.at(i)=='>') // End
    {
       break;
    }else if (s.at(i)>='0') // Attribute
    {
      attr=getWord(s.substr(i),1,"\n =>");
      val="";
      i+=attr.length();
      while (i<len)
      {
        if (s.at(i)=='=')
        {
          ++i;
          while(i<len)
          {
            if (((quote=s.at(i))=='\'') || ((quote=s.at(i))=='\"'))
            {
              ++i;
              while (i<len)
              {
                if (s.at(i)==quote)
                  break;
                val+=s.at(i);
                ++i;
              }
              break;
            }
            ++i;
          }
          break;
        }else if (s.at(i)==' ')
        {
          break;
        }else if (s.at(i)=='>')
        {
          --i;
          break;
        }
        ++i;
      }
      attribute.insert(map<string,string>::value_type(attr,val));
    }
    ++i;
  }
	if (i > len)
		return;
  if (s.at(i-1)=='/')
    return;

  ++i;
  if (i>=len) // Error in xml document
    return;

  // Find body
  count=1;
  j=i;
  while (j<len)
  {
    if (s.at(j)=='<')
    {
      if (s.at(j+1)=='/')
      {
        if (getWord(s.substr(j+2),1,"\n >")==name)
        {
          --count;
          if (count==0)
            break;
        }
      }else if (getWord(s.substr(j+1),1,"\n >")==name)
      {
        ++count;
        j+=1,name.length();
      }
    }
    ++j;
  }
  text=trim(s.substr(i,j-i));
//  text=s.substr(i,j-i);

  // Interprete body
  i=0;
  len=text.length();
	if (!len)
		return;
  while (i<(len-1))
  {
    if (text.at(i)=='<')
    {
      if (text.at(i+1)>='A')
      {
        ss=XML::getFirstBlock(text.substr(i));
        if (!ss.length()) // Error
          return;
        e.clear();
        e.interprete(ss);
        element.push_back(e);
        text.replace(i,ss.length(),esc);
//        i+=strlen(esc)-1;
        len=text.length();
      }
    }
    ++i;
  }
  text=trim(text);
}

XML::XML()
{
	version="1.0";
	encoding="utf-8";
};

XML::~XML()
{
};

// Convert <block/> to <block></block>
const std::string XML::expandSingleBlock(const std::string& s)
{
  string block,start;
	size_t len, p1, p2;
  char quot,c;

  len=s.length();
  p1=0;
  start="";
  first:
  while (p1<len)
  {
    c=s.at(p1);
    block+=s.at(p1);
    if ((s.at(p1)=='<') && ((p1+1)<len) && (s.at(p1+1)>='0') && (s.at(p1+1)!='/'))
    {
      ++p1;
      start=getWord(s.substr(p1),1," >/\n");
      while (p1<(len-1))
      {
        c=s.at(p1);
        if (s.at(p1)=='>')
        {
          block+=s.at(p1);
          ++p1;
          break;
        }else if (s.at(p1)=='/')
        {
          p2=p1+1;
          while (p2<len)
          {
            c=s.at(p2);
            if ((s.at(p2)>' ')&& (s.at(p2)!='>'))
            {
              block+=s.at(p2);
              p1=p2+1;
              goto first;
            }else if (s.at(p2)=='>')
            {
              block+="></";
              block+=start;
              block+=">";
              p1=p2+1;
              goto first;
            }
            ++p2;
          }
        }else if (((quot=s.at(p1))=='\'') || ((quot=s.at(p1))=='\"'))
        {
          block+=s.at(p1);
          ++p1;
          while (p1<len)
          {
            c=s.at(p1);
            block+=s.at(p1);
            if (s.at(p1)==quot)
            {
              ++p1;
              break;
            }
            ++p1;
          }
        }else
        {
          block+=s.at(p1);
          ++p1;
        }
      }
    }else
    {
      ++p1;
    }
  }
  return block;
}

const std::string XML::getFirstBlock(const std::string& s)
{
  int count;
	size_t len,p1,p2;
  string startTag;
  string ss;
  p1=0;
  len=s.length();

  while (p1<(len-1))
  {
    if ((s.at(p1)=='<') && (s.at(p1+1)>='0') && (s.at(p1+1)!='/'))
    {
      startTag=getWord(s.substr(p1+1),1,"\n >");
      count=1;
      p2=p1+1+startTag.length();
      while (p2<(len-1))
      {
        if (s.at(p2)=='<')
        {
          if (s.at(p2+1)=='/')
          {
            if (getWord(s.substr(p2+2),1,"\n >")==startTag)
            {
              --count;
              if (count==0)
              {
                p2=s.find(">",p2+2);
                return s.substr(p1,p2-p1+1);
              }
            }
          }else if (getWord(s.substr(p2+1),1,"\n >")==startTag)
          {
            ++count;
          }
        }
        ++p2;
      }
    }
    ++p1;
  }
  return "";
}

bool XML::read(const std::string& xmlstring)
{
  string s;
  size_t i,j;
  i=-1;
  if ((i=xmlstring.find("<?xml",i+1))==string::npos)
    return false;
  if ((j=xmlstring.find("?>",i+1))==string::npos)
    return false;
  s=xmlstring.substr(i,j-i+2);
	if ((i=s.find(" version=",0))!=string::npos)
    version=getQuote(s.substr(i+9));
  if ((i=s.find(" encoding=",0))!=string::npos)
    encoding=getQuote(s.substr(i+10));
  i=j-1;
  while ((i=xmlstring.find("<",i+1))!=string::npos)
  {
		if (i >= (xmlstring.length() - 1))
			return false;
    if (xmlstring.at(i+1)>='A')
      break;
  }
  if (i==string::npos)
    return false;
  s=xmlstring.substr(i);

  // Only new line are used in xml
  i=-1;
  while ((i=s.find("\r\n",i+1))!=string::npos)
    s.replace(i,2,"\n");
  i=-1;
  while ((i=s.find("\r",i+1))!=string::npos)
    s.replace(i,1,"\n");

  s=expandSingleBlock(s);
  s=getFirstBlock(s);  

	root.interprete(s);
  return true;
};

const std::string XML::write()
{
	string xmlstring;
  // Header
  xmlstring="<?xml version=\"";
  xmlstring+=version;
  xmlstring+="\" encoding=\"";
  xmlstring+=encoding;
  xmlstring+="\"?>\n";

  writeBlock(root,xmlstring,"");

	return xmlstring;
};

bool XML::readfile(const std::string& filename)
{
	string content;
	char ch;
	ifstream ifs(filename);
	if (!ifs)
		return false;
	while (ifs.get(ch))
		content+=ch;
	ifs.close();
	return read(content);
};

bool XML::writefile(const std::string& filename)
{
  string content;
	ofstream ofs(filename);
  content=write();
  if (content.empty())
    return false;
	ofs << content;
	ofs.close();
	return true;
};

const std::string XML::stripentities(const std::string& s)
{
  size_t i=0,j;
  char sz[16];
  string ss=s;
  while ((i=ss.find("&#",i))!=string::npos)
  {
    j=ss.find(";",i);
    if ((j!=string::npos) && (j<(i+6)))
    {
      sz[0]=(char)atoi(ss.substr(i+2).c_str());
      sz[1]='\0';
      ss.replace(i,j-i+1,sz);
      ++i;
    }else
    {
      i+=2;
    }
  }
  return ss;
}

void XML::writeBlock(XMLElement& xe, std::string& s, std::string tab)
{
  char c,esc='\27';
  size_t len,i;
  vector<XMLElement>::iterator xit;
  map<string,string>::iterator ait;

  // Write tag
  s+=tab;
  s+="<";
  s+=xe.name;
  ait=xe.attribute.begin();
  while (ait!=xe.attribute.end())
  {
    s+=" ";
    s+=ait->first;
    s+="=\"";
    s+=ait->second;
    s+="\"";
    ++ait;
  }
  s+=">\n";
  
  // Write body;
  xit=xe.element.begin();
  len=xe.text.length();
  for (i=0;i<len;i++)
  {
    c=xe.text[i];
    if (c==esc)
    {
      if (xit!=xe.element.end())
      {
        writeBlock(*xit,s,tab+"  ");
        ++xit;
      }
    }else
    {
//      s+=c;
    }
  }
  while (xit!=xe.element.end())
  {
    writeBlock(*xit,s,tab+"  ");
    ++xit;
  }

  s+=tab;
  s+="</";
  s+=xe.name;
  s+=">\n";
}

bool XML::getElement(int n, int& current, std::vector<XMLElement>& cel, XMLElement& el)
{
  vector<XMLElement>::iterator it=cel.begin();
  while (it!=cel.end())
  {
    ++current;
    if (n==current)
    {
      el=*it;
      return true;
    }
    if (getElement(n,current,it->element,el))
      return true;
    ++it;
  }
  return false;
}

bool XML::getElement(int n, XMLElement& el)
{
  if (n==0)
  {
    el=root;
    return true;
  }
  int current=0;
  return getElement(n,current,root.element,el);
}

