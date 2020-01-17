#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include "dirent.h"
#include "log.h"

#define LOG_BUFFER_SIZE (64*1024)
#define MAX_LOGFILE_SIZE (5*1024*1024)
#define MAX_LOGFILE_NUM (5)
/* #define FilePath "./log/" */
#define LOG_PATH "/home/ubuntu/log/"
#define FileNameLen 64
#define PUB_LOG_MT

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *gpLogFile = NULL;
char FilePath[256] = {'/', 'w','w','w','/','l', 'o', 'g', '/', 0};

/* Functions Used Internally */
static int log_msg(const char *file, int line, const char *function, pub_log_level level, const char *msg);
static const char *get_level_str(pub_log_level level);

/*
@获取路径下文件个数
*/
static int get_file_num(const char *file)
{
	DIR *dir = NULL;  
	int num = 0;
    struct dirent *entry;  
    if((dir = opendir(file))==NULL){
        fprintf(stderr,"opendir(%s) failed!\n", file);  
        return -1;  
    }
    else  
    {  
        while(entry=readdir(dir))  
        {         
			if(entry->d_type == 8)
			{
				num++;
			}		 
        }  
        closedir(dir);    
    }  
	return num;
}

/*
@获取日志文件大小
*/
static unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;	

	if(strlen(path)==0)
		return -1;
	
	struct stat statbuff;
	if(stat(path, &statbuff) < 0)
	{
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
	}
	return filesize;
}

/*
@获取最老文件
*/
static int obtain_oldest_file(char *filename)
{
	DIR *dir = NULL; 
	int dirn=0,dirIndex=0;
	struct dirent **namelist = NULL;
	char cmd[32];

	memset(filename,0,FileNameLen);

	//路径不存在则返回空文件名
	if((dir = opendir(FilePath))==NULL)  
	{  
		return -1;
	}
	
	//排序查找
	dirn=scandir(FilePath, &namelist, 0, alphasort);

	for(dirIndex=0;dirIndex<dirn;dirIndex++)
	{
		if(namelist[dirIndex]->d_type == 8)
		{
			//找到一个刷新文件名
			sprintf(filename,"%s%s",FilePath,namelist[dirIndex]->d_name);
			break;
		}	
	}

	for(dirIndex=0;dirIndex<dirn;dirIndex++)
	{
		free(namelist[dirIndex]);
	}
	
	free(namelist);
	closedir(dir);	  

	return 0;
}


/*
@获取最新的文件
*/
static int obtain_latest_file(char *filename)
{
	DIR *dir = NULL; 
	int dirn=0,dirIndex=0;
	struct dirent **namelist = NULL;
	char cmd[32];

	memset(filename,0,FileNameLen);

	//路径不存在则返回空文件名
	if((dir = opendir(FilePath))==NULL)  
	{  
		//创建日志路径
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"mkdir -p %s",FilePath);
		system(cmd);
		return -1;
	}
	
	//排序查找
	dirn=scandir(FilePath, &namelist, 0, alphasort);

	for(dirIndex=0;dirIndex<dirn;dirIndex++)
	{
		if(namelist[dirIndex]->d_type == 8)
		{
			//找到一个刷新文件名
			sprintf(filename,"%s%s",FilePath,namelist[dirIndex]->d_name);			
		}	
	}

	for(dirIndex=0;dirIndex<dirn;dirIndex++)
	{
		free(namelist[dirIndex]);
	}
	
	free(namelist);
	closedir(dir);	  

	return 0;
}

/*
@删除日志
*/
static void delete_file(char *filePath)
{
	if(filePath==NULL)
		return;
	
	remove(filePath);
}

