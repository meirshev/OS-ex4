
#ifndef CACHEMANAGER_H
#define	CACHEMANAGER_H

#include <string>
#include <set>
#include <istream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <list>
#include <time.h>
#include <cstdlib>
#include <string>

using namespace std;


#ifndef LOG_H
#define	LOG_H

#define log_struct(st, field, format, typecast) \
  log_msg("    " #field " = " #format "\n", typecast st->field)

//a struct of the log
struct Log
{
	FILE *logFile;
	string logPath;
	Log();
	void init(char *path);
	void log_open();
	void log_msg(bool time, const char *format);
};

#endif	/* LOG_H */



#ifndef BLOCK_H
#define	BLOCK_H

#define UNINIT_BLOCK -1

//a struct of the block
class Block
{
public:
	size_t blockSize;
	int blockNum;
	string relPath;
	string fullPath;
	char* data;
	int usedTimes;
	Block(size_t block_size);
	~Block();
	void initializeBlock(int newBlockNum, string realtivePathToSet, string fullPathToSet);
};

#endif	/* BLOCK_H */


//a struct of the CacheManager
struct CacheManager
{
	/**
	 Comparator (implementing only operator()) for the used times multi set
	 */
	struct byUsedComparator
	{
		bool operator () (Block* first, Block* second);
	};
	
	// a set of blocks ordered by their used value, so that the lfu block is the last one
	//multiset<Block*, byUsedComparator> blocks;

	list<Block*> blocks;


	size_t cacheBlockSize;
	
	CacheManager();
	~CacheManager();
	void initializeManager(size_t blockSizeToSet, size_t totalBlocks, float fNew, float fOld);
	int initializeCacheBlock(Block *blockToRet, int blockNum, int fileDesc,
		string relPath, string fullPath);
	int findBlock(Block **foundBlock, int blockNum, string relPath);
	int blockFromCache(Block **pointerToReturned, int blockNum, int fileDesc, 
		string relPath, string fullPath);
	int readBlockToBuffer(char* buffer, int blockNum, int fileDesc, 
		string relPath, string fullPath, int readSoFar, int offset, size_t blockSize);

	float _fOld;
	float _fNew;
	int _fOldMargin;
	int _fNewMargin;

	void renameCacheFile(string oldRelPath, string oldFullPath,
			string newRelPath, string newFullPath);
	void updateLog(Log &destLog);
};

#endif	/* CACHEMANAGER_H */

