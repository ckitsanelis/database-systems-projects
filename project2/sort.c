#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "BF.h"
#include "sort.h"
#include "algorithms.h"


int Sorted_CreateFile(char *fileName) {
	int fileDesc;
	void *block;
	Sorted_Info* info = malloc(sizeof(Sorted_Info));
	//Create new BF file
	if(BF_CreateFile(fileName) < 0) {
  		BF_PrintError("Could not create file\n");
  		return -1;
	}
	fileDesc = BF_OpenFile(fileName);
	if(fileDesc < 0) {
		BF_PrintError("Could not open file\n");
  		return -1;
	}
	//Allocate block 0
	if(BF_AllocateBlock(fileDesc) < 0) {
  		BF_PrintError("Could not allocate block\n");
  		return -1;
  	}
  	//And pass into it the correct Sorted info struct
  	info->isSorted = 1;
  	info->fieldNo = -1;
  	info->lastBlock = 1;
  	info->recordsinLastBlock = 0;
  	if(BF_ReadBlock(fileDesc,0,&block) < 0) {
 	 	BF_PrintError("Could not read block\n");
  		return -1;
  	}
	memcpy(block,info,sizeof(Sorted_Info));
	if(BF_WriteBlock(fileDesc,0)) {
		BF_PrintError("Could not write to block\n");
  		return -1;
	}
	if(BF_AllocateBlock(fileDesc) < 0) {
  		BF_PrintError("Could not allocate block\n");
  		return -1;
  	}
	free(info);
	printf("Created Sorted File with filedesc %d\n",fileDesc);
	return 0;
}

int Sorted_OpenFile(char *fileName) {
	int fileDesc;
	void* block;
	Sorted_Info info; 
	fileDesc = BF_OpenFile(fileName);
	if(fileDesc < 0) {
		BF_PrintError("Could not open file\n");
  		return -1;
	}
	if(BF_ReadBlock(fileDesc,0,&block) < 0) {
 	 	BF_PrintError("Could not read block\n");
  		return -1;
  	}
  	memcpy(&info,block,sizeof(Sorted_Info));
  	//Check if file is sorted
  	if(info.isSorted != 1)
  		return -1;
	return fileDesc;
}

int Sorted_CloseFile(int fileDesc) {
	if(BF_CloseFile(fileDesc) < 0) {
  		BF_PrintError("Could not close file\n");
  		return -1;
  	}
  //printf("Closed the Sorted File\n");
	return 0;
}

