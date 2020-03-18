#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include <ctype.h>

//Datastructure to maintain the state data for the Record manager
typedef struct DatabaseManager {
        BM_PageHandle page;
        BM_BufferPool buffer;
        int numRows;
        int maxRecords;
        int freePage;
        int snumAttr;
        char sattrName[5];
        int sdataTypes[5];
        int stypeLength[5];


}DatabaseManager;

DatabaseManager *dbm;

//A fixed attribute size
int attribute_size = 10;

//denotes the amount of records scanned.
int scanCount =  0;


//denotes the page Number.
int pNo = 0;

// ======= TABLE AND MANAGER FUNCTIONS ======== //
//Initializes the record manager by initializing the storage manager.
extern RC initRecordManager(void *mgmtData){
        initStorageManager();
        return RC_OK;
}

//It shuts down the record manager and also the Buffer Manager.
extern RC shutdownRecordManager (){
        shutdownBufferPool(&dbm->buffer);
        dbm = NULL;
        free(dbm);
        return RC_OK;
}

//Creates the Table using the storage manager functions and A FIFO Queue with size 40.
extern RC createTable (char *name, Schema *schema){
        //allocate memory for our data structure.
        dbm =(DatabaseManager *) malloc(sizeof(DatabaseManager));
        int result;

        //create a buffer manager with 40 pages and a FIFO queue.
        if(initBufferPool(&dbm->buffer,name,40,RS_FIFO,NULL) != RC_OK) {
                return RC_ERROR;
        }

        //allocate a string equal to the size of a page.
        char data[PAGE_SIZE];
        char *dataPage;

        //make all the characters in the page as '-' which is a keyword for this program.
        for(int i =0; i<PAGE_SIZE; i++) {
                data[i] = '-';
        }


        int index = 0;

        //store the first free page of the table. We use 0 page for schema.
        data[index] = '1';
        index = index + sizeof(int);
        dbm->freePage = 1;

        //store the number of rows in the table. when creating new table its always 0.
        data[index] = '0';
        index = index + sizeof(int);
        dbm->numRows = 0;



        char temp[20];
        //store the number of attribues in the schema
        sprintf(temp,"%d",schema->numAttr);
        data[index]= *temp;
        index = index + sizeof(int);

        //copy it to our data structure.
        dbm->snumAttr = schema->numAttr;


        int i,len,count = 3;

        //store all the attributes of the schema to the table.
        for(i = 0; i < schema->numAttr; i++) {
                dataPage = &data[index];
                //copy the attribute names.
                for(int j=0; j<strlen(schema->attrNames[i]); j++) {
                        data[index] = schema->attrNames[i][j];
                }
                char *temp = &dbm->sattrName[i];
                strncpy(temp,schema->attrNames[i],1);

                //copy the attribute types.
                index = index + attribute_size;
                sprintf(temp,"%d",schema->dataTypes[i]);
                data[index] = *temp;
                index = index + sizeof(int);
                dbm->sdataTypes[i] = schema->dataTypes[i];

                //copy the attribute typelength.
                char ts[20];
                sprintf(ts,"%d",schema->typeLength[i]);
                data[index] = *ts;
                index = index + sizeof(int);
                dbm->stypeLength[i] = schema->typeLength[i];



        }
        //mark the end of string
        data[index] = '\0';

        //the file handler.
        SM_FileHandle fh;

        //Create the pageFile using the file handler and insert the data string into the page.
        if((result =createPageFile(name)) != RC_OK)
                return result;
        if((result = openPageFile(name,&fh)) != RC_OK)
                return result;
        if((result = writeBlock(0, &fh, data)) != RC_OK)
                return result;


        if((result = closePageFile(&fh)!= RC_OK))
                return result;

        return RC_OK;
}

