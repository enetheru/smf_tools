// HpiHandler.h: interface for the CHpiHandler class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#if !defined(AFX_HPIHANDLER_H__68DF4969_8792_4893_98F7_8092C36479D7__INCLUDED_)
#define AFX_HPIHANDLER_H__68DF4969_8792_4893_98F7_8092C36479D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <map>
#include <vector>

using std::string;
using std::map;

class CHpiHandler  
{
public:
	void MakeLower(char* s);
	void MakeLower(string &s);
	int LoadFile(string name,void* buffer);
	int GetFileSize(string name);
	CHpiHandler();
	virtual ~CHpiHandler();

	struct FileData{
		string hpiname;
		int size;
	};
	map<string,FileData> files;
	std::vector<std::string> GetFilesInDir(std::string dir);
};

extern CHpiHandler* hpiHandler;

#endif // !defined(AFX_HPIHANDLER_H__68DF4969_8792_4893_98F7_8092C36479D7__INCLUDED_)