//Use this to insert random records to a file(like heap)
int insertEntry(int fileDesc,Record Record) {
	int maxRecords = BLOCK_SIZE/sizeof(Record);
	void* block;
	Sorted_Info info;
	//Update block 0 of file, that the file is not sorted anymore
	if(BF_ReadBlock(fileDesc,0,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
  		return -1;
  	}
  	memcpy(&info,block,sizeof(Sorted_Info));
  	info.isSorted = 0;
  	info.fieldNo = -1;
  	//Check if new record fits in last block of file
	if(info.recordsinLastBlock < maxRecords) {
		int tempBlock = info.lastBlock;
		int tempRecord = info.recordsinLastBlock;
		info.recordsinLastBlock++;
		//Update block 0 of the extra record in the last block of the file
		memcpy(block,&info,sizeof(Sorted_Info));
		if(BF_WriteBlock(fileDesc,0)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
		if(BF_ReadBlock(fileDesc,tempBlock,&block)<0) {
 	 		BF_PrintError("Could not read block\n");
  			return -1;
  		}
  		//Insert record in the correct place of the last block
  		memcpy(block+tempRecord*sizeof(Record),&Record,sizeof(Record));
  		if(BF_WriteBlock(fileDesc,tempBlock)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
		printf("Inserted record in block %d that now has %d records\n",tempBlock,++tempRecord);
	}
	else {
		//Update block 0 that the new record will be inserted in a new block
		info.lastBlock++;
		info.recordsinLastBlock = 1;
		int tempBlock = info.lastBlock;
		memcpy(block,&info,sizeof(Sorted_Info));
		if(BF_WriteBlock(fileDesc,0)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
		//Allocate the new block
		if(BF_AllocateBlock(fileDesc) < 0) {
  			BF_PrintError("Could not allocate block\n");
  			return -1;
  		}
  		if(BF_ReadBlock(fileDesc,tempBlock,&block)<0) {
 	 		BF_PrintError("Could not read block\n");
  			return -1;
  		}
  		//And place the record in the beginning of it
  		memcpy(block,&Record,sizeof(Record));
  		if(BF_WriteBlock(fileDesc,tempBlock)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
		printf("Inserted record in new block %d\n",tempBlock);
	}
	return 0;
}

//Works the same way as plain insertEntry but it doesn't have to update the block 0 of file, since it stays sorted
int Sorted_InsertEntry(int fileDesc,Record record) {
	int maxRecords = BLOCK_SIZE/sizeof(Record);
	void* block;
	Sorted_Info info; 
	if(BF_ReadBlock(fileDesc,0,&block)<0) {
 	 	BF_PrintError("Could not read block\n");
  	return -1;
  }
  memcpy(&info,block,sizeof(Sorted_Info));
  if(info.recordsinLastBlock < maxRecords) {
		int tempBlock = info.lastBlock;
		int tempRecord = info.recordsinLastBlock;
		info.recordsinLastBlock++;
		memcpy(block,&info,sizeof(Sorted_Info));
		if(BF_WriteBlock(fileDesc,0)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
		if(BF_ReadBlock(fileDesc,tempBlock,&block)<0) {
 	 		BF_PrintError("Could not read block\n");
  			return -1;
  		}
  	memcpy(block+tempRecord*sizeof(Record),&record,sizeof(Record));
  	if(BF_WriteBlock(fileDesc,tempBlock)) {
			BF_PrintError("Could not write to block\n");
  		return -1;
		}
	}
	else {
		info.lastBlock++;
		info.recordsinLastBlock = 1;
		int tempBlock = info.lastBlock;
		memcpy(block,&info,sizeof(Sorted_Info));
		if(BF_WriteBlock(fileDesc,0)) {
			BF_PrintError("Could not write to block\n");
  			return -1;
		}
		if(BF_AllocateBlock(fileDesc) < 0) {
  		BF_PrintError("Could not allocate block\n");
  		return -1;
  	}
  	if(BF_ReadBlock(fileDesc,tempBlock,&block) < 0) {
 	 		BF_PrintError("Could not read block\n");
  		return -1;
  	}
  	memcpy(block,&record,sizeof(Record));
  	if(BF_WriteBlock(fileDesc,tempBlock)) {
			BF_PrintError("Could not write to block\n");
  		return -1;
		}
	}
	return 0;
}

void Sorted_SortFile(char *fileName,int fieldNo) {
	int fileDesc, blocks, recordsinLastBlock, perasmata = 0, i = 0;
	char finalfile[strlen(fileName)+8],intString[2];
	void *block;
	Sorted_Info finfo;
	strcpy(finalfile,fileName);
	strcat(finalfile,"Sorted");
	sprintf(intString,"%d",fieldNo);
	strcat(finalfile,intString);
	fileDesc = BF_OpenFile(fileName);
	if(fileDesc < 0) {
		BF_PrintError("Could not open file\n");
  		return;
	}
	//Get number of blocks and records in the file
	if(BF_ReadBlock(fileDesc,0,&block) < 0) {
 	 	BF_PrintError("Could not read block\n");
  		return;
  	}
  	memcpy(&finfo,block,sizeof(Sorted_Info));
  	blocks = finfo.lastBlock;
  	recordsinLastBlock = finfo.recordsinLastBlock;
  	//Use bubble sort to sort each block and save it in a different file
  	Bubble_Sort(fileDesc,fileName,fieldNo,blocks,recordsinLastBlock);
  	Sorted_CloseFile(fileDesc);
  	//Compute how many times we will need to use merge sort
  	while(i < blocks) {
  		perasmata++;
  		i = pow(2,perasmata);
  	}
  	printf("We will sort the file in %d stages\n\n", perasmata);
  	int fd1, fd2, fd3, oldfile, newfile, both;
  	i = 1;
  	while(i < perasmata) {
  		printf("Stage %d\n\n", i);
  		oldfile = 0;
  		newfile = 0;
  		while(oldfile < blocks) {
  			//Creating the name of the files to be read
  			both = 1;
  			char temp1[strlen(fileName)+8];
  			char temp2[strlen(fileName)+8];
  			strcpy(temp1,fileName);
  			strcpy(temp2,fileName);
  			sprintf(intString,"%d",i-1);
  			strcat(temp1,intString);
  			strcat(temp2,intString);
  			sprintf(intString,"%d",oldfile);
  			strcat(temp1,intString);
  			fd1 = BF_OpenFile(temp1);
  			oldfile++;
  			if(oldfile < blocks) {
  				sprintf(intString,"%d",oldfile);
  				strcat(temp2,intString);
  				fd2 = BF_OpenFile(temp2);
  				oldfile++;
  			}
  			else {
  				strcpy(temp2,"NO_FILE");
  				fd2 = -5;
  				both = 0;
  			}
  			//Creating the name of the file to write in
  			char temp3[strlen(fileName)+8];
  			strcpy(temp3,fileName);
  			sprintf(intString,"%d",i);
  			strcat(temp3,intString);
  			sprintf(intString,"%d",newfile);
  			strcat(temp3,intString);
  			BF_CreateFile(temp3);
  			fd3 = BF_OpenFile(temp3);
  			newfile++;
  			printf("Reading files: %s , %s\nWe will receive: %s\n",temp1,temp2,temp3);
  			//Merge sort the records from the files with filedescriptors fd1 and fd2 and write the results in the file fd3
  			Merge_Sort(fd1,fd2,fd3);
  			//We can not remove the 2 previous temp files
  			remove(temp1);
  			printf("Deleting file: %s\n", temp1);
  			if(both == 1) {
  				remove(temp2);
  				printf("Deleting file: %s\n", temp2);
  			}
  		}
  		blocks = newfile;
  		i++;
  	}


  	//Last merge sort will happen here to write the result in the special file name
  	char temp1[strlen(fileName)+8];
  	char temp2[strlen(fileName)+8];
  	strcpy(temp1,fileName);
  	strcpy(temp2,fileName);
  	sprintf(intString,"%d",i-1);
  	strcat(temp1,intString);
  	strcat(temp1,"0");
  	fd1 = BF_OpenFile(temp1);
  	strcat(temp2,intString);
  	strcat(temp2,"1");
  	fd2 = BF_OpenFile(temp2);
  	if(BF_CreateFile(finalfile) < 0) {
 	 	BF_PrintError("Could not create file\n");
  		return;
  	}
  	fd3 = BF_OpenFile(finalfile);
  	if(fd3 < 0) {
 	 	BF_PrintError("Could not open file\n");
  		return;
  	}
  	Merge_Sort(fd1,fd2,fd3);
  	remove(temp1);
  	remove(temp2);
  	printf("Deleting file: %s\n", temp1);
  	printf("Deleting file: %s\n", temp2);

    printf("\nCreated sorted file: %s\n\n",finalfile);
}


int Sorted_checkSortedFile(char *fileName,	int fieldNo) {
	int fileDesc;
	void *block;
	Sorted_Info info; 
	fileDesc = BF_OpenFile(fileName);
	if(fileDesc < 0) {
		BF_PrintError("Could not open file\n");
  		return -1;
	}
	//Open block 0 of the file
	if(BF_ReadBlock(fileDesc,0,&block) < 0) {
 	 	printf("File with name %s is NOT Sorted\n", fileName);
  		return -1;
  	}
  	memcpy(&info,block,sizeof(Sorted_Info));
  	//And check if it is sorted with the correct fieldNo
  	if(info.isSorted == 1 && info.fieldNo == fieldNo) {
  		printf("\nFile with name %s is Sorted\n\n", fileName);
  		Sorted_CloseFile(fileDesc);
		return 0;
	}
	else {
		printf("\nFile with name %s is NOT Sorted\n\n", fileName);
		Sorted_CloseFile(fileDesc);
		return -1;
	}
}

void Sorted_GetAllEntries(int fileDesc,int* fieldNo,void *value) {
	int i;
	int current_block;
	int lastBlock;
	int recordsinLastBlock;
	int foundRecords = 0;
	int maxRecords = BLOCK_SIZE/sizeof(Record);
	Record tempRecord;
	void* block;
	//Get the data from block 0 of the file
	if(BF_ReadBlock(fileDesc,0,&block) < 0) {
 	 	BF_PrintError("Could not read block\n");
  		return;
  	}
  	memcpy(&lastBlock,block+2*sizeof(int),sizeof(int));
  	memcpy(&recordsinLastBlock,block+3*sizeof(int),sizeof(int));
  	// Print all records
	if(value == NULL) {
    printf("________________________________________________________________\nShowing all records\n");
		current_block = 1;
		//Run through all blocks
		while(current_block <= lastBlock) {
			if(BF_ReadBlock(fileDesc,current_block,&block) < 0) {
 	 			BF_PrintError("Could not read block\n");
  				return;
  			}
  			//Get all records from the block and print them
  			if(current_block != lastBlock) {
  				for(i=0 ; i < maxRecords ; i++) {
  					memcpy(&tempRecord,block+i*sizeof(Record),sizeof(Record));
  					print_record(tempRecord);
  					foundRecords++;
  				}
  			}
  			//Last block might not be full with records
  			else {
  				for(i=0 ; i < recordsinLastBlock ; i++) {
  					memcpy(&tempRecord,block+i*sizeof(Record),sizeof(Record));
  					print_record(tempRecord);
  					foundRecords++;
  				}
  			}
  			current_block++;
  		}
  		printf("\nFound %d records in all %d blocks\n\n",foundRecords,lastBlock);
	}
	else {
		int totalRecords, l;
		//This array is used to keep track of the distinct blocks we have checked
		int blocksArray[(lastBlock/2)+1];
		for(l = 0; l <= (lastBlock/2)+1; l++)
			blocksArray[l] = 0;
		totalRecords = ((lastBlock - 1) * maxRecords) + recordsinLastBlock;
    printf("________________________________________________________________\nStarting binary search\n\n");
		binary_search(0,totalRecords,&foundRecords,fileDesc,*fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
		printf("\nChecked %d blocks and found %d records\n\n",array_size(blocksArray),foundRecords);
	}
}

//Add unique elements to array
void add_to_array(int blocksArray[], int element) {
	int i = 0;
	while(blocksArray[i] != 0) {
		if(blocksArray[i] == element)
			break;
		i++;
	}
	blocksArray[i] = element;
}

//Get the number of elements in the array
int array_size(int blocksArray[]) {
	int i = 0;
	while(blocksArray[i] != 0)
		i++;
	return i;
}

void print_record(Record rec) {
	printf("\nID : %d\nName : %s\nSurname : %s\nCity : %s\n\n",rec.id,rec.name,rec.surname,rec.city);
}
