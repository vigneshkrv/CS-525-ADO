
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"

//The Page Frame structure which holds the actual page frame in the Buffer Manager.
typedef struct Frame {
        SM_PageHandle content;
        int dirty;
        int pinStatus;
        int freeStat;
        int fixCount;
        PageNumber PageNumber;
        void *qPointer ;
} Frame;

//The Queue Data structure which simulates a queue for the FIFO and LRU Page Repalcement Algrithm
typedef struct Queue {
        int count;
        int position;
        Frame *framePointer;
        int pNo;
} Queue;

// This varible indicates the current size of the buffer manager
int bufferSize= 0;

//This variable indicates the maximum size of the buffer Manager.
int maxBufferSize;

//This variable indicates the maximum size of the Queue.
int queueSize;

//This variable indicates if the buffer is full or not.
int isBufferFull= 0;

//This variable indicates if the current queue size.
int currentQueueSize;

//This varaible keeps track of the number of writes to the disk.
int writeCount = 0;

//This variable keeps track of the number of reads from the disk.
int readCount = 0;

//This function initiates the buffer manager with the necessary parameters.
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData){


        // Initializing all the variables.
        queueSize = maxBufferSize = numPages;
        currentQueueSize = 0;
        writeCount= 0;
        readCount =0;

        //setting the values in the buffer manager variable.
        bm->pageFile = (char *)pageFileName;
        bm->numPages = numPages;
        bm->strategy = strategy;


        int i;

        //Allocating space for the PageFrame and the Queue.
        Frame *pageFrame = malloc(sizeof(Frame) * numPages);
        Queue *queueFrame = malloc(sizeof(Queue) * numPages);

        //Initializing all the values in the PageFrame and Queue.
        for(i=0; i<numPages; i++) {
                pageFrame[i].content = (SM_PageHandle)malloc(PAGE_SIZE);
                pageFrame[i].dirty = 0;
                pageFrame[i].pinStatus = 0;
                pageFrame[i].freeStat = 0;
                pageFrame[i].PageNumber = -1;
                pageFrame[i].fixCount = 0;

                queueFrame[i].count = 0;
                queueFrame[i].position = 0;
                queueFrame[i].framePointer = NULL;
                queueFrame[i].pNo = -1;
        }

        //The firs PageFrame always contains the pointer to the Queue.
        pageFrame[0].qPointer = queueFrame;

        //The pageFrame is stored in the bufferpool variable.
        bm->mgmtData = pageFrame;
        return RC_OK;
}

//This function returns the PageFrame if the pagenumber and the bufferpool variable are given.
Frame * returnPagePointer(BM_BufferPool *const bm,BM_PageHandle *const page){
        Frame *pageFrames = (Frame *)bm->mgmtData;
        for(int i =0; i<maxBufferSize; i++) {
                if(page->pageNum == pageFrames[i].PageNumber) {
                        return &pageFrames[i];
                        break;
                }
        }
}

//This function returns the maximum position value in the queue, used for LRU.
int maxQueue(Queue *q){
        int max = -1;
        for(int i = 0; i < currentQueueSize; i++) {
                if(q[i].position > max) {
                        max = q[i].position;
                }
        }
        return max;
}