/*
@创建文件
*/
static int create_file()
{
	struct tm p;
    struct timeval now;
	time_t timep;
	FILE *fp = NULL;
	char fileName[FileNameLen];
	
	timep=time(NULL);
	localtime_r(&timep,&p);
    gettimeofday(&now, NULL);
	//如果当前文件已经达到个数上限，则删除最老文件,然后创建新文件
	if(get_file_num(FilePath)>= MAX_LOGFILE_NUM)
	{
		obtain_oldest_file(fileName);
		delete_file(fileName);
	}

	//
	sprintf(fileName, "%s%04d-%02d-%02d-%02d-%02d-%02d-%03d.log",FilePath,
            p.tm_year + 1900, p.tm_mon + 1, p.tm_mday,
            p.tm_hour, p.tm_min, p.tm_sec, (int)now.tv_usec/1000);
	fp = fopen(fileName, "w");
	if (fp == NULL)
	{
		return -1;
	}
	fflush(fp);
	fclose(fp);
	return 0;
}

static int log_msg(const char *file, int line, const char *function, pub_log_level level, const char *msg)
{
	FILE *fp = stderr;
	char time_str[64] = {0};
	const char *level_str;
	struct tm tm;
	time_t timevalue = 0;

	pthread_mutex_lock(&mutex);
	
	if (level == PUB_LOG_LEVEL_DEBUG) {
		fp = stdout;
	}
	else
	{
        if (gpLogFile != NULL){
            /* if file size is two big, close it first */
            long size = ftell(gpLogFile);
            if (size > MAX_LOGFILE_SIZE){
                fclose(gpLogFile);
                gpLogFile = NULL;
            }
        }
        if (gpLogFile == NULL){
            char fileName[FileNameLen] = {0};
            //获取最新的日志文件
            obtain_latest_file(fileName);
            
            //获取不到正常的文件大小
            if((get_file_size(fileName)>MAX_LOGFILE_SIZE)){
                create_file();
            }
            obtain_latest_file(fileName);
            gpLogFile=fopen(fileName, "a");
            if(gpLogFile==NULL){
                pthread_mutex_unlock(&mutex);
                return -1;
            }
        }
        fp = gpLogFile;
	}

	timevalue=time(NULL);
	localtime_r(&timevalue,&tm);
    struct timeval now;
    gettimeofday(&now, NULL);
	sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d-%03d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)now.tv_usec/1000);

	level_str = get_level_str(level);

#ifdef PUB_LOG_MT
	fprintf(fp, "[%s] [%s] [%s(%d): %s] %s\n",
		time_str,
		level_str,
		file, line, function,
		msg);
#else /* PUB_LOG_MT */
	fprintf(fp, "[%s] [%d] [%s] [%s(%d): %s] %s\n",
		time_str, (int)getpid(), 
		level_str,
		file, line, func,
		msg);
#endif /* PUB_LOG_MT */

	fflush(fp);
	pthread_mutex_unlock(&mutex);

	return 0;
}

static
const char *get_level_str(pub_log_level level)
{
	const char *p = "UNKNOWN";
	switch (level) {
		case PUB_LOG_LEVEL_INFO:
			p = "INFO";
			break;
		case PUB_LOG_LEVEL_WARNING:
			p = "WARNING";
			break;
		case PUB_LOG_LEVEL_ERROR:
			p = "ERROR";
			break;
		case PUB_LOG_LEVEL_DEBUG:
			p = "DEBUG";
			break;
		default:
			p = "UNKNOWN";
			break;
	}
	return p;
}

void _pub_module(const char *path)
{
    char command[1024] = {0};
    snprintf(FilePath, 256, "%s%s/", LOG_PATH, path);
    snprintf(command, 1024, "mkdir -p %s", FilePath);
    system(command);
}

int _pub_log(const char *file, int line, const char *function, pub_log_level level, const char *fmt, ...)
{
	char msg[LOG_BUFFER_SIZE];
	va_list args;

	memset(msg, 0, sizeof(msg));
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	
	log_msg(file, line, function, level, msg);

	return 0;
}

void _pub_assert(const char *file, int line, const char *function, int expr, const char *expr_str)
{
	if (!expr) {
		_pub_log(file, line, function, PUB_LOG_LEVEL_DEBUG, "Assertion failed: %s.", expr_str);
		exit(1);
	}
}

