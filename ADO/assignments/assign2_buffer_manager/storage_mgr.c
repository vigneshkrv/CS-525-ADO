#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include "storage_mgr.h"
typedef int RC;

#define RC_ERROR -1

FILE *newPage;

extern void initStorageManager (void) 
{
	newPage = NULL;
}

extern RC createPageFile (char *fileName) 
{
	newPage = fopen(fileName, "w+");
	if(newPage == NULL) 
	{
		return RC_FILE_NOT_FOUND;
	} 
	else 
	{
		SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
		if(fwrite(emptyPage, sizeof(char), PAGE_SIZE,newPage) < PAGE_SIZE)
		{
			return RC_ERROR;
		}
		else
		{
			printf("write succeeded \n");
		}
		fclose(newPage);
		free(emptyPage);
		return RC_OK;
	}
}

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
	newPage = fopen(fileName, "r");
	if(newPage == NULL) 
	{
		return RC_FILE_NOT_FOUND;
	} 
	else 
	{ 
		fHandle->fileName = fileName;
		fHandle->curPagePos = 0;
		struct stat fileInfo;
		if(fstat(fileno(newPage), &fileInfo) < 0)  
		{  
			return RC_ERROR;
		}
		fHandle->totalNumPages = fileInfo.st_size/ PAGE_SIZE;
		fclose(newPage);
		return RC_OK;
	}
}

extern RC closePageFile (SM_FileHandle *fHandle) 
{
	if(newPage != NULL)
	{
		newPage = NULL;	
	}
	return RC_OK; 
}


extern RC destroyPageFile (char *fileName) 
{
	newPage = fopen(fileName, "r");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND; 
	}
	remove(fileName);
	return RC_OK;
}

extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (pageNum > fHandle->totalNumPages || pageNum < 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}
        newPage = fopen(fHandle->fileName, "r");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	int isSeekSuccess = fseek(newPage, (pageNum * PAGE_SIZE), SEEK_SET);
	if(isSeekSuccess == 0) 
	{
		if(fread(memPage, sizeof(char), PAGE_SIZE, newPage) < PAGE_SIZE)
		{
			return RC_ERROR;
		}
		
	} 
	else 
	{
		return RC_READ_NON_EXISTING_PAGE; 
	}
	fHandle->curPagePos = ftell(newPage); 
	fclose(newPage);
	return RC_OK;
}

extern int getBlockPos (SM_FileHandle *fHandle) 
{
	return fHandle->curPagePos;
}

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
	newPage = fopen(fHandle->fileName, "r");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	for(int i = 0; i < PAGE_SIZE; i++) 
	{
		char c = fgetc(newPage);
		if(feof(newPage))
		{
			break;
		}
		else
			memPage[i] = c;
	}
	fHandle->curPagePos = ftell(newPage); 
	fclose(newPage);
	return RC_OK;
}

extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
 {
	if(fHandle->curPagePos <= PAGE_SIZE) 
	{
		return RC_READ_NON_EXISTING_PAGE;	
	} 
	else 
	{
		int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
		int startPosition = (PAGE_SIZE * (currentPageNumber - 2));
		newPage = fopen(fHandle->fileName, "r");
		if(newPage == NULL)
		{
			return RC_FILE_NOT_FOUND;
		}
		fseek(newPage, startPosition, SEEK_SET);
		for(int i = 0; i < PAGE_SIZE; i++) 
		{
			memPage[i] = fgetc(newPage);
		}
		fHandle->curPagePos = ftell(newPage);
		fclose(newPage);
		return RC_OK;
	}
}

extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
	int startPosition = (PAGE_SIZE * (currentPageNumber - 2));
	newPage = fopen(fHandle->fileName, "r");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	fseek(newPage, startPosition, SEEK_SET);
	for(int i = 0; i < PAGE_SIZE; i++) 
	{
		char c = fgetc(newPage);		
		if(feof(newPage))
		{
			break;
		}		
		memPage[i] = c;
	}
	fHandle->curPagePos = ftell(newPage); 
	fclose(newPage);
	return RC_OK;		
}

extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if(fHandle->curPagePos == PAGE_SIZE) 
	{
		return RC_READ_NON_EXISTING_PAGE;	
	} 
	else 
	{
		int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
		int startPosition = (PAGE_SIZE * (currentPageNumber - 2));
		newPage = fopen(fHandle->fileName, "r");
		if(newPage == NULL)
		{
			return RC_FILE_NOT_FOUND;
		}
		fseek(newPage, startPosition, SEEK_SET);
		for(int i = 0; i < PAGE_SIZE; i++) 
		{
			char c = fgetc(newPage);		
			if(feof(newPage))
			{
				break;
			}
			memPage[i] = c;
		}
		fHandle->curPagePos = ftell(newPage); 
		fclose(newPage);
		return RC_OK;
	}
}

extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	newPage = fopen(fHandle->fileName, "r");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	int startPosition = (fHandle->totalNumPages - 1) * PAGE_SIZE;
	fseek(newPage, startPosition, SEEK_SET);
	for(int i = 0; i < PAGE_SIZE; i++) 
	{
		char c = fgetc(newPage);		
		if(feof(newPage))
		{
			break;
		}
		memPage[i] = c;
	}
	fHandle->curPagePos = ftell(newPage); 
	fclose(newPage);
	return RC_OK;	
}

extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) 
{	
	if (pageNum > fHandle->totalNumPages || pageNum < 0)
	{
		return RC_WRITE_FAILED;
	}
        newPage = fopen(fHandle->fileName, "r+");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	int startPosition = pageNum * PAGE_SIZE;
	if(pageNum == 0) 
	{ 
		fseek(newPage, startPosition, SEEK_SET);	
		int i;
		for(i = 0; i < PAGE_SIZE; i++) 
		{
			
			if(feof(newPage)) 
			{
				appendEmptyBlock(fHandle);
			}
			fputc(memPage[i], newPage);
		}
		fHandle->curPagePos = ftell(newPage); 
		fclose(newPage);	
	} 
	else 
	{	
		fHandle->curPagePos = startPosition;
		fclose(newPage);
		writeCurrentBlock(fHandle, memPage);
	}
	return RC_OK;
}

extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
{
	newPage = fopen(fHandle->fileName, "r+");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	appendEmptyBlock(fHandle);
	fseek(newPage, fHandle->curPagePos, SEEK_SET);
	fwrite(memPage, sizeof(char), strlen(memPage), newPage);
	fHandle->curPagePos = ftell(newPage);
	fclose(newPage);
	return RC_OK;
}


extern RC appendEmptyBlock (SM_FileHandle *fHandle) 
{
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
	int isSeekSuccess = fseek(newPage, 0, SEEK_END);
	if( isSeekSuccess == 0 ) 
	{
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, newPage);
	} 
	else 
	{
		free(emptyBlock);
		return RC_WRITE_FAILED;
	}
	free(emptyBlock);
	fHandle->totalNumPages++;
	return RC_OK;
}

extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
 {
	newPage = fopen(fHandle->fileName, "a");
	if(newPage == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	while(numberOfPages > fHandle->totalNumPages)
	{
		appendEmptyBlock(fHandle);
	}
	fclose(newPage);
	return RC_OK;
}
