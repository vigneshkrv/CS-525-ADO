#  Running the Script

* Go to Project root (assign2) using Terminal.

* Type ls to list the files and check that we are in the correct directory.

* Type "make" to compile all project files including "test_assign2_1.c" file

* Type "make run_test_1" to run "test_assign2_1.c" file.

* Type "make test_2" to compile Custom test file "test_assign2_2.c".

* Type "make run_test_2" to run "test_assign2_2.c" file.

* Type "make clean" to delete old compiled .o files.

# Functions :
——————————

The main purpose of these methods is to manage a set of fixed page frames (pages in memory)
inside the buffer pool. From the storage manager the page number is used to read the requested
page inside the pool i.e pinning. A pointer to a page is returned to a request if the page is
found inside the pool or else it is fetched from the disk. If the buffer is full, one of the page frames must be replaced with another. For this purpose the pool uses any
one of the two methods namely LRU and FIFO. Least Recently Used method swaps pages that was least recently fetched. First In First Out method uses first come first serve logic. The page that was fetched first gets replaced first. Changes made in the frames are reflected
back to the disk when a page leaves the pool.

## Note : The Queue data structure in this program is not an exact implementation of a queue. The position variable is used to virtually swap the pageFrames position in the array of strucures in order to simulate the functionality of a queue. 


## Buffer Pool Functions :
_____________________

	1. initBufferPool
		- Used to create a buffer pool with fixed page frames taking count from numPages
		- All status values are set to 0 and pointers to NULL
		- The Queue and the PageFrames are also initialized with the values.

	2. shutdownBufferPool
		- Used to destroy a buffer pool
		- Frees memory allocated to page frames and queues.
		- The needed pointers and mgmtData is loaded to variables and checked against pinStatus.
		- If pinStatus is not equal 0 error is returned. Otherwise pages are freed and bufferSize is set to 0.

	3.forceFlushPool
		- Used for writing all dirty pages back to memory
		- mgmtData is loaded and all pages that are marked dirty are written to disk by running loop till bufferSize.
		- The written pages are now reset to "not dirty" i.e 0 values and writeCount is incremented.

## Page Management Functions :
_________________________

	1. pinPage
		- For pinning the respective page
		- Stores the page's page number and data in temporary variables
		- If bufferSize is equal to maxBufferSize or the buffer is full one of the replacment strategy is used for replacing and pinning.
		- Else pages are read and pinned by using readBlock() and ensureCapacity() function from storage_manager.c file.
    - The fixCount of the page is incremented.

	2. unpinPage
		- For unpinning the specified page
		- The page number in the buffer is found and its pin status is changed to 0.
		- If TRUE the pinStaus is changed and fixCount is decremented.

	3. markDirty
		- Used to a mark a page dirty
		- The dirty bit is set if the client changes the file.
		- The page number in the buffer is found and its dirty flag is changed to 1.

	4. forcePage
		- Used to write a page back to memory.
		- The page number in the buffer is found and it is written back to disk using the functions openPageFile() and writeBlock() of storage_mgr.c file and the dirty flag is changed to 0.

## Statistics Functions :
____________________

	1. getFrameContents
		- Returns the array of page numbers currently in the buffer.
		- NO_PAGE constant is returned if there are no pages currently in the buffer.

	2. getDirtyFlags
		- returns an arrays of boolean values representing the dirty status of the pages in the buffer.

	3. getFixCounts
		- returns an array of the fixCounts of all the pages in buffer.

	4. getNumReadIO
		- returns the number of reads done by the buffer manager using the readCount variable.

	5. getNumWriteIO
		- returns the number of writes done by the buffer manager using the writeCount variable.



## Test Case 2:

The test_assign2_2.c checks another implementation of the LRU algorithm where some of the pages are marked dirty and the number of writes performed by the buffer manager is verified.

## Memory Leak Checks :
——————————————————————

 - Used Valgrind to check for potential memory leaks in the program.

 - Memory Leak check were done for both the test cases. No significant loss of memory was found.
