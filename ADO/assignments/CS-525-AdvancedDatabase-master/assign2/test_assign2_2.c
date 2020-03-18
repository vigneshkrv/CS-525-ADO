#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "test_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// var to store the current test's name
char *testName;

// check whether two the content of a buffer pool is the same as an expected content
// (given in the format produced by sprintPoolContent)
#define ASSERT_EQUALS_POOL(expected,bm,message)			        \
  do {									\
    char *real;								\
    char *_exp = (char *) (expected);                                   \
    real = sprintPoolContent(bm);					\
    if (strcmp((_exp),real) != 0)					\
      {									\
	printf("[%s-%s-L%i-%s] FAILED: expected <%s> but was <%s>: %s\n",TEST_INFO, _exp, real, message); \
	free(real);							\
	exit(1);							\
      }									\
    printf("[%s-%s-L%i-%s] OK: expected <%s> and was <%s>: %s\n",TEST_INFO, _exp, real, message); \
    free(real);								\
  } while(0)

// test and helper methods
static void testCreatingAndReadingDummyPages (void);
static void createDummyPages(BM_BufferPool *bm, int num);
static void checkDummyPages(BM_BufferPool *bm, int num);
static void testFIFO (void);
static void testLRU (void);

// main method
int
main (void)
{
  initStorageManager();
  testName = "";

  //testCreatingAndReadingDummyPages();
  //testReadPage();
  //testFIFO();
  testLRU();
}


void
createDummyPages(BM_BufferPool *bm, int num)
{
  int i;
    BM_PageHandle *h = MAKE_PAGE_HANDLE();

    CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL));

    for (i = 0; i < num; i++)
      {
        CHECK(pinPage(bm, h, i));
        sprintf(h->data, "%s-%i", "Page", h->pageNum);
        CHECK(markDirty(bm, h));
        CHECK(unpinPage(bm,h));
      }

    CHECK(shutdownBufferPool(bm));

    free(h);
}

void
checkDummyPages(BM_BufferPool *bm, int num)
{
  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  char *expected = malloc(sizeof(char) * 512);

  CHECK(initBufferPool(bm, "testbuffer.bin", 3, RS_FIFO, NULL));

  for (i = 0; i < num; i++)
    {
      CHECK(pinPage(bm, h, i));

      sprintf(expected, "%s-%i", "Page", h->pageNum);
      ASSERT_EQUALS_STRING(expected, h->data, "reading back dummy page content");

      CHECK(unpinPage(bm,h));
    }

  CHECK(shutdownBufferPool(bm));

  free(expected);
  free(h);
}


// test the LRU page replacement strategy
void
testLRU (void)
{
  // expected results
  const char *poolContents[] = {
    // read first five pages and directly unpin them
    "[0 0],[-1 0],[-1 0],[-1 0]" ,
    "[0 0],[1 0],[-1 0],[-1 0]",
    "[0 0],[1 0],[2 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0]",

    // use some of the page to create a fixed LRU order without changing pool content
    "[0 0],[1 0],[2 0],[3 0]",
    "[0 0],[1 0],[2 0],[3 0]",
    "[0 0],[1 0],[2 0],[3 0]",
    "[0 0],[1 0],[2 0],[3 0]",

    // check that pages get evicted in LRU order
    "[0 0],[4 0],[2 0],[3 0]",
    "[0 0],[4 0],[2 0],[5 0]",
    "[0 0],[4 0],[6 0],[5 0]",
    "[7 0],[4 0],[6 0],[5 0]",

    //mark dirty a few bits and unpinPage
    "[7 0],[4x0],[6 0],[5 0]",
    "[7x0],[4x0],[6 0],[5 0]",
    "[7 0],[4 0],[6 0],[5 0]",

    };
  const int orderRequests[] = {1,3,2,0};
  const int numLRUOrderChange = 4;

  int i;
  int snapshot = 0;
  BM_BufferPool *bm = MAKE_POOL();
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  testName = "Testing LRU page replacement";

  CHECK(createPageFile("testbuffer.bin"));
  createDummyPages(bm, 100);
  CHECK(initBufferPool(bm, "testbuffer.bin", 4, RS_LRU, NULL));

  // reading first five pages linearly with direct unpin and no modifications
  for(i = 0; i < 4; i++)
  {
      pinPage(bm, h, i);
      unpinPage(bm, h);
      ASSERT_EQUALS_POOL(poolContents[snapshot], bm, "check pool content reading in pages");
      snapshot++;
  }

  // read pages to change LRU order
  for(i = 0; i < numLRUOrderChange; i++)
  {
      pinPage(bm, h, orderRequests[i]);
      unpinPage(bm, h);
      ASSERT_EQUALS_POOL(poolContents[snapshot], bm, "check pool content using pages");
      snapshot++;
  }
  //pin the 4th page
  pinPage(bm, h, 4);

  // replace pages and check that it happens in LRU order
  for(i = 0; i < 4; i++)
  {
      pinPage(bm, h, 4 + i);
      unpinPage(bm, h);
      ASSERT_EQUALS_POOL(poolContents[snapshot], bm, "check pool content using pages");
      snapshot++;
  }



  //Mark the 7th page as dirty.
  h->pageNum = 4;
  markDirty(bm,h);
  ASSERT_EQUALS_POOL(poolContents[snapshot], bm, "check pool content using pages");
  snapshot++;

  //Mark the 4th page as dirty.
  h->pageNum = 7;
  markDirty(bm,h);
  ASSERT_EQUALS_POOL(poolContents[snapshot], bm, "check pool content using pages");
  snapshot++;



  forceFlushPool(bm);
  ASSERT_EQUALS_POOL(poolContents[snapshot],bm,"pool content after flush");






  // check number of write IOs
  ASSERT_EQUALS_INT(2, getNumWriteIO(bm), "check number of write I/Os");
  ASSERT_EQUALS_INT(8, getNumReadIO(bm), "check number of read I/Os");

  CHECK(shutdownBufferPool(bm));
  CHECK(destroyPageFile("testbuffer.bin"));

  free(bm);
  free(h);

  TEST_DONE();
}
