#include <string>
#include <set>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <istream>
#include <iostream>
#include <unistd.h>
#include <math.h>
#include <vector>
#include "CacheManager.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <time.h>
#include "params.h"
#include <set>


using namespace std;

/**
 * this method opens the log file
 */
void Log::log_open()
{

	logFile = fopen(logPath.c_str(), "a");


	if (logFile == NULL)
	{
		perror("logfile");
		exit(EXIT_FAILURE);
	}


	setvbuf(logFile, NULL, _IOLBF, 0);
}

/**
 * Default ctor
 */
Log::Log()
{
	return;
}

/**
 * this method initializes a log file
 */
void Log::init(char* path)
{

	this->logPath = string(path)+"/.filesystem.log";


	log_open();
}

/**
 * this message writes to the log.
 */
void Log::log_msg(bool addTime, const char *format)
{
	string msg;

	if (addTime)
	{

		time_t msgTime = time(NULL);
		stringstream ss;
		ss << msgTime << " " << format << "\n";
		msg = ss.str();
	}
	else
	{
		msg = string(format) + "\n";
	}
	fprintf(logFile, msg.c_str());
}


/**
 * this method a block of a given size.
 */
Block::Block(size_t sizeToSet)
{
	this->blockSize = sizeToSet;

	this->data = (char*)malloc(sizeToSet);
	if (data == NULL)
	{
		cout << "memory allocation crash" << endl;
	}
	for (size_t blockIdx = 0; blockIdx < this->blockSize; blockIdx++)
	{
		data[blockIdx] = '\0';
	}


	this->usedTimes = 0;
	this->relPath = "";

	blockNum = UNINIT_BLOCK;
}

/**
 * dtor
 */
Block::~Block()
{
	free (data);
}

/**
 * this method initializes a block
 */
void Block::initializeBlock(int newBlockNum, string realtivePathToSet, string fullPathToSet)
{

	this->blockNum = newBlockNum;
	this->usedTimes = 0;
	this->fullPath = fullPathToSet;
	this->relPath = realtivePathToSet;


	for (size_t blockIdx = 0; blockIdx < this->blockSize; blockIdx++)
	{
		this->data[blockIdx] = '\0';
	}
}


/**
 * Default constructor.ss
 */
CacheManager::CacheManager()
{

}

/**
 * Destructor.
 */
CacheManager::~CacheManager()
{
	list <Block*>::iterator  blockIter = blocks.begin();
	while (blockIter != blocks.end())
	{
		delete *blockIter;
		blockIter++;
	}
	blocks.clear();
}

/**
 * method to initialize the manager and its fields.
 */
void CacheManager::initializeManager(size_t blockSizeToSet, size_t totalBlocks, float fNew, float fOld)
{


	this->cacheBlockSize = blockSizeToSet;
	this->_fNew = fNew;
	this->_fOld = fOld;

	int numOfNewBlks = floor(totalBlocks * fNew);
	int numOfOldBlks = floor(totalBlocks * fOld);

	this->_fOldMargin = totalBlocks - numOfOldBlks;
	this->_fNewMargin = numOfNewBlks ;

	size_t blockIdx;

	for (blockIdx = 0; blockIdx < totalBlocks; blockIdx++)
	{
		Block* newBlock = new Block(blockSizeToSet);
		this->blocks.push_front(newBlock);
	}
}



/**
 * this method initializes a block in the cache.
 */
int CacheManager::initializeCacheBlock(Block *blockToRet, int blockNum, int fileDesc,
		string relPath, string fullPath)
{


	char* buffer = (char*)aligned_alloc(cacheBlockSize, cacheBlockSize);
//	cout << fileDesc << "  fileDesc" << endl;
//	cout << cacheBlockSize << "  cacheBlockSize" << endl;
//	cout << blockNum << "  blockNum" << endl;
	if(buffer == NULL)
	{
	    return NULL;
	}
	int readReturn = pread(fileDesc, buffer, cacheBlockSize, cacheBlockSize * (blockNum - 1));
//	cout << "pread" << endl;
	if (readReturn <= 0)
	{
//		cout << "readReturn" << endl;
		return readReturn;
	}

	blockToRet->initializeBlock(blockNum, relPath, fullPath);

	memcpy(blockToRet->data, buffer, cacheBlockSize);

	free (buffer);

	return readReturn;
}


/**
 * this method searches the cache for a given block
 */