//Open the table file from the disk and perform operations and set data in the RM_TableData structure;
extern RC openTable (RM_TableData *rel, char *name){
        //the data page.
        SM_PageHandle page;

        //set the name of the table.
        rel->name = name;

        //we store the datastructure address here.
        rel->mgmtData = dbm;

        //allocating space for our schema.
        Schema *s = (Schema *)malloc(sizeof(Schema));

        //set the number of attributes.
        s->numAttr = dbm->snumAttr;


        int count = s->numAttr;

        // Allocate the space for the attribute names,datatypes and typeLengths.
        s->attrNames = (char **)malloc(count * sizeof(char *));
        s->dataTypes = malloc(count * sizeof(DataType));
        s->typeLength = malloc(count * sizeof(int));

        int i,len,c=3;

        //set the attribute names by mallocing the 2D char array.
        for(i =0; i<3; i++) {

                s->attrNames[i] = (char *)malloc(attribute_size);

                //set the attribute name.
                char *temp = &dbm->sattrName[i];
                strncpy(s->attrNames[i],temp,1);

                //set the datatype.
                s->dataTypes[i] = dbm->sdataTypes[i];

                //set the typeLength.
                s->typeLength[i] = dbm->stypeLength[i];

        }

        //store the schema to the Table Handler.
        rel->schema = s;


        return RC_OK;

}

//Close the table by flushing all the changes to the Disk.
extern RC closeTable (RM_TableData *rel){
        DatabaseManager  *dbmanger = rel->mgmtData;
        forceFlushPool(&dbmanger->buffer);

        return RC_OK;
}

//Delete the table using the record manager functions.
extern RC deleteTable (char *name){
        destroyPageFile(name);

        return RC_OK;
}

//Returns the number of tuples in the table.
extern int getNumTuples (RM_TableData *rel){
        DatabaseManager *dbm = (DatabaseManager *)rel->mgmtData;
        return dbm->numRows;

}

// ==== HANDLING RECORDS IN THE TABLE FUNCTIONS ==== //
//Insert record, inserts a record into the table taking data from the record structure.
extern RC insertRecord (RM_TableData *rel, Record *record){
        //Get the address for our data structure.
        DatabaseManager *dbmanager = rel->mgmtData;

        //Set char to the page size
        char d[PAGE_SIZE];
        char *data,*location;

        //get the MaxRecords.
        int max_records = dbmanager->maxRecords;

        //Get the number of Rows in the table.
        int numRows = getNumTuples(rel);
        int freeSlot,pageNumber;

        // Get the size of each record by using the Schema.
        int record_size = getRecordSize(rel->schema);

        //set the pageNumber as the page marked as free Page.
        pageNumber = dbmanager->freePage;

        //Pin the first free page and try to write the record.
        pinPage(&dbmanager->buffer,&dbmanager->page,pageNumber);


        int loc=0;

        //get the address for the RID data structure.
        RID *recordID = &record->id;

        //make the char data pointer to the pinned page data.
        data = dbmanager->page.data;

        //set the recordID page as the current Free Page.
        recordID->page = pageNumber;

        //The free slot is the current number of rows in the table.
        freeSlot = dbmanager->numRows;

        //mark the slot for the record.
        recordID->slot = freeSlot;
        int flag = 0;

        //If the number of rows exceeds the page capacity move to the next page.
        if(numRows > (PAGE_SIZE/record_size)) {
                //increase the page count.
                pageNumber++;
                dbmanager->freePage = dbmanager->freePage + 1;
                flag = 1;

                //make the number of rows in the page as 0.
                dbmanager->numRows = 0;

                //unpin the current page to pin the next Page.
                unpinPage(&dbmanager->buffer,&dbmanager->page);

                //Set the recordID page and slot to the new values.
                recordID->page= pageNumber;
                recordID->slot = dbmanager->numRows;

                //pin the new page into the buffer for reading the data.
                pinPage(&dbmanager->buffer,&dbmanager->page,recordID->page);

                //set the char data pointer to the pinned page data.
                data =  dbmanager->page.data;

        }

        //mark them dirty because we are inserting a record.
        markDirty(&dbmanager->buffer,&dbmanager->page);

        //To find the starting point for the record, we have to move the pointer to the
        //slot which is free. so using the formula below we can move the pointer.
        location = data;
        location = location + (recordID->slot * record_size);

        //same formula but for a numerical position.
        loc = (recordID->slot * record_size);

        //using the numerical positon copy record data into the page data one character at a time.
        for(int counter = 0; counter < record_size; counter++) {
                data[loc++] = record->data[counter];
        }

        //Unpin the page
        unpinPage(&dbmanager->buffer,&dbmanager->page);

        //force the page to the disk
        forcePage(&dbmanager->buffer,&dbmanager->page);

        //Increment the number of rows in the table.
        dbmanager->numRows++;

        return RC_OK;
}