//The LRU Function is a void function which the maintains the queue and performs LRU when the buffer size is full.
void LRU(BM_BufferPool *const bm, BM_PageHandle *const page,int pageNum){
        //Get the PageFrame pointer and the Queue pointer.
        Frame *pageFrames = (Frame *)bm->mgmtData;
        Queue *queue = (Queue *)pageFrames[0].qPointer;

        //if the pageFrame is already in buffer make its position 1 so that we know it was the last accessed page.
        //increment the rest.
        for(int i =0; i <currentQueueSize; i++) {
                if(pageFrames[i].PageNumber == pageNum) {

                        queue[i].position = 1;

                        pageFrames[i].fixCount++;
                        for(int j=0; j<currentQueueSize; j++) {
                                if(j != i)
                                        queue[j].position++;
                        }

                        //update the page handler.
                        page->data  = pageFrames[i].content;
                        page->pageNum = pageFrames[i].PageNumber;
                        return;
                }
        }


        //if the buffer is not full insert pages normally.
        if(currentQueueSize < bm->numPages) {
                //insert the first page in the queue.
                if(currentQueueSize == 0) {

                        queue[0].position = 1;
                        queue[0].framePointer = &pageFrames[0];

                        currentQueueSize++;

                        return;
                }
                else{
                        //find the place in the queue which is free and insert the page.
                        for(int i =0; i<maxBufferSize; i++) {
                                //finding which pageFrame is null.
                                if(queue[i].framePointer == NULL) {
                                        queue[i].position = 1;
                                        for(int j=0; j<maxBufferSize; j++) {
                                                if(j != i)
                                                        queue[j].position +=1;
                                        }
                                        queue[i].framePointer = returnPagePointer(bm,page);

                                        currentQueueSize++;
                                        return;
                                }

                        }
                }

        }
        //If the queue is full we need to do LRU.
        else if(currentQueueSize == queueSize ) {

                for(int i =0; i<currentQueueSize; i++) {

                        // The queue frame with the maximum position is the least recently used, so we must replace that.
                        if(queue[i].position == maxQueue(queue)) {
                                //check if no other client is accessing the page.
                                if(pageFrames[i].fixCount == 0) {
                                        queue[i].position = 1;
                                        for(int j=0; j<currentQueueSize; j++) {
                                                if(j != i)
                                                        queue[j].position++;

                                        }

                                        SM_FileHandle fh;
                                        //if the page is dirty write it back to the disk.
                                        if(pageFrames[i].dirty ==1) {
                                                openPageFile(bm->pageFile, &fh);
                                                ensureCapacity(pageFrames[i].PageNumber,&fh);
                                                writeBlock(pageFrames[i].PageNumber, &fh, pageFrames[i].content);
                                                writeCount++;
                                        }

                                        // pin the page and store the pagenumber.
                                        pageFrames[i].pinStatus = 1;
                                        pageFrames[i].PageNumber = pageNum;
                                        pageFrames[i].freeStat = 1;
                                        pageFrames[i].dirty = 0;
                                        pageFrames[i].fixCount = 0;

                                        //read the data from the disk and store it in the page handler.
                                        openPageFile(bm->pageFile,&fh);
                                        ensureCapacity(pageFrames[i].PageNumber, &fh);
                                        readBlock(pageFrames[i].PageNumber,&fh,pageFrames[i].content);

                                        //increment the read count
                                        readCount++;

                                        //update the page handler.
                                        page->data  = pageFrames[i].content;
                                        page->pageNum = pageFrames[i].PageNumber;


                                        return;
                                }
                        }
                }
                // if the page is in use by other client find the next largest page and check if we can replace that
                for(int i =0; i<currentQueueSize; i++) {
                        int temp = maxQueue(queue)-1;
                        if(queue[i].position == temp && pageFrames[i].fixCount == 0) {
                                queue[i].position = 1;
                                for(int j=0; j<currentQueueSize; j++) {
                                        if(j != i)
                                                queue[j].position++;

                                }

                                SM_FileHandle fh;
                                if(pageFrames[i].dirty ==1) {
                                        openPageFile(bm->pageFile, &fh);
                                        ensureCapacity(pageFrames[i].PageNumber,&fh);
                                        writeBlock(pageFrames[i].PageNumber, &fh, pageFrames[i].content);
                                        writeCount++;
                                }

                                // pin the page and store the pagenumber.
                                pageFrames[i].pinStatus = 1;
                                pageFrames[i].PageNumber = pageNum;
                                pageFrames[i].freeStat = 1;
                                pageFrames[i].dirty = 0;
                                pageFrames[i].fixCount = 0;

                                //read the data from the disk and store it in the page handler.
                                openPageFile(bm->pageFile,&fh);
                                ensureCapacity(pageFrames[i].PageNumber, &fh);
                                readBlock(pageFrames[i].PageNumber,&fh,pageFrames[i].content);

                                //increment the read count
                                readCount++;

                                //update the page handler.
                                page->data  = pageFrames[i].content;
                                page->pageNum = pageFrames[i].PageNumber;


                                return;
                        }
                }
        }
}

