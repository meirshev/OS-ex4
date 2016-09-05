#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <iostream>
#include <string>
#include <fuse.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

#define _FILE_OFFSET_BITS  64

#include "params.h"
#include "Log.h"
#include "CacheManager.h"
#include <string>
#include <cstring>

using namespace std;

struct fuse_operations caching_oper;

string caching_fullpath(string partialPath)
{
	if (partialPath == "/.filesystem.log")
	{
		return "";
	}

	string fullPath = BB_STATE->rootdir + partialPath;

	return fullPath;
}


int caching_getattr(const char *path, struct stat *statbuf)
{

	BB_STATE->cacheLog.log_msg(true, "getattr");
	
	int retstat = 0;
	
	string fullPath = caching_fullpath(path);
	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullPath == "")
	{
		return -ENOENT;
	}

	retstat = lstat(fullPath.c_str(), statbuf);
	if (retstat != 0)
	{
		return -errno;
	}
	return retstat;
}



int caching_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "fgetattr");
	

	int retstat = 0;
	string fullPath = caching_fullpath(path);
	

	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullPath == "")
	{
		return -ENOENT;
	}
	

	if (!strcmp(path, "/"))
	{
		return caching_getattr(path, statbuf);
	}

	retstat = fstat(fi->fh, statbuf);
	if (retstat < 0)
	{
		return -errno;
	}

	return retstat;
}



int caching_access(const char *path, int mask)
{

	BB_STATE->cacheLog.log_msg(true, "access");
	
	string fullPath=caching_fullpath(path);
	

	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullPath == "")
	{
		return -ENOENT;
	}

	int retstat = 0;


	retstat = access(fullPath.c_str(), mask);

	if (retstat < 0)
	{
		return -errno;
	}
	
	return retstat;
}


int caching_open(const char *path, struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "open");
	
	int retstat = 0;
	int fd;
	
	string fullPath = caching_fullpath(path);
	

	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullPath == "")
	{
		return -ENOENT;
	}
    fi->direct_io = 1;

//	cout << "before open" << endl;
//	cout << fullPath << endl;
	//(O_RDONLY|O_DIRECT|O_SYNC)
	fd = open(fullPath.c_str(), O_RDONLY | O_DIRECT | O_SYNC);
	if (fd < 0)
	{
	    cout << strerror(errno) << endl;
		retstat = -errno;
	}
//	cout << "after open" << endl;

	fi->fh = fd;
	return retstat;
}


int caching_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "read");

	string fullPath = caching_fullpath(path);

	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	if (fullPath == "")
	{
		return -ENOENT;
	}

	int blockSize = BB_STATE->blockSize;
	int readCounter = 0;
	int lastBlock = (offset + size) / blockSize + 1;
	int firstBlock = offset / blockSize + 1;

	int lastOffset = offset % blockSize;
	int fullRead;
	if (size < (unsigned int)blockSize - lastOffset)
	{
		fullRead = size;
	}
	else
	{
		fullRead = blockSize - lastOffset;
	}


	readCounter += BB_STATE->cache.readBlockToBuffer(buf, firstBlock, fi->fh,
			path, fullPath, 0, lastOffset, fullRead);

	if (readCounter < 0)
	{
		return -errno;
	}


	size_t remainingRead = size - readCounter;
	size_t currRead, partialRead;

	if (remainingRead > 0)
	{
		int blockIdx = firstBlock + 1;
		int partialReadSuccess;
		for (; blockIdx <= lastBlock; blockIdx++)
		{

			if (remainingRead >= (unsigned int)blockSize)
			{
				currRead = blockSize;
			}
			else
			{
				currRead = remainingRead;
			}


			partialReadSuccess = BB_STATE->cache.readBlockToBuffer(buf, blockIdx, fi->fh,
					path, fullPath, readCounter, 0, currRead);


			if (partialReadSuccess < 0)
			{
				return -errno;
			}
			partialRead = (unsigned int)partialReadSuccess;


			remainingRead -= partialRead;
			readCounter += partialRead;
		}
	}

	return readCounter;
}


int caching_flush(const char *path, struct fuse_file_info *fi)
{
	
	BB_STATE->cacheLog.log_msg(true, "flush");
	
    return 0;
}


int caching_release(const char *path, struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "release");
	

	string fullPath = caching_fullpath(path);
	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullPath == "")
	{
		return -ENOENT;
	}
	int retstat = 0;


	retstat = close(fi->fh);

	return retstat;
}

int caching_opendir(const char *path, struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "opendir");
	

	string fullPath = caching_fullpath(path);
	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullPath == "")
	{
		return -ENOENT;
	}

	DIR *dp;
	dp = opendir(fullPath.c_str());

	if (dp == NULL)
	{
		return -errno;
	}


	fi->fh = (intptr_t) dp;

	return 0;
}