//Update the record having ID in the RID and the corresponding table.
extern RC updateRecord (RM_TableData *rel, Record *record){
        //get the datamanager structure.
        DatabaseManager *dbmanager = rel->mgmtData;

        char *dataPointer;

        //Pin the page that has the record.
        pinPage(&dbmanager->buffer,&dbmanager->page,record->id.page);

        //get the ID for the record.
        RID recordID = record->id;

        //get the size of the record.
        int record_size = getRecordSize(rel->schema);

        //set the char pointer to the starting of the pinned page data.
        dataPointer = dbmanager->page.data;

        //move the data pointer to the specified record.
        dataPointer = dataPointer + (recordID.slot * record_size);

        //copy the data from the record to the datapointer.
        memcpy(dataPointer,record->data,record_size);

        //mark the page as dirty since we are updating
        markDirty(&dbmanager->buffer,&dbmanager->page);
        //unpin the page.
        unpinPage(&dbmanager->buffer,&dbmanager->page);
        //force the page back to the disk.
        forcePage(&dbmanager->buffer,&dbmanager->page);

        return RC_OK;
}

//Delete the record.
extern RC deleteRecord(RM_TableData *rel, RID id){
        //get the data structure
        DatabaseManager *dbmanager = rel->mgmtData;

        //Pin the page which has teh record.
        pinPage(&dbmanager->buffer,&dbmanager->page,id.page);

        //dbmanager->freePage = id.page;

        //set the char pointer to the pinned page data.
        char *data =dbmanager->page.data;

        //get the size of the record.
        int record_size = getRecordSize(rel->schema);

        //using the token mark the record as deleted.
        for(int i=0; i<record_size; i++) {
                data[(id.slot * record_size) + i] = '-';
        }

        //mark the page as dirty
        markDirty(&dbmanager->buffer,&dbmanager->page);

        //unpin the page.
        unpinPage(&dbmanager->buffer,&dbmanager->page);
        //force the page to the disk.
        forcePage(&dbmanager->buffer,&dbmanager->page);

        return RC_OK;
}

//Retreives the record from the memory.
extern RC getRecord (RM_TableData *rel, RID id, Record *record){
        //get the data structure.
        DatabaseManager *dbmanager = (DatabaseManager *)rel->mgmtData;

        //Pin the page which has the record.
        pinPage(&dbmanager->buffer,&dbmanager->page,id.page);

        //get the size of the record.
        int record_size = getRecordSize(rel->schema);

        //set the char pointer to the pinned page data.
        char *d = dbmanager->page.data;

        //for the record size copy the record data into the record handler.
        for(int i =0; i<record_size; i++) {
                record->data[i] = d[(id.slot*record_size)+i];
        }

        //un pin the page from the buffer.
        unpinPage(&dbmanager->buffer,&dbmanager->page);

        return RC_OK;
}


// ===== SCAN FUNCTIONS ====//
//initializes the scan and sets the condition into the mgmtData void Pointer and sets the table handler
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){
        scan->mgmtData = cond;
        scan->rel = rel;

        return RC_OK;
}

