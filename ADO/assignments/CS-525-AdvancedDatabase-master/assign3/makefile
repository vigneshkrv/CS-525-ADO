HEADERS = storage_mgr.h dberror.h test_helper.h buffer_mgr_stat.h tables.h record_mgr.h
CC = gcc
CFLAGS = -I

default: test_1

test_1: test_case_1.o buffer_mgr.o storage_mgr.o dberror.o buffer_mgr_stat.o expr.o record_mgr.o rm_serializer.o
	$(CC) -o test_1 test_case_1.o buffer_mgr.o storage_mgr.o dberror.o buffer_mgr_stat.o expr.o record_mgr.o rm_serializer.o

test_case_1.o: test_assign3_1.c $(HEADERS)
	$(CC) -o test_case_1.o -c test_assign3_1.c -lm

record_mgr.o: record_mgr.c $(HEADERS)
		$(CC) -c record_mgr.c -o record_mgr.o


expr.o: expr.c $(HEADERS)
		$(CC) -c expr.c -o expr.o
rm_serializer.o: rm_serializer.c $(HEADERS)
		$(CC) -c rm_serializer.c -o rm_serializer.o

buffer_mgr.o: buffer_mgr.c $(HEADERS)
		$(CC) -c buffer_mgr.c -o buffer_mgr.o
storage_mgr.o: storage_mgr.c $(HEADERS)
		$(CC) -c storage_mgr.c -o storage_mgr.o

dberror.o: dberror.c dberror.h
		$(CC) -c dberror.c

buffer_mgr_stat.o: buffer_mgr_stat.c $(HEADERS)
		$(CC) -c buffer_mgr_stat.c

clean:
	$(RM) test_1 test_2 *.o *~
run:
	./test_1
