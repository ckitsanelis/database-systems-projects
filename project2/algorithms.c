#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BF.h"
#include "sort.h"

void Merge_Sort(int fd1, int fd2, int fd3) {
	//Only have to copy one file to the next stage
	if(fd2 == -5) {
		int totalBlocks, i;
		void *block, *newblock;
		//Read how many blocks this file has
		if(BF_ReadBlock(fd1,0,&block) < 0) {
 	 		BF_PrintError("Could not read block\n");
  			return;
  		}
  		memcpy(&totalBlocks,block + 2*sizeof(int),sizeof(int));
  		//Copy each block to the new file
  		for(i = 0; i <= totalBlocks; i++) {
  			if(BF_ReadBlock(fd1,i,&block) < 0) {
 	 			BF_PrintError("Could not read block\n");
  				return;
  			}
  			if(BF_AllocateBlock(fd3) < 0) {
  				BF_PrintError("Could not allocate block\n");
  				return;
  			}
  			if(BF_ReadBlock(fd3,i,&newblock) < 0) {
 	 			BF_PrintError("Could not read block\n");
  				return;
  			}
  			memcpy(newblock,block,BLOCK_SIZE);
  			if(BF_WriteBlock(fd3,i)) {
  				BF_PrintError("Could not write block\n");
  				return;
  			}
  		}
  		Sorted_CloseFile(fd1);
  		Sorted_CloseFile(fd3);
	}
	else {
		int maxRecords1 = BLOCK_SIZE / sizeof(Record);
		int maxRecords2 = maxRecords1;
		int file1Blocks, file2Blocks, file1LastRecords, file2LastRecords;
		int current1Block, current2Block, current1Record, current2Record;
		int finished = 0, changeBlock1 = 1, changeBlock2 = 1, fieldNo;
		void *block1, *block2, *newblock;
		Sorted_Info info1, info2, newinfo;
		Record record1, record2;
		//Get number of blocks and records from each file
		if(BF_ReadBlock(fd1,0,&block1) < 0) {
 	 		BF_PrintError("Could not read block\n");
  			return;
  		}
  		memcpy(&info1,block1,sizeof(Sorted_Info));
  		file1Blocks = info1.lastBlock;
  		file1LastRecords =info1.recordsinLastBlock;
  		if(BF_ReadBlock(fd2,0,&block2) < 0) {
 	 		BF_PrintError("Could not read block\n");
  			return;
  		}
  		memcpy(&info2,block2,sizeof(Sorted_Info));
  		file2Blocks = info2.lastBlock;
  		file2LastRecords = info2.recordsinLastBlock;
  		fieldNo = info2.fieldNo;
  		//Compute number of blocks and records the new file will have
  		int totalRecords, newLastBlock = 0, tempRecords = 0;
  		totalRecords = (file1Blocks + file2Blocks - 2) * maxRecords1 + file1LastRecords + file2LastRecords;
  		while(tempRecords < totalRecords) {
  			newLastBlock++;
  			tempRecords += 8;
  		}
  		//Insert the data in the first block of the new file
  		if(BF_AllocateBlock(fd3) < 0) {
  			BF_PrintError("Could not allocate block\n");
  			return;
  		}
  		if(BF_ReadBlock(fd3,0,&newblock) < 0) {
 	 		BF_PrintError("Could not read block\n");
  			return;
  		}
  		newinfo.isSorted = 1;
  		newinfo.fieldNo = fieldNo;
  		newinfo.lastBlock = 1;
  		newinfo.recordsinLastBlock = 0;
  		memcpy(newblock,&newinfo,sizeof(Sorted_Info));
  		if(BF_WriteBlock(fd3,0)) {
  			BF_PrintError("Could not write block\n");
  			return;
  		}
  		if(BF_AllocateBlock(fd3) < 0) {
  			BF_PrintError("Could not allocate block\n");
  			return;
  		}
  		//Go to the first record of each file
  		current1Block = 1;
  		current2Block = 1;
  		current1Record = 0;
  		current2Record = 0;
  		if(fieldNo == 0) {
  			//Until both files are fully read
	  		while(finished == 0) {
	  			if(changeBlock1 == 1) {
	  				if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock1 = 0;
	  			}
	  			if(changeBlock2 == 1) {
	  				if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock2 = 0;
	  			}
	  			//Compare the 2 records
	  			memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  			memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  			printf("\n\nComparing these records: \n");print_record(record1);print_record(record2);
	  			//Insert from first file
	  			if(record1.id < record2.id) {
	  				printf("Choosing the first one\n");
	  				Sorted_InsertEntry(fd3,record1);
	  				if(current1Record < maxRecords1 - 1)				//Dont need to change block, just go to next record in same block
	  					current1Record++;
	  				else {
	  					if(current1Block == file1Blocks) {				//Finished reading all records from first file
	  						//We have to copy all remaining records from the second file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record2);
	  							Sorted_InsertEntry(fd3,record2);
	  							if(current2Record < maxRecords2 -1) {
	  								current2Record++;
	  								memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current2Block++;
	  								if(current2Block > file2Blocks)
	  									finished = 1;
	  								else {
	  									current2Record = 0;
	  									if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current2Block == file2Blocks)
	  										maxRecords2 = file2LastRecords;
	  									memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current1Record = 0;
	  						current1Block++;
	  						changeBlock1 = 1;
	  						if(current1Block == file1Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords1 = file1LastRecords;
	  					}
	  				}
	  			}
	  			//Insert from second file
	  			else {
	  				printf("Choosing the second one\n");
	  				Sorted_InsertEntry(fd3,record2);
	  				if(current2Record < maxRecords2 - 1)				//Dont need to change block, just go to next record in same block
	  					current2Record++;
	  				else {
	  					if(current2Block == file2Blocks) {				//Finished reading all records from second file
	  						//We have to copy all remaining records from the first file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record1);
	  							Sorted_InsertEntry(fd3,record1);
	  							if(current1Record < maxRecords1 -1) {
	  								current1Record++;
	  								memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current1Block++;
	  								if(current1Block > file1Blocks)
	  									finished = 1;
	  								else {
	  									current1Record = 0;
	  									if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current1Block == file1Blocks)
	  										maxRecords1 = file1LastRecords;
	  									memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current2Record = 0;
	  						current2Block++;
	  						changeBlock2 = 1;
	  						if(current2Block == file2Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords2 = file2LastRecords;
	  					}
	  				}
	  			}
	  		}
	  	}
		if(fieldNo == 1) {
  			//Until both files are fully read
	  		while(finished == 0) {
	  			if(changeBlock1 == 1) {
	  				if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock1 = 0;
	  			}
	  			if(changeBlock2 == 1) {
	  				if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock2 = 0;
	  			}
	  			//Compare the 2 records
	  			memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  			memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  			printf("\n\nComparing these records: \n");print_record(record1);print_record(record2);
	  			//Insert from first file
				if(strcmp(record1.name,record2.name) < 0) {
					printf("Choosing the first one\n");
	  				Sorted_InsertEntry(fd3,record1);
	  				if(current1Record < maxRecords1 - 1)				//Dont need to change block, just go to next record in same block
	  					current1Record++;
	  				else {
	  					if(current1Block == file1Blocks) {				//Finished reading all records from first file
	  						//We have to copy all remaining records from the second file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record2);
	  							Sorted_InsertEntry(fd3,record2);
	  							if(current2Record < maxRecords2 -1) {
	  								current2Record++;
	  								memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current2Block++;
	  								if(current2Block > file2Blocks)
	  									finished = 1;
	  								else {
	  									current2Record = 0;
	  									if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current2Block == file2Blocks)
	  										maxRecords2 = file2LastRecords;
	  									memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current1Record = 0;
	  						current1Block++;
	  						changeBlock1 = 1;
	  						if(current1Block == file1Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords1 = file1LastRecords;
	  					}
	  				}
	  			}
	  			//Insert from second file
	  			else {
	  				printf("Choosing the second one\n");
	  				Sorted_InsertEntry(fd3,record2);
	  				if(current2Record < maxRecords2 - 1)				//Dont need to change block, just go to next record in same block
	  					current2Record++;
	  				else {
	  					if(current2Block == file2Blocks) {				//Finished reading all records from second file
	  						//We have to copy all remaining records from the first file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record1);
	  							Sorted_InsertEntry(fd3,record1);
	  							if(current1Record < maxRecords1 -1) {
	  								current1Record++;
	  								memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current1Block++;
	  								if(current1Block > file1Blocks)
	  									finished = 1;
	  								else {
	  									current1Record = 0;
	  									if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current1Block == file1Blocks)
	  										maxRecords1 = file1LastRecords;
	  									memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current2Record = 0;
	  						current2Block++;
	  						changeBlock2 = 1;
	  						if(current2Block == file2Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords2 = file2LastRecords;
	  					}
	  				}
	  			}
	  		}
	  	}
		if(fieldNo == 2) {
  			//Until both files are fully read
	  		while(finished == 0) {
	  			if(changeBlock1 == 1) {
	  				if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock1 = 0;
	  			}
	  			if(changeBlock2 == 1) {
	  				if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock2 = 0;
	  			}
	  			//Compare the 2 records
	  			memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  			memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  			printf("\n\nComparing these records: \n");print_record(record1);print_record(record2);
	  			//Insert from first file
	  			if(strcmp(record1.surname,record2.surname) < 0) {
	  				printf("Choosing the first one\n");
	  				Sorted_InsertEntry(fd3,record1);
	  				if(current1Record < maxRecords1 - 1)				//Dont need to change block, just go to next record in same block
	  					current1Record++;
	  				else {
	  					if(current1Block == file1Blocks) {				//Finished reading all records from first file
	  						//We have to copy all remaining records from the second file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record2);
	  							Sorted_InsertEntry(fd3,record2);
	  							if(current2Record < maxRecords2 -1) {
	  								current2Record++;
	  								memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current2Block++;
	  								if(current2Block > file2Blocks)
	  									finished = 1;
	  								else {
	  									current2Record = 0;
	  									if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current2Block == file2Blocks)
	  										maxRecords2 = file2LastRecords;
	  									memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current1Record = 0;
	  						current1Block++;
	  						changeBlock1 = 1;
	  						if(current1Block == file1Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords1 = file1LastRecords;
	  					}
	  				}
	  			}
	  			//Insert from second file
	  			else {
	  				printf("Choosing the second one\n");
	  				Sorted_InsertEntry(fd3,record2);
	  				if(current2Record < maxRecords2 - 1)				//Dont need to change block, just go to next record in same block
	  					current2Record++;
	  				else {
	  					if(current2Block == file2Blocks) {				//Finished reading all records from second file
	  						//We have to copy all remaining records from the first file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record1);
	  							Sorted_InsertEntry(fd3,record1);
	  							if(current1Record < maxRecords1 -1) {
	  								current1Record++;
	  								memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current1Block++;
	  								if(current1Block > file1Blocks)
	  									finished = 1;
	  								else {
	  									current1Record = 0;
	  									if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current1Block == file1Blocks)
	  										maxRecords1 = file1LastRecords;
	  									memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current2Record = 0;
	  						current2Block++;
	  						changeBlock2 = 1;
	  						if(current2Block == file2Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords2 = file2LastRecords;
	  					}
	  				}
	  			}
	  		}
	  	}
		if(fieldNo == 3) {
  			//Until both files are fully read
	  		while(finished == 0) {
	  			if(changeBlock1 == 1) {
	  				if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock1 = 0;
	  			}
	  			if(changeBlock2 == 1) {
	  				if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 				BF_PrintError("Could not read block\n");
	  					return;
	  				}
	  				changeBlock2 = 0;
	  			}
	  			//Compare the 2 records
	  			memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  			memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  			printf("\n\nComparing these records: \n");print_record(record1);print_record(record2);
	  			//Insert from first file
	  			if(strcmp(record1.city,record2.city) < 0) {
	  				printf("Choosing the first one\n");
	  				Sorted_InsertEntry(fd3,record1);
	  				if(current1Record < maxRecords1 - 1)				//Dont need to change block, just go to next record in same block
	  					current1Record++;
	  				else {
	  					if(current1Block == file1Blocks) {				//Finished reading all records from first file
	  						//We have to copy all remaining records from the second file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record2);
	  							Sorted_InsertEntry(fd3,record2);
	  							if(current2Record < maxRecords2 -1) {
	  								current2Record++;
	  								memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current2Block++;
	  								if(current2Block > file2Blocks)
	  									finished = 1;
	  								else {
	  									current2Record = 0;
	  									if(BF_ReadBlock(fd2,current2Block,&block2) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current2Block == file2Blocks)
	  										maxRecords2 = file2LastRecords;
	  									memcpy(&record2,block2 + current2Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current1Record = 0;
	  						current1Block++;
	  						changeBlock1 = 1;
	  						if(current1Block == file1Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords1 = file1LastRecords;
	  					}
	  				}
	  			}
	  			//Insert from second file
	  			else {
	  				printf("Choosing the second one\n");
	  				Sorted_InsertEntry(fd3,record2);
	  				if(current2Record < maxRecords2 - 1)				//Dont need to change block, just go to next record in same block
	  					current2Record++;
	  				else {
	  					if(current2Block == file2Blocks) {				//Finished reading all records from second file
	  						//We have to copy all remaining records from the first file
	  						while(finished == 0) {
	  							printf("Leftover record\n");print_record(record1);
	  							Sorted_InsertEntry(fd3,record1);
	  							if(current1Record < maxRecords1 -1) {
	  								current1Record++;
	  								memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  							}
	  							else {
	  								current1Block++;
	  								if(current1Block > file1Blocks)
	  									finished = 1;
	  								else {
	  									current1Record = 0;
	  									if(BF_ReadBlock(fd1,current1Block,&block1) < 0) {
	 	 									BF_PrintError("Could not read block\n");
	  										return;
	  									}
	  									if(current1Block == file1Blocks)
	  										maxRecords1 = file1LastRecords;
	  									memcpy(&record1,block1 + current1Record*sizeof(Record),sizeof(Record));
	  								}
	  							}
	  						}
	  					}
	  					//There are still other blocks in the file
	  					else {
	  						current2Record = 0;
	  						current2Block++;
	  						changeBlock2 = 1;
	  						if(current2Block == file2Blocks)			//Last block in the file might have less than maximum records that fit in a block
	  							maxRecords2 = file2LastRecords;
	  					}
	  				}
	  			}
	  		}
	  	}

		Sorted_CloseFile(fd1);
  		Sorted_CloseFile(fd2);
  		Sorted_CloseFile(fd3);
	}
}


void Bubble_Sort(int fileDesc, char *fileName, int fieldNo, int initialBlocks, int recordsinLastBlock) {
	int i, j, k, newFileDesc;
	int maxRecords = BLOCK_SIZE / sizeof(Record);
	void *oldblock, *newblock;
	char intString[2];
	char tempfile[strlen(fileName)+1];
	char newfile[strlen(tempfile)+1];
	Record r1, r2 ,tempRecord;
	Sorted_Info finfo;
	strcpy(tempfile,fileName);
	strcat(tempfile,"0");
	//Have to run through all blocks in original file
	for(i = 0 ; i < initialBlocks ; i++) {
		if(i == initialBlocks - 1)
			maxRecords = recordsinLastBlock;
		if(BF_ReadBlock(fileDesc,i+1,&oldblock) < 0) {
  			BF_PrintError("Could not read block\n");
  			return;
  		}
  		//Create the name for the new file
		strcpy(newfile,tempfile);
		sprintf(intString,"%d",i);
		strcat(newfile,intString);
		if(BF_CreateFile(newfile) < 0) {
 	 		BF_PrintError("Could not create file\n");
  			return;
  		}
  		newFileDesc = BF_OpenFile(newfile);
  		if(newFileDesc < 0) {
  			BF_PrintError("Could not open file\n");
  			return;
  		}
  		if(BF_AllocateBlock(newFileDesc) < 0) {
  			BF_PrintError("Could not allocate block\n");
  			return;
  		}
  		//Insert data in block 0 of the new file
  		if(BF_AllocateBlock(newFileDesc) < 0) {
  			BF_PrintError("Could not allocate block\n");
  			return;
  		}
  		if(BF_ReadBlock(newFileDesc,0,&newblock) < 0) {
  			BF_PrintError("Could not read block\n");
  			return;
  		}
  		finfo.isSorted = 1;
  		finfo.fieldNo = fieldNo;
  		finfo.lastBlock = 1;
  		finfo.recordsinLastBlock = maxRecords;
  		memcpy(newblock,&finfo,sizeof(Sorted_Info));
  		if(BF_WriteBlock(newFileDesc,0)) {
  			BF_PrintError("Could not write block\n");
  			return;
  		}
  		if(BF_ReadBlock(newFileDesc,1,&newblock) < 0) {
  			BF_PrintError("Could not read block\n");
  			return;
  		}
  		memcpy(newblock,oldblock,BLOCK_SIZE);
  		//Take 2 records at a time and check if they are in correct order
  		//If they aren't, swap the records
  		if(fieldNo == 0) {
  			for(j = 0 ; j < maxRecords-1 ; j++) {
  				int swapped = 0;
  				for(k = 0 ; k < maxRecords-1-j; k++) {
  					memcpy(&r1,newblock + k * sizeof(Record),sizeof(Record));
  					memcpy(&r2,newblock + (k+1) * sizeof(Record),sizeof(Record));
  					if(r1.id > r2.id) {
  						memcpy(&tempRecord,&r1,sizeof(Record));
  						memcpy(newblock + k * sizeof(Record),&r2,sizeof(Record));
  						memcpy(newblock + (k+1) * sizeof(Record),&tempRecord,sizeof(Record));
  						swapped = 1;
  					}
  				}

  				if(swapped == 0)
  					break;
  			}
  		}
  		if(fieldNo == 1) {
  			for(j = 0 ; j < maxRecords-1 ; j++) {
  				int swapped = 0;
  				for(k = 0 ; k < maxRecords-1-j; k++) {
  					memcpy(&r1,newblock + k * sizeof(Record),sizeof(Record));
  					memcpy(&r2,newblock + (k+1) * sizeof(Record),sizeof(Record));
  					if(strcmp(r1.name,r2.name) > 0) {
  						memcpy(&tempRecord,&r1,sizeof(Record));
  						memcpy(newblock + k * sizeof(Record),&r2,sizeof(Record));
  						memcpy(newblock + (k+1) * sizeof(Record),&tempRecord,sizeof(Record));
  						swapped = 1;
  					}
  				}

  				if(swapped == 0)
  					break;
  			}
  		}
  		if(fieldNo == 2) {
  			for(j = 0 ; j < maxRecords-1 ; j++) {
  				int swapped = 0;
  				for(k = 0 ; k < maxRecords-1-j; k++) {
  					memcpy(&r1,newblock + k * sizeof(Record),sizeof(Record));
  					memcpy(&r2,newblock + (k+1) * sizeof(Record),sizeof(Record));
  					if(strcmp(r1.surname,r2.surname) > 0) {
  						memcpy(&tempRecord,&r1,sizeof(Record));
  						memcpy(newblock + k * sizeof(Record),&r2,sizeof(Record));
  						memcpy(newblock + (k+1) * sizeof(Record),&tempRecord,sizeof(Record));
  						swapped = 1;
  					}
  				}

  				if(swapped == 0)
  					break;

  			}
  		}
  		if(fieldNo == 3) {
  			for(j = 0 ; j < maxRecords-1 ; j++) {
  				int swapped = 0;
  				for(k = 0 ; k < maxRecords-1-j; k++) {
  					memcpy(&r1,newblock + k * sizeof(Record),sizeof(Record));
  					memcpy(&r2,newblock + (k+1) * sizeof(Record),sizeof(Record));
  					if(strcmp(r1.city,r2.city) > 0) {
  						memcpy(&tempRecord,&r1,sizeof(Record));
  						memcpy(newblock + k * sizeof(Record),&r2,sizeof(Record));
  						memcpy(newblock + (k+1) * sizeof(Record),&tempRecord,sizeof(Record));
  						swapped = 1;
  					}
  				}

  				if(swapped == 0)
  					break;

  			}
  		}
  		if(BF_WriteBlock(newFileDesc,1)) {
  			BF_PrintError("Could not write block\n");
  			return;
  		}
  		Sorted_CloseFile(newFileDesc);
	}
}


void binary_search(int firstRecord, int lastRecord, int* foundRecords, int fileDesc, int fieldNo, void* value, int lastBlock, int recordsinLastBlock, int maxRecords, int blocksArray[]) {
	if(firstRecord > lastRecord)
		return;
	int midRecord;
	void* block;
	int searchBlock = 0;
	int recordCounter = 0;
	int recordInBlock;
	Record tempRecord;
	// Find block that has the midRecord
	midRecord = (lastRecord + firstRecord) / 2;
	while(recordCounter < midRecord) {
		recordCounter = recordCounter + maxRecords;
		searchBlock++;
	}
	//Find the record's spot in the block
	recordInBlock = midRecord - ((searchBlock - 1) * maxRecords ) - 1;
	if(BF_ReadBlock(fileDesc,searchBlock,&block) < 0) {
 	 	BF_PrintError("Could not read block\n");
  		return;
  	}
  	//Add the number of the block to the array if it isn't already in
  	add_to_array(blocksArray,searchBlock);
  	printf("Checking block %d record number %d\n", searchBlock,midRecord);
  	if(fieldNo == 0) {
  		int idValue = *((int *) value);
  		int leftShift = 0, rightShift = 0;
  		memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  		//Compare the value we are searching for with the one in the middle record
  		if(idValue < tempRecord.id)
  			binary_search(firstRecord,midRecord - 1,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else if (idValue > tempRecord.id)
  			binary_search(midRecord + 1,lastRecord,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else {
  			//Found a record with the value we are looking for
  			print_record(tempRecord);
  			(*foundRecords)++;
  			int tempRecordInBlock = recordInBlock;
  			int k = 1;
  			int leftEnd = 0;
  			int rightEnd = 0;
  			//Check the smaller and bigger records for the posibility of more records with same value
  			while(leftEnd == 0) {
  				//Check smaller records in the same block we are in
  				if(recordInBlock > 0) {
  					recordInBlock--;
  					memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  					if(tempRecord.id == idValue) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						leftEnd = 1;
  				}
  				else {
  					//Have to change to smaller block if it is possible
  					if((searchBlock - k) != 0) {
  						add_to_array(blocksArray,searchBlock - k);
  						recordInBlock = 7;
  						//Read the block
  						if(BF_ReadBlock(fileDesc,searchBlock - k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional left block %d\n", searchBlock - k);
  						leftShift = 1;
  						k++;
  						//And check the last record in this block
  						memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  						if(tempRecord.id == idValue) {
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							leftEnd = 1;
  					}
  					else
  						leftEnd = 1;
  				}
  			}
  			if(leftShift == 1) {
  				if(BF_ReadBlock(fileDesc,searchBlock,&block) < 0) {
 	 				BF_PrintError("Could not read block\n");
  					return;
  				}
  			}
  			//Same operation for records and blocks that are bigger
  			k = 1;
  			while(rightEnd == 0) {
  				if(tempRecordInBlock < 7) {
  					tempRecordInBlock++;
  					if(rightShift == 1 && searchBlock + k - 1 == lastBlock && tempRecordInBlock == recordsinLastBlock)
  						break;
  					memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  					if(tempRecord.id == idValue) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						rightEnd = 1;
  				}
  				else {
  					if((searchBlock + k) != lastBlock + 1) {
  						add_to_array(blocksArray,searchBlock + k);
  						tempRecordInBlock = 0;
  						if(BF_ReadBlock(fileDesc,searchBlock + k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional right block %d\n", searchBlock + k);
  						rightShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  						if(tempRecord.id == idValue) {
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							rightEnd = 1;
  					}
  					else
  						rightEnd = 1;
  				}
  			}
  		}
  	}
  	else if(fieldNo == 1) {
  		int leftShift = 0, rightShift = 0;
  		char strValue[20];
  		strcpy(strValue,value);
  		memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  		if(strcmp(tempRecord.name,strValue) > 0)
  			binary_search(firstRecord,midRecord - 1,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else if(strcmp(tempRecord.name,strValue) < 0)
  			binary_search(midRecord + 1,lastRecord,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else {
  			print_record(tempRecord);
  			(*foundRecords)++;
  			int tempRecordInBlock = recordInBlock;
  			int k = 1;
  			int leftEnd = 0;
  			int rightEnd = 0;
  			while(leftEnd == 0) {
  				if(recordInBlock > 0) {
  					recordInBlock--;
  					memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  					if(strcmp(tempRecord.name,strValue) == 0) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						leftEnd = 1;
  				}
  				else {
  					if((searchBlock - k) != 0) {
  						add_to_array(blocksArray,searchBlock - k);
  						recordInBlock = 7;
  						if(BF_ReadBlock(fileDesc,searchBlock - k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional left block %d\n", searchBlock - k);
  						leftShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  						if(strcmp(tempRecord.name,strValue) == 0){
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							leftEnd = 1;
  					}
  					else
  						leftEnd = 1;
  				}
  			}
  			if(leftShift == 1) {
  				if(BF_ReadBlock(fileDesc,searchBlock,&block) < 0) {
 	 				BF_PrintError("Could not read block\n");
  					return;
  				}
  			}
  			k = 1;
  			while(rightEnd == 0) {
  				if(tempRecordInBlock < 7) {
  					tempRecordInBlock++;
  					if(rightShift == 1 && searchBlock + k - 1 == lastBlock && tempRecordInBlock == recordsinLastBlock)
  						break;
  					memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  					if(strcmp(tempRecord.name,strValue) == 0) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						rightEnd = 1;
  				}
  				else {
  					if((searchBlock + k) != lastBlock + 1) {
  						add_to_array(blocksArray,searchBlock + k);
  						tempRecordInBlock = 0;
  						if(BF_ReadBlock(fileDesc,searchBlock + k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional right block %d\n", searchBlock + k);
  						rightShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  						if(strcmp(tempRecord.name,strValue) == 0){
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							rightEnd = 1;
  					}
  					else
  						rightEnd = 1;
  				}
  			}
  		}
  	}
  	else if(fieldNo == 2) {
  		int leftShift = 0, rightShift = 0;
  		char strValue[20];
  		strcpy(strValue,value);
  		memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  		if(strcmp(tempRecord.surname,strValue) > 0)
  			binary_search(firstRecord,midRecord - 1,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else if(strcmp(tempRecord.surname,strValue) < 0)
  			binary_search(midRecord + 1,lastRecord,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else {
  			print_record(tempRecord);
  			(*foundRecords)++;
  			int tempRecordInBlock = recordInBlock;
  			int k = 1;
  			int leftEnd = 0;
  			int rightEnd = 0;
  			while(leftEnd == 0) {
  				if(recordInBlock > 0) {
  					recordInBlock--;
  					memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  					if(strcmp(tempRecord.surname,strValue) == 0){
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						leftEnd = 1;
  				}
  				else {
  					if((searchBlock - k) != 0) {
  						add_to_array(blocksArray,searchBlock - k);
  						recordInBlock = 7;
  						if(BF_ReadBlock(fileDesc,searchBlock - k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional left block %d\n", searchBlock - k);
  						leftShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  						if(strcmp(tempRecord.surname,strValue) == 0) {
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							leftEnd = 1;
  					}
  					else
  						leftEnd = 1;
  				}
  			}
  			if(leftShift == 1) {
  				if(BF_ReadBlock(fileDesc,searchBlock,&block) < 0) {
 	 				BF_PrintError("Could not read block\n");
  					return;
  				}
  			}
  			k = 1;
  			while(rightEnd == 0) {
  				if(tempRecordInBlock < 7) {
  					tempRecordInBlock++;
  					if(rightShift == 1 && searchBlock + k - 1 == lastBlock && tempRecordInBlock == recordsinLastBlock)
  						break;
  					memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  					if(strcmp(tempRecord.surname,strValue) == 0) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						rightEnd = 1;
  				}
  				else {
  					if((searchBlock + k) != lastBlock + 1) {
  						add_to_array(blocksArray,searchBlock + k);
  						tempRecordInBlock = 0;
  						if(BF_ReadBlock(fileDesc,searchBlock + k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional right block %d\n", searchBlock + k);
  						rightShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  						if(strcmp(tempRecord.surname,strValue) == 0) {
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							rightEnd = 1;
  					}
  					else
  						rightEnd = 1;
  				}
  			}
  		}
  	}
  	else if(fieldNo == 3) {
  		int leftShift = 0, rightShift = 0;
  		char strValue[20];
  		strcpy(strValue,value);
  		memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  		if(strcmp(tempRecord.city,strValue) > 0)
  			binary_search(firstRecord,midRecord - 1,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else if(strcmp(tempRecord.city,strValue) < 0)
  			binary_search(midRecord + 1,lastRecord,foundRecords,fileDesc,fieldNo,value,lastBlock,recordsinLastBlock,maxRecords,blocksArray);
  		else {
  			print_record(tempRecord);
  			(*foundRecords)++;
  			int tempRecordInBlock = recordInBlock;
  			int k = 1;
  			int leftEnd = 0;
  			int rightEnd = 0;
  			while(leftEnd == 0) {
  				if(recordInBlock > 0) {
  					recordInBlock--;
  					memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  					if(strcmp(tempRecord.city,strValue) == 0) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						leftEnd = 1;
  				}
  				else {
  					if((searchBlock - k) != 0) {
  						add_to_array(blocksArray,searchBlock - k);
  						recordInBlock = 7;
  						if(BF_ReadBlock(fileDesc,searchBlock - k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional left block %d\n", searchBlock - k);
  						leftShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (recordInBlock * sizeof(Record)),sizeof(Record));
  						if(strcmp(tempRecord.city,strValue) == 0) {
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							leftEnd = 1;
  					}
  					else
  						leftEnd = 1;
  				}
  			}
  			if(leftShift == 1) {
  				if(BF_ReadBlock(fileDesc,searchBlock,&block) < 0) {
 	 				BF_PrintError("Could not read block\n");
  					return;
  				}
  			}
  			k = 1;
  			while(rightEnd == 0) {
  				if(tempRecordInBlock < 7) {
  					tempRecordInBlock++;
  					if(rightShift == 1 && searchBlock + k - 1 == lastBlock && tempRecordInBlock == recordsinLastBlock)
  						break;
  					memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  					if(strcmp(tempRecord.city,strValue) == 0) {
  						print_record(tempRecord);
  						(*foundRecords)++;
  					}
  					else
  						rightEnd = 1;
  				}
  				else {
  					if((searchBlock + k) != lastBlock + 1) {
  						add_to_array(blocksArray,searchBlock + k);
  						tempRecordInBlock = 0;
  						if(BF_ReadBlock(fileDesc,searchBlock + k,&block) < 0) {
 	 						BF_PrintError("Could not read block\n");
  							return;
  						}
  						printf("Checking additional right block %d\n", searchBlock + k);
  						rightShift = 1;
  						k++;
  						memcpy(&tempRecord,block + (tempRecordInBlock * sizeof(Record)),sizeof(Record));
  						if(strcmp(tempRecord.city,strValue) == 0) {
  							print_record(tempRecord);
  							(*foundRecords)++;
  						}
  						else
  							rightEnd = 1;
  					}
  					else
  						rightEnd = 1;
  				}
  			}
  		}
  	}
}
