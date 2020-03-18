#  Running the Script

* Go to Project root (assign3) using Terminal.

* Type ls to list the files and check that we are in the correct directory.

* Type "make" to compile all project files including "test_assign3_1.c" file

* Type "make run" to run "test_assign3_1.c" file.

* Type "make clean" to delete old compiled .o files.

# Functions :
——————————

The Record Manager here deals with the creation of table, schemas and record operations using the function defined in the Buffer Manager and the Record Manager.

## Table and Manager Function :
_____________________

	1. initRecordManager
		- initializes the storage manager.

	2. shutdownRecordManager
		- shuts down the record manager and shuts down the buffer manager in turn.
		- deallocates the memory for the data structure and writes all dirty pages to the disk.

	3. createTable
		- creates a table in the memory and initializes the buffer manager.
		- the buffer manager is init with 40 pages and FIFO strategy.
		- the table is created using the storage manager and written to the disk
	4. openTable
		- the table is opened from memory and the state variables are set.
	5. closeTable
		- the table is closed and the contents of the pages marked dirty are forced to the disk to maintain consistency of the database.
	6. getNumTuples
		- returns the number of rows in the table.

## Record Handling Functions :
_________________________

	1. insertRecord
		- inserts a record in to  the table.
		- it sets the ID for each record based on the page number and the slot.
		- each page can handle PAGE_SIZE/record_size number of records.
		- the records are handled via the buffer manager with minimum amount of buffer miss.

	2. deleteRecord
		- the record is deleted from the table.
		- find the location of the record using the offset and delete the record using the keyword '-'
		- mark the page dirty and write it to the disk.

	3. updateRecord
		- update the record in the table.
		- using the RID find the location of the record in the table and update the record with the new value of the record.

	4. getRecord
		- get the record associated with the RID.
		- go to the location pointed by the RID structure and fetch the record and copy it into the record data structure.

## Scan Function :
____________________

	1. startScan
		- initialize the scan handler with the condition for scan and the table associated with the scan.

	2. next
		- returns the record that match the condition.
		- it maintains a state of the amount of records scanned and it fetches the next record when the function is called again.
		- it checks if the record matches the condition provied, it returns only if it matches the condition until there's a record that matches that condition.

	3. closeScan
		- it closes the scan handler.

## Schema Function:
____________________
	1. getRecordSize
		- returns the size of the record using the schema.

	2. createSchema
		- creates the schema based on the parameters passed.
	3. freeSchema
		- deletes the schema and frees the space allocated to the schema to prevent memory leaks.

## Attribute Function:
	1. createRecord
		- creates an empty record with the keyword '-'.
		- it uses the schema to allocate space for the record.
	2. freeRecord
		- deallocates the space for the record and also deallocates the space allcoated for the string in the record.
	3. getAttr
		- returns the value of the attribute requested.
		- the attrNum represents which attribute and we allocate space for the value and store it in that structure.
	4. setAttr
		- we get the value for the attribute and store it in the record.
		- based on the datatype of the value we perform different operations to store the value.


## Memory Leak Checks :
——————————————————————

 - Used Valgrind to check for potential memory leaks in the program.

 - Memory Leak check were done for the test cases. No significant loss of memory was found.