//The FIFO Function is a void function which the maintains the queue and performs FIFO when the buffer size is full.
void FIFO(BM_BufferPool *const bm, BM_PageHandle *const page,int pageNum){
        //Get the PageFrame pointer and the Queue pointer.
        Frame *pageFrames = (Frame *)bm->mgmtData;
        Queue *queue = (Queue *)pageFrames[0].qPointer;

        //if the buffer is not full insert pages normally.
        if(currentQueueSize < bm->numPages) {
                //insert the first page in the queue.
                if(currentQueueSize == 0) {

                        queue[0].position = 1;
                        queue[0].framePointer = &pageFrames[0];
                        queue[0].pNo = pageNum;
                        currentQueueSize++;

                        return;
                }
                else{
                        //find the place in the queue which is free and insert the page.
                        for(int i =0; i<maxBufferSize; i++) {
                                //finding which pageFrame is null.
                                if(queue[i].framePointer == NULL) {
                                        queue[i].position = 1;
                                        queue[i].pNo = pageNum;
                                        for(int j=0; j<maxBufferSize; j++) {
                                                if(j != i)
                                                        queue[j].position +=1;
                                        }
                                        queue[i].framePointer = returnPagePointer(bm,page);
                                        currentQueueSize++;
                                        return;
                                }
                        }
                }
        }
        //If the queue is full we need to do FIFO.
        else if(currentQueueSize == queueSize ) {


                for(int i =0; i<currentQueueSize; i++) {
                        //if the page already exists in buffer then just increment the fix count.
                        if(pageFrames[i].PageNumber == pageNum) {
                                pageFrames[i].fixCount += 1;
                                return;
                        }
                        //Remove the element with the maximum position value which will be first one which came in to the buffer
                        if(queue[i].position == currentQueueSize) {
                                if(pageFrames[i].fixCount == 0) {

                                        //change its position to the first and insert the new frame.
                                        queue[i].position = 1;
                                        for(int j=0; j<currentQueueSize; j++) {
                                                if(j != i)
                                                        queue[j].position++;

                                        }

                                        SM_FileHandle fh;
                                        if(pageFrames[i].dirty ==1) {
                                                openPageFile(bm->pageFile, &fh);
                                                ensureCapacity(pageFrames[i].PageNumber,&fh);
                                                writeBlock(pageFrames[i].PageNumber, &fh, pageFrames[i].content);
                                                writeCount++;
                                        }

                                        // pin the page and store the pagenumber.
                                        pageFrames[i].pinStatus = 1;
                                        pageFrames[i].PageNumber = pageNum;
                                        //printf("%d",pf->PageNumber);
                                        pageFrames[i].freeStat = 1;
                                        pageFrames[i].dirty = 0;
                                        pageFrames[i].fixCount = 0;

                                        //read the data from the disk and store it in the page handler.

                                        openPageFile(bm->pageFile,&fh);
                                        ensureCapacity(pageFrames[i].PageNumber, &fh);
                                        readBlock(pageFrames[i].PageNumber,&fh,pageFrames[i].content);

                                        //increment the read count
                                        readCount++;

                                        //update the page handler.
                                        page->data  = pageFrames[i].content;
                                        page->pageNum = pageFrames[i].PageNumber;


                                        return;
                                }
                        }
                }

                // if the page is in use by other client find the next largest page and check if we can replace that
                for(int i =0; i<currentQueueSize; i++) {
                        int temp = currentQueueSize-1;
                        if(queue[i].position == temp && pageFrames[i].fixCount == 0) {
                                queue[i].position = 1;
                                for(int j=0; j<currentQueueSize; j++) {
                                        if(j != i)
                                                queue[j].position++;

                                }

                                SM_FileHandle fh;
                                if(pageFrames[i].dirty ==1) {
                                        openPageFile(bm->pageFile, &fh);
                                        ensureCapacity(pageFrames[i].PageNumber,&fh);
                                        writeBlock(pageFrames[i].PageNumber, &fh, pageFrames[i].content);
                                        writeCount++;
                                }

                                // pin the page and store the pagenumber.
                                pageFrames[i].pinStatus = 1;
                                pageFrames[i].PageNumber = pageNum;
                                pageFrames[i].freeStat = 1;
                                pageFrames[i].dirty = 0;
                                pageFrames[i].fixCount = 0;

                                //read the data from the disk and store it in the page handler.
                                openPageFile(bm->pageFile,&fh);
                                ensureCapacity(pageFrames[i].PageNumber, &fh);
                                readBlock(pageFrames[i].PageNumber,&fh,pageFrames[i].content);

                                //increment the read count
                                readCount++;

                                //update the page handler.
                                page->data  = pageFrames[i].content;
                                page->pageNum = pageFrames[i].PageNumber;


                                return;
                        }
                }
        }
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum){


        int size = bm->numPages;
        Frame *pageFrame = (Frame *)bm->mgmtData;

        //if the buffer is full call the replacement strategy.
        if(bufferSize == maxBufferSize) {
                isBufferFull = 1;
                if(bm->strategy == RS_FIFO)
                        FIFO(bm,page,pageNum);
                else
                        LRU(bm,page,pageNum);
                return RC_OK;
        }
        else if(isBufferFull == 0) {
                if(bufferSize == 0) {
                        if(bm->strategy == RS_FIFO)
                                FIFO(bm,page,pageNum);
                        else
                                LRU(bm,page,pageNum);

                        SM_FileHandle fh;

                        // pin the page and store the pagenumber.
                        pageFrame[0].pinStatus = 1;
                        pageFrame[0].PageNumber = pageNum;
                        pageFrame[0].freeStat = 1;
                        pageFrame[0].fixCount++;

                        //read the data from the disk and store it in the page handler.

                        openPageFile(bm->pageFile,&fh);
                        ensureCapacity(pageNum, &fh);
                        readBlock(pageNum,&fh,pageFrame[0].content);

                        //increment the read count
                        readCount++;

                        //update the page handler.
                        page->data  = pageFrame[0].content;
                        page->pageNum = pageFrame[0].PageNumber;


                        bufferSize++;
                        return RC_OK;

                }
                else{
                        for(int i=1; i<size; i++) {
                                if(pageFrame[i].freeStat == 0) {
                                        SM_FileHandle fh;
                                        if(bm->strategy == RS_FIFO)
                                                FIFO(bm,page,pageNum);
                                        else
                                                LRU(bm,page,pageNum);

                                        // pin the page and store the pagenumber.
                                        pageFrame[i].freeStat = 1;
                                        pageFrame[i].pinStatus = 1;
                                        pageFrame[i].PageNumber = pageNum;
                                        pageFrame[i].fixCount++;


                                        openPageFile(bm->pageFile,&fh);
                                        ensureCapacity(pageNum, &fh);
                                        readBlock(pageNum,&fh,pageFrame[i].content);

                                        readCount++;

                                        //update the page handler.
                                        page->data  = pageFrame[i].content;
                                        page->pageNum = pageNum;

                                        bufferSize++;
                                        return RC_OK;
                                }
                        }
                        return RC_OK;
                }
        }

}