int CacheManager::findBlock(Block **foundBlock, int blockNum, string fullPath)
{

	list<Block*>::iterator blockIter = this->blocks.begin();


	bool found = false;

	while(blockIter != blocks.end())
	{

		if (((*blockIter)->blockNum == blockNum) and ((*blockIter)->fullPath == fullPath))
		{

			*foundBlock = *blockIter;
			found = true;
		}
		blockIter++;
	}

	if (found)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

/**
 * this method sets the requested block and if it is not in the cache
 * it brings it from the disk.
 */
int CacheManager::blockFromCache(Block **pointerToReturned, int blockNum, int fileDesc,
		string relPath, string fullPath)
{

	if (findBlock(pointerToReturned, blockNum, fullPath) == -1)
	{

		vector<Block*> lfuVector;
		list<Block *>::iterator listIterator = blocks.begin();
		int i = 0;

		while (i < this->_fOldMargin)
		{
			listIterator++;
			i++;
		}

		int minUsedTimes = (*listIterator)->usedTimes;

		listIterator = blocks.begin();


		while (listIterator != blocks.end())
		{
			if (minUsedTimes > (*listIterator)->usedTimes)
			{
				minUsedTimes = (*listIterator)->usedTimes;
			}
			listIterator++;
		}


		listIterator = blocks.begin();


		while (i < this->_fOldMargin)
		{
			listIterator++;
			i++;
		}


		while (listIterator != blocks.end())
		{
			if ((*listIterator)->usedTimes == minUsedTimes)
			{
				lfuVector.push_back(*listIterator);
			}
			listIterator++;
		}

		listIterator = blocks.begin();
		while (i < this->_fOldMargin)
		{
			listIterator++;
			i++;
		}


		int counter = 0;
		while (listIterator != blocks.end() && counter < lfuVector.size())
		{
			if ((*listIterator)->usedTimes == minUsedTimes)
			{
				counter ++;
			}
			listIterator++;
		}


		*pointerToReturned = (*listIterator);

		int initReturn = initializeCacheBlock(*pointerToReturned, blockNum, fileDesc,
				relPath, fullPath);

		if (initReturn > 0)
		{
			(*pointerToReturned)->usedTimes++;

			list<Block *>::iterator blockIter = blocks.begin();


			while (blockIter != blocks.end())
			{
				if ((*pointerToReturned) == (*blockIter))
				{
					blocks.erase(blockIter);

					blocks.push_front(*pointerToReturned);
					break;
				}
				blockIter ++;
			}


		}
		return initReturn;
	}
	else
	{



		(*pointerToReturned)->usedTimes++;

		list<Block *>::iterator blockIter = blocks.begin();


		while (blockIter != blocks.end())
		{
			if ((*pointerToReturned) == (*blockIter))
			{
				blocks.erase(blockIter);

				blocks.push_front(*pointerToReturned);
				break;
			}
			blockIter ++;
		}


	}
	return 0;
}

/**
 * this method reads a block to a given buffer
 */
int CacheManager::readBlockToBuffer(char* buffer, int blockNum, int fileDesc,
		string relPath, string fullPath, int readSoFar, int offset, size_t blockSize)
{

	Block *retBlock;

	int blockGet = blockFromCache(&retBlock, blockNum, fileDesc,
			relPath, fullPath);

	if (blockGet < 0)
	{
		return blockGet;
	}

	if ((unsigned int)blockGet > blockSize)
	{
		blockGet = blockSize;
	}

	size_t copySize = blockGet;

	memcpy(buffer + readSoFar, retBlock->data + offset, copySize);

	return blockGet;
}

/**
 * this method renames a file in the cache
 */
void CacheManager::renameCacheFile(string oldRelPath, string oldFullPath,
		string newRelPath, string newFullPath)
{

	list<Block *>::iterator blockIter = blocks.begin();
	while (blockIter != blocks.end())
	{
		if ((*blockIter)->relPath == oldRelPath)
		{

			if ((*blockIter)->relPath.find(oldRelPath) != string::npos)
			{
				size_t oldSize;
				oldSize = oldRelPath.size();
				(*blockIter)->relPath.erase(0, oldSize);
				(*blockIter)->relPath = newRelPath + (*blockIter)->relPath;
				oldSize = oldFullPath.size();
				(*blockIter)->fullPath.erase(0, oldSize);
				(*blockIter)->fullPath = newFullPath + (*blockIter)->fullPath;
			}
		}
		blockIter++;
	}
}

/**
 * this method updates the log file.
 */
void CacheManager::updateLog(Log &destLog)
{

	list<Block *>::reverse_iterator blockIter = blocks.rbegin();

	while(blockIter != blocks.rend())
	{
		stringstream ss;
		ss << (*blockIter)->relPath.c_str() << " " << (*blockIter)->blockNum
		<< " " << (*blockIter)->usedTimes;
		string s =  ss.str();
		const char* msg = s.c_str();
		destLog.log_msg(false, msg);

		blockIter++;
	}
}
