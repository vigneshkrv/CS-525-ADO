#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testNewCases(void);

/* main function running all tests */
int
main (void)
{
  testName = "";

  initStorageManager();
  testNewCases();

  return 0;
}
void delay(){
	int c = 1, d = 1;

   for ( c = 1 ; c <= 32767 ; c++ )
       for ( d = 1 ; d <= 32767 ; d++ )
       {}

}

void testNewCases(){
  SM_FileHandle fh;
  testName = "Testing additional Methods";
  SM_PageHandle ph;
  int i;
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");

  ph = (SM_PageHandle) malloc(PAGE_SIZE);
  TEST_CHECK(readFirstBlock(&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
		ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");

	printf("first block was empty\n");
  for (i=0; i < PAGE_SIZE; i++)
		ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock(0,&fh,ph));
  printf("Writing Block Number 1\n");

  delay(1000);


  TEST_CHECK(writeCurrentBlock(&fh,ph));
  printf("Writing Block Number 2\n");

  delay(1000);

  TEST_CHECK(writeCurrentBlock(&fh,ph));
  printf("Writing Block Number 3 \n");

  delay(1000);

  TEST_CHECK(writeBlock(1,&fh,ph));
  printf("Writing Block Number 2 Again\n ");

  delay(1000);


  TEST_CHECK(readBlock(0,&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("Reading First Block\n");

  delay(1000);

  TEST_CHECK(readNextBlock(&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("Reading Next Block\n");

  delay(1000);

  TEST_CHECK(readNextBlock(&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("Reading Next Block Again\n");

  delay(1000);

  TEST_CHECK(readPreviousBlock(&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("Reading Previous Block\n");

  for(i=0;i<PAGE_SIZE;i++){
    ph[i] = (i % 10) + '1';
  }
  TEST_CHECK(writeCurrentBlock(&fh,ph));
  printf("Writing A new value in the Current Block\n");
  delay(1000);

  TEST_CHECK(readCurrentBlock(&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '1'), "character in page read from disk is the one we expected.");
  printf("Reading Current Block\n");
  delay(1000);

  for(i=0;i<PAGE_SIZE;i++){
    ph[i] = (i % 10) + '2';
  }
  TEST_CHECK(writeBlock(3,&fh,ph));
  printf("Writing To Block 3 Directly\n");
  delay(1000);

  TEST_CHECK(readBlock(3,&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '2'), "character in page read from disk is the one we expected.");
  printf("Reading the 3rd Block to verify\n");


  delay(1000);


  printf("Current total number of pages : %d \n",fh.totalNumPages);
  printf("Current Page Position: %d \n",fh.curPagePos);

  printf("Ensuring Capacity to add 2 More pages and write to them\n");

  TEST_CHECK(ensureCapacity(6,&fh));
  delay(1000);
  printf("Current total number of pages : %d \n",fh.totalNumPages);
  printf("Current Page Position: %d \n",fh.curPagePos);

  for(i=0;i<PAGE_SIZE;i++){
    ph[i] = (i % 10) + '5';
  }
  TEST_CHECK(writeBlock(6,&fh,ph));
  printf("Writing To the newly created block\n");
  delay(1000);


  TEST_CHECK(readBlock(6,&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '5'), "character in page read from disk is the one we expected.");
  printf("Reading from the newely created block to verify\n");
  delay(1000);
  
  TEST_CHECK(readLastBlock(&fh,ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '5'), "character in page read from disk is the one we expected.");
  printf("Reading the Last Block to verify\n");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));


  free(ph);
  TEST_DONE();


}