//Get the next record that matches the scan condition.
extern RC next (RM_ScanHandle *scan, Record *record){
        //Fetch the expression condition from the mgmtData of scan handler.
        Expr *cond = (Expr *)scan->mgmtData;

        //Get the schema of the table.
        Schema *schema = scan->rel->schema;

        //Check and see if the condition is NULL
        if (cond == NULL)
        {
                return RC_ERROR;
        }

        //Allocate space to store the value
        Value *result = (Value *) malloc(sizeof(Value));

        //Get the size of the record.
        int record_size = getRecordSize(schema);

        //Calculate the maximum amount of records for the page.
        int maxRecords = PAGE_SIZE/record_size;

        //get the number of rows in the table currently.
        int totalRows = dbm->numRows;

        // Runs a loop until all the records are scanned.
        while(scanCount <= totalRows) {
                //pin the page which has the data.
                pinPage(&dbm->buffer,&dbm->page,1);

                //set the char pointer to the pinned page data.
                char *data = dbm->page.data;

                //move the pointer to the start location of the data.
                data = data + (record_size*scanCount);

                //set the record ID datas.
                record->id.page = 1;
                record->id.slot = scanCount;

                //make the char pointer to the starting point of the record data.
                char *recordData = record->data;

                //increase the number of rows scanned
                scanCount++;

                //copy the data from the page pointer to the record.
                memcpy(recordData,data,record_size);

                //check if the record satisfies the expression.
                evalExpr(record,schema,cond,&result);

                //if the result is satisfies we return to the main function
                if(result->v.boolV == TRUE) {
                        //unpin the page
                        unpinPage(&dbm->buffer,&dbm->page);
                        return RC_OK;
                }
        }
        //make the number of rows scanned as 0.
        scanCount = 0;

        //mark no more tuples to be scanned anymore.
        return RC_RM_NO_MORE_TUPLES;
}

//Close the scan.
extern RC closeScan (RM_ScanHandle *scan)
{
        return RC_OK;
}


// ====== SCHEMA FUNCTIONS =====//
//get the size of the record.
extern int getRecordSize (Schema *schema){
        // get the schema.
        Schema *s = schema;
        int count = s->numAttr;
        int i,size = 0;

        //for all the attributes in the schema increase the space nneeded to store the record.
        for(i=0; i<count; i++) {

                switch(s->dataTypes[i]) {
                case DT_INT: size += sizeof(int); break;
                case DT_STRING: size += s->typeLength[i]; break;
                case DT_FLOAT: size += sizeof(float); break;
                case DT_BOOL: size += sizeof(bool); break;
                }
        }

        return size;
}

//create the schema from the parameters
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
        Schema *s = (Schema *)malloc(sizeof(Schema));
        s->numAttr = numAttr;
        s->attrNames = attrNames;
        s->dataTypes = dataTypes;
        s->typeLength = typeLength;
        s->keySize = keySize;
        s->keyAttrs = keys;

        return s;
}

//Delete the schema and all the iterative mallocs.
extern RC freeSchema (Schema *schema){
        Schema *s = schema;
        int i;
        for(i =0; i<s->numAttr; i++) {
                free(s->attrNames[i]);
        }
        free(s->typeLength);
        free(s->dataTypes);
        free(s->keyAttrs);
        free(s);
        return RC_OK;
}


//==== DEALING WITH THE RECORD AND ATTRIBUTE VALUE FUNCTIONS ====//
//create the Record. The reocord is of the style (---------) for the size of the record.
extern RC createRecord (Record **record, Schema *schema){
        //allocate space for the record.
        Record *r =(Record *)malloc(sizeof(Record));

        //mark the slot as -1 saying it is not stored yet.
        r->id.page = r->id.slot = -1;

        //get the size of the record.
        int size = getRecordSize(schema);

        //allocate space for storing the data of the record.
        r->data = (char*)malloc(size);

        //fil the record with the keyword -.
        for(int k=0; k<getRecordSize(schema); k++) {
                r->data[k] = '-';
        }

        //copy the record address the parameter.
        *record = r;
        return RC_OK;
}

