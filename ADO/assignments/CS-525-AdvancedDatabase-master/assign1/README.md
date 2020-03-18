
# RUNNING THE SCRIPT
## Note : A delay of 1000 ms is added in the second test case file which enables us to see the sequence of operations more clearly.

* Go to Project root (assign1) using Terminal.

* Type ls to list the files and check that we are in the correct directory.

* Type "make" to compile all project files including "test_assign1_1.c" file

* Type "make run_test_1" to run "test_assign1_1.c" file.

* Type "make test_2" to compile Custom test file "test_assign1_2.c".

* Type "make run_test_2" to run "test_assign1_2.c" file.

* Type "make clean" to delete old compiled .o files.

# File related methods


## extern void initStorageManager (void)
  - We first initialize the Storage Manager and make the FileStream object to null.

  - Methods used for file operations are createPageFile, openPageFile, closePageFile and destroPageFile.

## extern RC createPageFile(char \*fileName)

 - We create a page file in w+ mode enabling it for both read and write operations using fopen() function.
 - We create an empty PageHandle and we use calloc() for allocating memory and setting the allocated memory to 0 bytes.
 - The name of the page file created is filename as specified in the parameter.
 - Post validations, we return either RC_OK if all the operations are successful or RC_FILE_NOT_FOUND in case the file is not created successfully.
 - Page is closed and memory is freed once the operations are completed to prevent any memory leaks.

## extern RC openPageFile(char \*fileName, SM_FileHandle \*fHandle)

 - This method opens the previously created existing page file in read only (r) mode using fopen() function.
 - On successful opening of the file, we initialize the fields of SM_FileHandle structure.
  fileName is assigned with the name of the current file, curPagePos is assigned 0 because to mark the starting of the page, the file pointer is assigned to mgmtInfo for usage later and the total number of pages is calculated by getting the file size by using ftell() and moving it to the end of the file and then dividing the total size by the size of each page.
 - Post validations, we return either RC_OK if all the operations are successful or RC_FILE_NOT_FOUND in case the file is not opened successfully.

## extern RC closePageFile(SM_FileHandle \*fHandle)

 - Since fHandle->mgmtinfo contains the file pointer, we close it using fclose().
 - For validation, the return value of fclose() is verified and RC_OK or RC_ERROR is returned accordingly.

## extern RC destroyPageFile(char \*fileName)

 - remove() is used to delete the file which deletes the file from the disk.

## extern RC readBlock (int pageNum, SM_FileHandle \*fHandle, SM_PageHandle memPage)

 - This method reads blocks from the file specified by pageNum parameter and and stores the contents in the memory pointed to by memPage.
 - Validation for page file pointer is done, if it's null, RC_FILE_NOT_FOUND is returned.
 - Validation for page number is done, we check if the page number is not less than 0 and not greater than the total number of pages. RC_READ_NON_EXISTING_PAGE is returned if the page number us incorrect.
 - fseek() is used to move the cursor to the beginning of the page. PageNum \* PAGE_SIZE gives us the starting point for the page. After moving the cursor we do a fread() and read the contents of the page to the memPage PageHandle.
 - The current position is saved to the fHandle->curPagePos variable.

## extern int getBlockPos(SM_FileHandle \*fHandle)
 - This function returns the current page position.

## extern RC readFirstBlock(SM_FileHandle \*fHandle , SM_PageHandle memPage)
 - This method reads the first page in the file. readBlock funtion is called by passing pageNumber as 0.


## extern RC readLastBlock(SM_FileHandle \*fHandle , SM_PageHandle memPage)
 - This method reads the last page of the file. readBlock() is called by passing (pageNumber-1) as the argument.

## extern RC readPreviousBlock(SM_FileHandle \*fHandle , SM_PageHandle memPage)
 - This method reads the previous block, we pass (currentPage-1) as the parameter to readBlock().

## extern RC readCurrentBlock (SM_FileHandle \*fHandle, SM_PageHandle memPage)
 - This method reads the current block, readblock() is called by passing currentPage as the parameter.

## extern RC readNextBlock (SM_FileHandle \*fHandle, SM_PageHandle memPage)
 - This method reads the next block, readBlock() is called by passing (currentPage+1) as the parameter.

## extern RC writeBlock (int pageNum, SM_FileHandle \*fHandle, SM_PageHandle memPage)
 - This method writes a page to the disk. It takes contents from the memPage PageHandle and writes it to the Disk.
 - Validation for page file pointer is done, if NULL, RC_FILE_NOT_FOUND error is returned.
 - fseek() is used to move the cursor to the beginning of the page. (PageNum \* PAGE_SIZE) gives us the starting point for the page.
 - After moving the cursor, fwrite() is used to write to the disk and the current position is saved to the fHandle->curPagePos variable.

## extern RC writeCurrentBlock (SM_FileHandle \*fHandle, SM_PageHandle memPage)
 - This method writes to the current block, writeBlock() is called by passing currentPosition as the parameter.

## extern RC appendEmptyBlock (SM_FileHandle \*fHandle)
- This method is used to increase the total number of pages in the file by one.
- It adds an empty page to the last block of the file and fills it with 0 bytes.
- Total number of pages information is obtained from SM_FileHandle pointer and incremented by 1.
- fseek() is used to go to the beginning of the new page added and then is filled with 0 bytes.

## extern RC ensureCapacity(int numberOfPages, SM_FileHandle \*fHandle)
- This method is used to ensure that the total numberof pages is equal to the intended number of pages (numberOfPages)
- Calculate the number of pages to be added is done buy subtracting the numberOfPages with the total number of pages pointed to by the handler.
- If the count is > 0, new pages are added by calling appendEmptyBlock() until the count becomes 0.
- If everything is okay, RC_OK is returned.


# Memeory Leak Checks

- Used Valgrind to check for potential memory leaks and debugging the memory leaks.

## Valgrind output for Test Case 1
       HEAP SUMMARY:
       ==9174==     in use at exit: 552 bytes in 1 blocks
       ==9174==   total heap usage: 15 allocs, 14 frees, 37,104 bytes allocated
       ==9174==
       ==9174== LEAK SUMMARY:
       ==9174==    definitely lost: 0 bytes in 0 blocks
       ==9174==    indirectly lost: 0 bytes in 0 blocks
       ==9174==      possibly lost: 0 bytes in 0 blocks
       ==9174==    still reachable: 552 bytes in 1 blocks
       ==9174==         suppressed: 0 bytes in 0 blocks


## Valgrind output for Test Case 2
       HEAP SUMMARY:
       ==9316==     in use at exit: 1,104 bytes in 2 blocks
       ==9316==   total heap usage: 25 allocs, 23 frees, 60,344 bytes allocated
       ==9316==
       ==9316== LEAK SUMMARY:
       ==9316==    definitely lost: 0 bytes in 0 blocks
       ==9316==    indirectly lost: 0 bytes in 0 blocks
       ==9316==      possibly lost: 0 bytes in 0 blocks
       ==9316==    still reachable: 1,104 bytes in 2 blocks
       ==9316==         suppressed: 0 bytes in 0 blocks
