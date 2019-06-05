#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BF.h"
#include "sort.h"


int main(int argc, char **argv) {
    
	if(argc>2) {
		printf("Incorrect arguments\n");
		exit(EXIT_FAILURE);
	}
	int fd, fd2;
	int fieldNo = 3;
	//int value = 3329966;
	char value[20];
	strcpy(value, "Keratsini");
	
	FILE *fp;
	char *token;
	char line[256];
	char filename[30] = "Records";
	char newFilename[40];
	strcpy(newFilename,filename);
	strcat(newFilename,"Sorted");
	char intString[2];
	sprintf(intString,"%d",fieldNo);
	strcat(newFilename,intString);

	Record record;
	BF_Init();
	Sorted_CreateFile(filename);
	fd = Sorted_OpenFile(filename);

	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp == NULL) {
			printf("There is no file with the name %s\n",argv[1]);
			return -1;
		}
		else {
			while(fgets(line,256,fp) != NULL) {	//Read every line
		        token = strtok(line,",");
		        record.id = atoi(token);

		        token = strtok(NULL, ",");
		        token++;
		        token[strlen(token) - 1] = 0;
		        strncpy(record.name, token, sizeof(record.name));

		        token = strtok(NULL, ",");
		        token++;
		        token[strlen(token) - 1] = 0;
		        strncpy(record.surname, token, sizeof(record.surname));

		        token = strtok(NULL, "\"");
		        strncpy(record.city, token, sizeof(record.city));

		        insertEntry(fd, record);
		    }
		}
	}
	else {
		fp = fopen("10000.csv","r");
		while(fgets(line,256,fp) != NULL) {	//Read every line
		    token = strtok(line,",");
		    record.id = atoi(token);

	        token = strtok(NULL, ",");
	        token++;
	        token[strlen(token) - 1] = 0;
	        strncpy(record.name, token, sizeof(record.name));

	        token = strtok(NULL, ",");
	        token++;
	        token[strlen(token) - 1] = 0;
	        strncpy(record.surname, token, sizeof(record.surname));

	        token = strtok(NULL, "\"");
	        strncpy(record.city, token, sizeof(record.city));

	        insertEntry(fd, record);
		}
	}

	Sorted_SortFile(filename,fieldNo);
	Sorted_checkSortedFile(newFilename, fieldNo);

	fd2 = Sorted_OpenFile(newFilename);
	Sorted_GetAllEntries(fd2,&fieldNo,NULL);
    
    return 1;
}