//unpinning the page.
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){

        Frame *pageFrame = (Frame *)bm->mgmtData;
        int i;
        for(i=0; i<bufferSize; i++) {
                if(pageFrame[i].PageNumber == page->pageNum) {

                        //make the pin status as 0.
                        pageFrame[i].pinStatus = 0;
                        //decrement the fix count since client is unpinning.
                        if(pageFrame[i].fixCount> 0)
                                pageFrame[i].fixCount--;
                        else
                                pageFrame[i].fixCount=0;
                        pageFrame[1].pinStatus = 0;

                        return RC_OK;
                }
        }
}

//marking the page as dirty.
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){

        Frame *pageFrame = (Frame *)bm->mgmtData;
        int i;
        for(i=0; i<bufferSize; i++) {
                if(pageFrame[i].PageNumber == page->pageNum) {
                        pageFrame[i].dirty = 1;

                        return RC_OK;
                }
        }
        return RC_ERROR;
}

//shut down the buffer pool and free all the memory allocated. write to disk all the pages before shutting down.
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
        Frame *pageFrame = (Frame *)bm->mgmtData;
        Queue *queue = pageFrame[0].qPointer;

        //writing all the data back to the disk.
        forceFlushPool(bm);

        int i;
        for(i = 0; i < bufferSize; i++)
        {

                if(pageFrame[i].pinStatus != 0)
                {
                        return RC_ERROR;
                }

        }
        //freeing the memory allocated to the content variable in frame pointer.
        for(i=0;i< maxBufferSize;i++){
          free(pageFrame[i].content);
        }


        free(queue);
        free(pageFrame);

        //reinitialize all the variables back to the normal state.
        bufferSize = 0;
        isBufferFull = 0;
        currentQueueSize=0;
        bm->mgmtData = NULL;

        return RC_OK;
}