int caching_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "readdir");
	string logFile = ".filesystem.log";

	int retstat = 0;
	DIR *dp;
	struct dirent *de;
	dp = (DIR *) (uintptr_t) fi->fh;
	de = readdir(dp);
	if (de == 0)
	{
		retstat = -errno;
		return retstat;
	}
	do {

		if ((strcmp(logFile.c_str(), de->d_name)) != 0)
		{

			if ((filler(buf, de->d_name, NULL, 0) != 0)) {
				return -ENOMEM;
			}
		}

	} while ((de = readdir(dp)) != NULL);
	
	return retstat;
	
}


int caching_releasedir(const char *path, struct fuse_file_info *fi)
{

	BB_STATE->cacheLog.log_msg(true, "releasedir");
	

	string fullPath = caching_fullpath(path);
	if (fullPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}


	closedir((DIR *) (uintptr_t) fi->fh);

	return 0;
}

int caching_rename(const char *path, const char *newpath)
{

	BB_STATE->cacheLog.log_msg(true, "rename");
	int retstat = 0;
	

	string fullOldPath = caching_fullpath(path);
	if (fullOldPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullOldPath == "")
	{
		return -ENOENT;
	}
	

	string fullNewPath = caching_fullpath(newpath);
	if (fullNewPath.size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (fullNewPath == "")
	{
		return -ENOENT;
	}


	retstat = rename(fullOldPath.c_str(), fullNewPath.c_str());
	if (retstat < 0)
	{
		retstat = -errno;
	}
	

	BB_STATE->cache.renameCacheFile(path, fullOldPath, newpath, fullNewPath);

	return retstat;
}

void *caching_init(struct fuse_conn_info *conn)
{
	return BB_STATE;
}

void caching_destroy(void *userdata)
{
	BB_STATE->cacheLog.log_msg(true, "destroy");
}


int caching_ioctl (const char *, int cmd, void *arg,
		struct fuse_file_info *, unsigned int flags, void *data)
{

	BB_STATE->cacheLog.log_msg(true, "ioctl");
	BB_STATE->cache.updateLog(BB_STATE->cacheLog);
	return 0;
}

void init_caching_oper()
{

	caching_oper.getattr = caching_getattr;
	caching_oper.access = caching_access;
	caching_oper.open = caching_open;
	caching_oper.read = caching_read;
	caching_oper.flush = caching_flush;
	caching_oper.release = caching_release;
	caching_oper.opendir = caching_opendir;
	caching_oper.readdir = caching_readdir;
	caching_oper.releasedir = caching_releasedir;
	caching_oper.rename = caching_rename;
	caching_oper.init = caching_init;
	caching_oper.destroy = caching_destroy;
	caching_oper.ioctl = caching_ioctl;
	caching_oper.fgetattr = caching_fgetattr;


	caching_oper.readlink = NULL;
	caching_oper.getdir = NULL;
	caching_oper.mknod = NULL;
	caching_oper.mkdir = NULL;
	caching_oper.unlink = NULL;
	caching_oper.rmdir = NULL;
	caching_oper.symlink = NULL;
	caching_oper.link = NULL;
	caching_oper.chmod = NULL;
	caching_oper.chown = NULL;
	caching_oper.truncate = NULL;
	caching_oper.utime = NULL;
	caching_oper.write = NULL;
	caching_oper.statfs = NULL;
	caching_oper.fsync = NULL;
	caching_oper.setxattr = NULL;
	caching_oper.getxattr = NULL;
	caching_oper.listxattr = NULL;
	caching_oper.removexattr = NULL;
	caching_oper.fsyncdir = NULL;
	caching_oper.create = NULL;
	caching_oper.ftruncate = NULL;
}

//basic main. You need to complete it.
int main(int argc, char* argv[])
{





	if ((argc < 5))
	{
		cout << "usage: CachingFileSystem rootdir mountdir numberOfBlocks blockSize fNew fOld" << endl;
		exit(1);
	}

	struct stat checkStat;
	stat("/tmp", &checkStat);
	int blockSize = checkStat.st_blksize;


	struct bb_state * cachingManager;
	int totalBlocks = atoi(argv[3]);

	float fOld = atof(argv[4]);

	float fNew = atof(argv[5]);

	if (fOld + fNew > 1 || fOld + fNew < 0 || fOld < 0 || fNew < 0)
	{
		cout << "CachingFileSystem rootdir mountdir numberOfBlocks fOld fNew" << endl;
		exit(1);
	}

	cachingManager = new bb_state;

	cachingManager->cache.initializeManager(blockSize, totalBlocks, fNew, fOld);
	cachingManager->blockSize = blockSize;
	cachingManager->cacheLog.init(argv[1]);
	cachingManager->rootdir = realpath(argv[1], NULL);

	if (string(argv[2]).size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	if (string(cachingManager->rootdir).size() > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	init_caching_oper();

	argv[1] = argv[2];
	for (int i = 2; i< (argc - 1); i++)
	{
		argv[i] = NULL;
	}


	argv[2] = (char*) "-s";
	argv[3] = (char*) "-f";

	argc = 4;



	int fuse_stat = fuse_main(argc, argv, &caching_oper, cachingManager);

	return fuse_stat;

}
