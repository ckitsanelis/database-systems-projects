#include "BF.h"
#include "hash.h"

//run with command gcc -o hash hash.c BF_64.a HT_main.c

int HT_CreateIndex( char *fileName, char attrType, char* attrName, int attrLength, int buckets) {
	int blockFile, i, j;
	HT_first* info = malloc(sizeof(HT_first));
	void *block;
	if(BF_CreateFile(fileName)<0) {
  		BF_PrintError("Could not create file\n");
  		return -1;
	}	
	blockFile = BF_OpenFile(fileName);
	if(blockFile<0) {
		BF_PrintError("Could not open file\n");
  		return -1;
	}
	if(BF_AllocateBlock(blockFile)<0) {
  		BF_PrintError("Could not allocate block\n");
  		return -1;
  	}
	strcpy(info->fileName,fileName);
	strcpy(info->attrName,attrName);
	info->attrType=attrType;
	info->attrLength=attrLength;
	info->size=buckets;
	info->isHash=1;
	int maxBuckets = BLOCK_SIZE / sizeof(int);//number of buckets each block can hold
	int blockSum=1; //number of blocks we'll need to allocate
	int bucketsLeft = buckets; //buckets that still need to fit in a block
	while(bucketsLeft >= maxBuckets) {
		blockSum++;
		bucketsLeft -= maxBuckets;
	}
	info->initialBlocks=blockSum+1;
	if(BF_ReadBlock(blockFile,0,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
  		return -1;
  	}
	memcpy(block,info,sizeof(HT_first));
	if(BF_WriteBlock(blockFile,0)) {
		BF_PrintError("Could not write to block\n");
  		return -1;
	}
	free(info);

	//hashTable construction
	for(i=0; i<blockSum; i++) { //allocate needed blocks
		int* hashTable;
		if(i==blockSum-1) {		//size of array
			hashTable = malloc( sizeof(int) * bucketsLeft);
			for (j=0; j<bucketsLeft; j++)
				hashTable[j] = -1;
		}
		else {
			hashTable = malloc( sizeof(int) * maxBuckets);
			for (j=0; j < maxBuckets; j++)
				hashTable[j] = -1;
		}
		if(BF_AllocateBlock(blockFile)<0) {
  			BF_PrintError("Could not allocate block\n");
  			return -1;
  		}
  		if(BF_ReadBlock(blockFile,i+1,&block)<0) {
 	 		BF_PrintError("Could not read block\n");
  			return -1;
  		}
  		if(i==blockSum-1)
			memcpy(block,hashTable,sizeof(int) * bucketsLeft);
		else
			memcpy(block,hashTable,sizeof(int) * maxBuckets);
		if(BF_WriteBlock(blockFile,i+1)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
  		free(hashTable);
	}
	printf("Created file with number of blocks = %d\n\n",BF_GetBlockCounter(blockFile));
	return 0;
}

HT_info* HT_OpenIndex(char *fileName) {
    int blockFile;
    void* block;
    HT_info* info;
    HT_first first_info;
    if((blockFile=BF_OpenFile(fileName))<0) {			//open the file
  		 BF_PrintError("Could not open file\n");
  		 return NULL;
  	}
    if(BF_ReadBlock(blockFile,0,&block)<0) {			//read from the first block
  		 BF_PrintError("Could not read block\n");
  		 return NULL;
  	}
  	memcpy(&first_info,block,sizeof(HT_first));
  	if(first_info.isHash!=1)			//check that the file is a hash file
  		return NULL;
  	info = malloc(sizeof(HT_info));				//allocate memory to save info
  	info->fileDesc = blockFile;
  	strcpy(info->attrName,first_info.attrName);
  	info->attrType = first_info.attrType;
  	info->attrLength = first_info.attrLength;
  	info->numBuckets = first_info.size;
  	info->initialBlocks = first_info.initialBlocks;
  	return info;
} 


int HT_CloseIndex( HT_info* header_info ) {
    if(BF_CloseFile(header_info->fileDesc)<0) {
  		BF_PrintError("Could not close file\n");
  		return -1;
  	}
    free(header_info);
    printf("Closed file and freed memory\n");
    return 0;
}

int HT_InsertEntry(HT_info header_info, Record record) {
	unsigned int hash_value;
	int maxRecords = (BLOCK_SIZE-(2*sizeof(int)))/sizeof(Record); 
	void* block;

	// Compute hash value
    if(header_info.attrType=='i' && strcmp(header_info.attrName,"id")==0)			//use hash_function_int
    	hash_value = hash_function_int(header_info.numBuckets,record.id);
    else if(header_info.attrType=='c') {											//use hash_function_char
    	if(strcmp(header_info.attrName,"city")==0)
    		hash_value = hash_function_char(header_info.numBuckets,record.city);
    	else if(strcmp(header_info.attrName,"name")==0)
    		hash_value = hash_function_char(header_info.numBuckets,record.name);
    	else if(strcmp(header_info.attrName,"surname")==0)
    		hash_value = hash_function_char(header_info.numBuckets,record.surname);
    	else{
    		printf("Incorrect 'c' attrType\n");
    		return -1;
    	}
    }
    else{
    	printf("Incorrect attrType\n");
    	return -1;
    }

    // Check if hash value block has already been created
	int maxBuckets = BLOCK_SIZE / sizeof(int);//number of buckets each block can hold
	int hashBlock=1, newBlock, numRecords, nextBlock, lastBlock, bucketValue, nextValue, blockDepth;
	while(hash_value >= maxBuckets) {
		hashBlock++;
		hash_value -= maxBuckets;
	}
	if(BF_ReadBlock(header_info.fileDesc,hashBlock,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
  		return -1;
  	}
  	memcpy(&bucketValue,block+hash_value*sizeof(int),sizeof(int));
  	if(bucketValue == -1) {												//hash value doesnt have a block
  		if(BF_AllocateBlock(header_info.fileDesc)<0) {					//create new block
  			BF_PrintError("Could not allocate block\n");
  			return -1;
  		}
  		if((newBlock=BF_GetBlockCounter(header_info.fileDesc))<0) {	//get number of newly created block
			BF_PrintError("Could not get block counter\n");
			return -1;
		}
		newBlock--;
		printf("New block has number %d\n",newBlock);
		memcpy(block+hash_value*sizeof(int),&newBlock,sizeof(int));	//write to hashtable block the corresponding block for this hash value
		if(BF_WriteBlock(header_info.fileDesc,hashBlock)<0) {
			BF_PrintError("Could not write to block\n");
			return -1;
		}
  		if(BF_ReadBlock(header_info.fileDesc,newBlock,&block)<0) {		//read the block for this hash value
			BF_PrintError("Could not read block\n");
			return -1;
		}
		numRecords = 1;
		memcpy(block,&numRecords,sizeof(int));
		nextValue = -1;
		memcpy(block+sizeof(int),&nextValue,sizeof(int));
		memcpy(block+2*sizeof(int),&record,sizeof(Record));				//copy the record after the space for the number of records in block and block number
		if(BF_WriteBlock(header_info.fileDesc,newBlock)<0) {
			BF_PrintError("Could not write to block\n");
			return -1;
		}
  	}
  	else {																// open corresponding block to hash value
  		if(BF_ReadBlock(header_info.fileDesc,bucketValue,&block)<0) {
			BF_PrintError("Could not read block\n");
			return -1;
		}

		blockDepth=1;
  		memcpy(&numRecords,block,sizeof(int));
  		if(numRecords == maxRecords) {																		//cant add another record in first block of hash value
  			int inserted = 0;
  			while(inserted == 0) {																			//need to find block that can fit another record
  				memcpy(&nextBlock,block+sizeof(int),sizeof(int));											//check if we have already made another block for this hash value
  				if(nextBlock != -1){																		//if we have
  					blockDepth++;
	  				if(BF_ReadBlock(header_info.fileDesc,nextBlock,&block)<0) {								//read the next block for this hash value
						BF_PrintError("Could not read block\n");
						return -1;
					}
					memcpy(&numRecords,block,sizeof(int));
					if(numRecords == maxRecords) {															//loop until we find a block that fits or we have to create a new block
						lastBlock = nextBlock;
					}
					else {																					//found block that has space
						memcpy(block+numRecords*sizeof(Record)+2*sizeof(int),&record,sizeof(Record));		//add record at the end
				  		numRecords++;
				  		memcpy(block,&numRecords,sizeof(int));												//+1 number of records
				  		if(BF_WriteBlock(header_info.fileDesc,nextBlock)<0) {								//write to block
							BF_PrintError("Could not write to block\n");
							return -1;
						}
						printf("Inserted record at block number %d\n\n",nextBlock);
						inserted = 1;
					}
				}
				else {																//there isn't an extra block for this hash value
					if(BF_AllocateBlock(header_info.fileDesc)<0) {					//create new block
			  			BF_PrintError("Could not allocate block\n");
			  			return -1;
			  		}
			  		if((newBlock=BF_GetBlockCounter(header_info.fileDesc))<0) {	//get number of newly created block
						BF_PrintError("Could not get block counter\n");
						return -1;
					}
					newBlock--;
					printf("New block has number %d\n",newBlock);
					memcpy(block+sizeof(int),&newBlock,sizeof(int));					//write to hashtable block the corresponding block for this hash value
					if(blockDepth == 1){												//check which block we have to save the next block number
						if(BF_WriteBlock(header_info.fileDesc,bucketValue)<0) {
							BF_PrintError("Could not write to block\n");
							return -1;
						}
					}
					else {
						if(BF_WriteBlock(header_info.fileDesc,lastBlock)<0) {
							BF_PrintError("Could not write to block\n");
							return -1;
						}
					}
					if(BF_ReadBlock(header_info.fileDesc,newBlock,&block)<0) {		//read the block for this hash value
						BF_PrintError("Could not read block\n");
						return -1;
					}
					numRecords = 1;
					memcpy(block,&numRecords,sizeof(int));
					nextValue = -1;
					memcpy(block+sizeof(int),&nextValue,sizeof(int));
					memcpy(block+2*sizeof(int),&record,sizeof(Record));				//copy the record after the space for the number of records in block and block number
					if(BF_WriteBlock(header_info.fileDesc,newBlock)<0) {
						BF_PrintError("Could not write to block\n");
						return -1;
					}
					printf("Inserted record at block number %d\n\n",newBlock);
					inserted=1;
				}
			}

  		}
  		else {																					//record fits in first block of hash value
	  		memcpy(block+numRecords*sizeof(Record)+2*sizeof(int),&record,sizeof(Record));		//add record at the end
	  		numRecords++;
	  		memcpy(block,&numRecords,sizeof(int));												//+1 number of records
	  		if(BF_WriteBlock(header_info.fileDesc,bucketValue)<0) {								//write to block
				BF_PrintError("Could not write to block\n");
				return -1;
			}
			printf("Inserted record at block number %d\n\n",bucketValue);
	  	}
  	}
    return 0;
}



int HT_GetAllEntries(HT_info header_info, void *value) {
	int totalRecords, foundRecords, i, end;
	int id;
	char str[25];
	unsigned int hash_value;
	Record temp_record;
    void* block;

    // Compute hash value
    if(header_info.attrType=='i' && strcmp(header_info.attrName,"id")==0) {			//use hash_function_int
    	id = *((int *) value);
    	hash_value = hash_function_int(header_info.numBuckets,id);
    }
    else if(header_info.attrType=='c') {											//use hash_function_char
    	strcpy(str,value);
    	if(strcmp(header_info.attrName,"city")==0)
    		hash_value = hash_function_char(header_info.numBuckets,str);
    	else if(strcmp(header_info.attrName,"name")==0)
    		hash_value = hash_function_char(header_info.numBuckets,str);
    	else if(strcmp(header_info.attrName,"surname")==0)
    		hash_value = hash_function_char(header_info.numBuckets,str);
    	else{
    		printf("Incorrect 'c' attrType\n");
    		return -1;
    	}
    }
    else{
    	printf("Incorrect attrType\n");
    	return -1;
    }

    // Find which block we will search first
	int maxBuckets = BLOCK_SIZE / sizeof(int);			//number of buckets each block can hold
	int hashBlock=1, numRecords, nextBlock, bucketValue;
	while(hash_value >= maxBuckets) {				//Find correct hash table block
		hashBlock++;
		hash_value -= maxBuckets;
	}
	if(BF_ReadBlock(header_info.fileDesc,hashBlock,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
  		return -1;
  	}
  	memcpy(&bucketValue,block+hash_value*sizeof(int),sizeof(int));
  	if(bucketValue == -1){
  		printf("No record with this value\n");
  		return 0;
  	}
  	if(BF_ReadBlock(header_info.fileDesc,bucketValue,&block)<0) {
		BF_PrintError("Could not read block\n");
		return -1;
	}
	totalRecords = 0;
	foundRecords = 0;
	end = 0;
	while(end == 0){
		memcpy(&numRecords,block,sizeof(int));
		for(i=0; i<numRecords; i++) {
			totalRecords++;
			memcpy(&temp_record,block+2*sizeof(int)+i*sizeof(Record),sizeof(Record));
			if(header_info.attrType=='i' && strcmp(header_info.attrName,"id")==0) {			//use hash_function_int
		    	if(temp_record.id == id) {
		    		print_record(temp_record);
		    		foundRecords++;
		    	}
		    }
		    else if(header_info.attrType == 'c') {											//use hash_function_char
		    	if(strcmp(header_info.attrName,"city") == 0 && strcmp(temp_record.city,str) == 0) {
		    		print_record(temp_record);
		    		foundRecords++;
		    	}
		    	else if(strcmp(header_info.attrName,"name") == 0 && strcmp(temp_record.name,str) == 0) {
		    		print_record(temp_record);
		    		foundRecords++;
		    	}
		    	else if(strcmp(header_info.attrName,"surname") == 0 && strcmp(temp_record.surname,str) == 0) {
		    		print_record(temp_record);
		    		foundRecords++;
		    	}
			}
		}
		memcpy(&nextBlock,block+sizeof(int),sizeof(int));
		if(nextBlock == -1)
			end = 1;
		else {
			if(BF_ReadBlock(header_info.fileDesc,nextBlock,&block)<0) {
				BF_PrintError("Could not read block\n");
				return -1;
			}
		}


	}
	printf("\nNumber of records that were read = %d\n", totalRecords);
	printf("Number of records that had the value = %d\n\n", foundRecords);
	return foundRecords;
}


int HashStatistics(char* filename) {
    int numBlocks, numBuckets, bucketsLeft, bucketBlocks, i, j, end;
    int maxBuckets = BLOCK_SIZE / sizeof(int);//number of buckets each block can hold
    int blockValue, nextBlock;
    int leastRecords, mostRecords, currRecords, blockRecords, totalRecords;
    int overflowBuckets, overflowBlocks;
    double averageBlocks, averageRecords;
    HT_info* info = HT_OpenIndex(filename);
    void* block;
	if((numBlocks=BF_GetBlockCounter(info->fileDesc))<0) {			//got number of blocks the file has
		BF_PrintError("Could not get block counter\n");
		return -1;
	}
	numBuckets = info->numBuckets;
	bucketsLeft = numBuckets;
	overflowBuckets = 0;
	leastRecords = 500;
	mostRecords = 0;
	averageRecords = 0;
	totalRecords = 0;
	for(i=1; i<info->initialBlocks; i++){						//for every block that is part of hash table
		if(i == info->initialBlocks -1){						//Might be less than max buckets that fit in a block
			for(j=0; j<bucketsLeft; j++){						//In every bucket of the block
				overflowBlocks = 0;
				bucketBlocks = 0;
				currRecords = 0;
				end = 0;
				if(BF_ReadBlock(info->fileDesc,i,&block)<0) {
					BF_PrintError("Could not read block\n");
					return -1;
				}
				memcpy(&blockValue,block+j*sizeof(int),sizeof(int));				//Get the first block corresponding to this hash value
				if(blockValue != -1){
					if(BF_ReadBlock(info->fileDesc,blockValue,&block)<0) {			//open the first block of this hash value
						BF_PrintError("Could not read block\n");
						return -1;
					}
					while(end == 0){
						bucketBlocks++;
						memcpy(&blockRecords,block,sizeof(int));
						currRecords += blockRecords;
						memcpy(&nextBlock,block+sizeof(int),sizeof(int));
						if(nextBlock == -1){
							end = 1;
							totalRecords += currRecords;
							if(currRecords > mostRecords)
								mostRecords = currRecords;
							if(currRecords < leastRecords)
								leastRecords = currRecords;
							averageRecords = (double)currRecords/(double)bucketBlocks;
							printf("Bucket with value %d has %d overflown blocks\n", ((i-1)*maxBuckets)+j,overflowBlocks);
							printf("Bucket with value %d has %d records.\n\n",((i-1)*maxBuckets)+j,currRecords);
						}
						else {
							overflowBlocks++;
							if(BF_ReadBlock(info->fileDesc,nextBlock,&block)<0) {			//open the first block of this hash value
								BF_PrintError("Could not read block\n");
								return -1;
							}
						}
					}
					if(overflowBlocks > 0)
						overflowBuckets++;
				}
				else
					printf("Bucket with value %d doesn't have any records\n\n", ((i-1)*maxBuckets)+j);
			}
		}
		else {
			for(j=0; j<maxBuckets; j++){						//Definitely has maximum buckets
				overflowBlocks = 0;
				bucketBlocks = 0;
				currRecords = 0;
				end = 0;
				if(BF_ReadBlock(info->fileDesc,i,&block)<0) {
					BF_PrintError("Could not read block\n");
					return -1;
				}
				memcpy(&blockValue,block+j*sizeof(int),sizeof(int));				//Get the first block corresponding to this hash value
				if(blockValue != -1){
					if(BF_ReadBlock(info->fileDesc,blockValue,&block)<0) {			//open the first block of this hash value
						BF_PrintError("Could not read block\n");
						return -1;
					}
					while(end == 0){
						bucketBlocks++;
						memcpy(&blockRecords,block,sizeof(int));
						currRecords += blockRecords;
						memcpy(&nextBlock,block+sizeof(int),sizeof(int));
						if(nextBlock == -1){
							end = 1;
							totalRecords += currRecords;
							if(currRecords > mostRecords)
								mostRecords = currRecords;
							if(currRecords < leastRecords)
								leastRecords = currRecords;
							averageRecords = (double)currRecords/(double)bucketBlocks;
							printf("Bucket with value %d has %d overflown blocks\n", ((i-1)*maxBuckets)+j,overflowBlocks);
							printf("Bucket with value %d has %d records.\n\n",((i-1)*maxBuckets)+j,currRecords);
						}
						else {
							overflowBlocks++;
							if(BF_ReadBlock(info->fileDesc,nextBlock,&block)<0) {			//open the first block of this hash value
								BF_PrintError("Could not read block\n");
								return -1;
							}
						}
					}
					if(overflowBlocks > 0)
						overflowBuckets++;
				}
				else
					printf("Bucket with value %d doesn't have any records\n\n", ((i-1)*maxBuckets)+j);
			}
			bucketsLeft -= maxBuckets;
		}
	}
	averageRecords = (double)totalRecords/(double)numBuckets;
	averageBlocks = (double)(numBlocks-(info->initialBlocks))/(double)numBuckets;
	printf("Total number of blocks = %d\n",numBlocks);
	printf("Least number of records in a bucket = %d\n",leastRecords);
	printf("Biggest number of records in a bucket = %d\n",mostRecords);
	printf("Average number of records in a bucket = %.03f\n",averageRecords);
	printf("Average number of blocks in a bucket = %.03f\n",averageBlocks);
	printf("Total number of buckets that have overflown = %d\n", overflowBuckets);
	return 0;
}


unsigned int hash_function_int(int hashsize,int num) {
	unsigned int hash_value;
	hash_value = num % hashsize;
	return hash_value;
}

unsigned int hash_function_char(int hashsize,char hash_name[20]) {
	unsigned long hash_value = 0;
	int i;
	while((i = *hash_name++))
		hash_value =  i + (hash_value << 6) + (hash_value << 16) - hash_value;
	hash_value = hash_value % hashsize;
	return (unsigned int)hash_value;
}

void print_record(Record rec) {
	printf("\nID=%d\nName=%s\nSurname=%s\nCity=%s\n",rec.id,rec.name,rec.surname,rec.city);
}