//Get the current offset for the attribute to get/set the attribute.
extern int getAttributeOffset(int attrNum, Schema *schema){
        int i,offset =0;

        for(i=0; i<attrNum; i++) {

                switch(schema->dataTypes[i]) {
                case DT_INT: offset += sizeof(int); break;
                case DT_STRING: offset += schema->typeLength[i]; break;
                case DT_FLOAT: offset += sizeof(float); break;
                case DT_BOOL: offset += sizeof(bool); break;
                }
        }

        return offset;
}
//deallocate the memory allocated for the record.
extern RC freeRecord (Record *record){
        free(record->data);
        free(record);
        return RC_OK;
}

//set the attribute for the record.
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){
        //get total number of attributes.
        int numAttr = schema->numAttr;
        int i;
        int offset=0;
        int dataP = 0;

        //get the offset for the attribute in the record.
        offset = getAttributeOffset(attrNum,schema);

        //calculate the position.
        dataP = dataP + offset;

        //according to the dataType set the value.
        switch(schema->dataTypes[attrNum]) {
        case 0: {
                // for INT value perform the count and insert it into the position of the record.
                char temp[20];
                sprintf(temp,"%d", value->v.intV);
                int count =0;
                //find the amount of digits.
                while(value->v.intV != 0)
                {
                        value->v.intV /= 10;
                        ++count;
                }
                //insert the digits into the record.
                for(int t = 0; t < count; t++) {
                        record->data[dataP+t] = temp[t];
                }
                break;
        }
        case 1: {
                //for the string insert the data.
                int length = schema->typeLength[attrNum];
                char *charPointer = &record->data[dataP];
                int t = dataP;
                for(int k=0; k<strlen(value->v.stringV); k++) {
                        record->data[t+k] = value->v.stringV[k];
                }
                break;
        }
        case 2: {
                char temp[20];
                sprintf(temp,"%f", value->v.floatV);
                record->data[dataP] =*temp;
                break;
        }
        case 3: {
                char temp[20];
                sprintf(temp,"%d", value->v.boolV);
                record->data[dataP] =*temp;
                break;
        }
      }
        return RC_OK;
}

//get the attribute from the record.
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){
        //allocate the space for the Value.
        Value *new_value = (Value *)malloc(sizeof(Value));
        int offset = 0;
        int dataP = 0;

        //get the attribute offset.
        offset = getAttributeOffset(attrNum,schema);

        //calculate the position.
        dataP = dataP + offset;

        //set the data pointer to the record data.
        char *data = record->data;
        data = data + offset;
        switch(schema->dataTypes[attrNum]) {
        case 0: {
                //for int data type calcualte the digits
                char temp[20];
                int count = 0;
                int i;
                //calculate if its a digit.
                for(i =0; i<4; i++) {
                        if((data[i] - '0') > 0) {
                                temp[count] = data[i];
                                count++;
                        }
                        else{
                                break;
                        }

                }
                //write them into the int after converting to int from string..
                int vs=0;
                for(int k =0; k<count; k++) {
                        vs = vs * 10 +(temp[k] - '0');

                }
                new_value->v.intV  = vs;
                new_value->dt = 0;
                break;
        }
        case 1: {
                //copy the string from the record to the value data structure.
                new_value->v.stringV = (char *)malloc(4);
                new_value->dt = 1;
                strncpy(new_value->v.stringV,data,4);
                new_value->v.stringV[4] = '\0';
                break;
        }
        case 2: {
                new_value->v.floatV  = data[dataP] - '0';
                new_value->dt = 2;
                break;
        }
        case 3: {
                new_value->v.boolV  = data[dataP] - '0';
                new_value->dt = 3;
                break;
        }
      }
        *value = new_value;
        return RC_OK;

}