extern RC forceFlushPool(BM_BufferPool *const bm)
{
        Frame *pageFrame = (Frame *)bm->mgmtData;

        int i;

        for(i = 0; i < bufferSize; i++)
        {        //check if a page frame is drity, if its dirty write it back to the disk.
                if(pageFrame[i].dirty == 1)
                {
                        SM_FileHandle fh;

                        openPageFile(bm->pageFile, &fh);

                        ensureCapacity(pageFrame[i].PageNumber,&fh);

                        writeBlock(pageFrame[i].PageNumber, &fh, pageFrame[i].content);

                        //make the dirty bit as 0.
                        pageFrame[i].dirty = 0;

                        //increment the write count.
                        writeCount++;
                }
        }
        return RC_OK;
}

//force write a page to the disk.
extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
        Frame *pageFrame = (Frame *)bm->mgmtData;

        for(int i = 0; i < bufferSize; i++)
        {
                //force a page to the disk using the page number.
                if(pageFrame[i].PageNumber == page->pageNum)
                {
                        //write the contents to the disk.
                        SM_FileHandle fh;
                        openPageFile(bm->pageFile, &fh);
                        writeBlock(pageFrame[i].PageNumber, &fh, pageFrame[i].content);

                        //increment the write count.
                        writeCount++;

                        pageFrame[i].dirty= 0;
                }
        }
        return RC_OK;
}

//returns the contents of the frame.
extern PageNumber *getFrameContents (BM_BufferPool *const bm)
{

        PageNumber *pageNumbers = malloc(sizeof(PageNumber) * bufferSize);
        Frame *pageFrame = (Frame *) bm->mgmtData;

        int i = 0;
        //returns the number if its -1 it returns the constant NO_PAGE.
        while(i < maxBufferSize) {
                pageNumbers[i] = (pageFrame[i].PageNumber != -1) ? pageFrame[i].PageNumber : NO_PAGE;
                i++;
        }
        return pageNumbers;
        free(pageNumbers);
}

//returns the boolean value(true,false) for the dirty bits in the page Frame.
extern bool *getDirtyFlags (BM_BufferPool *const bm)
{
        bool *dirtyBits = malloc(sizeof(bool) * bufferSize);
        Frame *pageFrame = (Frame *)bm->mgmtData;

        int i;
        for(i = 0; i < maxBufferSize; i++)
        {
                dirtyBits[i] = (pageFrame[i].dirty == 1) ? true : false;
        }
        return dirtyBits;
        free(dirtyBits);
}

//returns the fix counts of the page frames.
extern int *getFixCounts (BM_BufferPool *const bm)
{
        int *fixCounts = malloc(sizeof(int) * bufferSize);
        Frame *pageFrame= (Frame *)bm->mgmtData;

        int i = 0;
        while(i < maxBufferSize)
        {
                fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0;
                i++;
        }
        return fixCounts;
          free(fixCounts);
}

//returns the number of reads done by the buffer manager.
extern int getNumReadIO (BM_BufferPool *const bm)
{
        return readCount;
}

//returns the number of writes done by the buffer manager,
extern int getNumWriteIO (BM_BufferPool *const bm)
{
        return writeCount;
}
