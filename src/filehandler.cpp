#include "filehandler.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include "filefunctions.h"
//#include <boost/filesystem/exception.hpp>
#ifdef WIN32
#include "stdafx.h"
#endif

using namespace std;

CFileHandler::CFileHandler(const char* filename)
: filesize(-1),
	ifs(0)
{
	Init(filename);
}

CFileHandler::CFileHandler(std::string filename)
: filesize(-1),
	ifs(0)
{
	Init(filename.c_str());
}

void CFileHandler::Init(const char* filename)
{
	string fnstr;
	ifs = 0;
	printf("Opening %s\n",filename);
	try {
		fs::path fn(filename);
		fnstr = fn.string();
	} catch (void* err) {
		printf("Caught filesystem error in file %s\n",filename);
	//} catch (boost::filesystem::filesystem_error err) {
		fnstr.clear();
	}

	if (!fnstr.empty())
	{
		ifs=new std::ifstream(fnstr.c_str(), ios::in|ios::binary);
		if (ifs->is_open()) {
			ifs->seekg(0, ios_base::end);
			filesize = ifs->tellg();
			ifs->seekg(0, ios_base::beg);
			return;
		}
		delete ifs;
		ifs = 0;
	}
}

CFileHandler::~CFileHandler(void)
{
	if(ifs)
		delete ifs;
}

bool CFileHandler::FileExists()
{
	return ifs;
}

void CFileHandler::Read(void* buf,int length)
{
	if(ifs){
		ifs->read((char*)buf,length);
	}
}

void CFileHandler::Seek(int length)
{
	if(ifs){
		ifs->seekg(length);
	}
}

int CFileHandler::Peek()
{
	if(ifs){
		return ifs->peek();
	}
	return EOF;
}

bool CFileHandler::Eof()
{
	if(ifs){
		return ifs->eof();
	}
	return true;
}

std::vector<std::string> CFileHandler::FindFiles(std::string pattern)
{
	std::vector<fs::path> found;
	std::string patternPath=pattern;
	if(patternPath.find_last_of('\\')!=string::npos){
		patternPath.erase(patternPath.find_last_of('\\')+1);
	}
	if(patternPath.find_last_of('/')!=string::npos){
		patternPath.erase(patternPath.find_last_of('/')+1);
	}
	if(pattern.find('\\')==string::npos && pattern.find('/')==string::npos)
		patternPath.clear();
	std::string filter=pattern;
	filter.erase(0,patternPath.length());
	fs::path fn(patternPath);
	found = find_files(fn,filter);
	std::vector<std::string> foundstrings;
	for (std::vector<fs::path>::iterator it = found.begin(); it != found.end(); it++)
		foundstrings.push_back(it->string());

	//todo: get a real regex handler
	while(filter.find_last_of('*')!=string::npos)
		filter.erase(filter.find_last_of('*'),1);
//	while(filter.find_last_of('.')!=string::npos)
//		filter.erase(filter.find_last_of('.'),1);
	std::transform(filter.begin(), filter.end(), filter.begin(), (int (*)(int))std::tolower);

	return foundstrings;
}

int CFileHandler::FileSize()
{
	return filesize;
}
