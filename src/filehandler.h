#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <vector>
#include <string>

class CFileHandler
{

public:
	CFileHandler(const char* filename);
	CFileHandler(std::string filename);
	~CFileHandler(void);
	
	bool FileExists();

	void Read(void* buf,int length);
	void Seek(int pos);
	int Peek();
	bool Eof();
	int FileSize();

	static std::vector<std::string> FindFiles(std::string pattern);
private:
	void Init(const char* filename);
	int filesize;
	std::ifstream* ifs;
};

#endif // __FILE_HANDLER_H__
