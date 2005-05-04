// HpiHandler.cpp: implementation of the CHpiHandler class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "stdafx.h"
#include "hpihandler.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CHpiHandler* hpiHandler=new CHpiHandler();

CHpiHandler::CHpiHandler()
{
}

CHpiHandler::~CHpiHandler()
{
}

int CHpiHandler::GetFileSize(string name)
{
	return 0;
}

int CHpiHandler::LoadFile(string name, void *buffer)
{
	return 0;
}

void CHpiHandler::MakeLower(string &s)
{
	for(unsigned int a=0;a<s.size();++a){
		if(s[a]>='A' && s[a]<='Z')
			s[a]=s[a]+'a'-'A';
	}
}

void CHpiHandler::MakeLower(char *s)
{
	for(int a=0;s[a]!=0;++a)
		if(s[a]>='A' && s[a]<='Z')
			s[a]=s[a]+'a'-'A';
}

std::vector<std::string> CHpiHandler::GetFilesInDir(std::string dir)
{
	std::vector<std::string> found;

	return found;
}
