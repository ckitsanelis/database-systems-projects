#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "BF.h"
#include "exhash.h"


int main(int argc, char* argv[]) {

	if(argc>2){
		printf("Incorrect arguments\n");
		exit(EXIT_FAILURE);
	}

	FILE *fp;
	char *token;
	char line[256];

    	Record record;
	EH_info* info;
	BF_Init();
	EH_CreateIndex("EH_file", "name", 'c', 4, 3);
	info=EH_OpenIndex("EH_file");

	if(argc == 2){
		fp = fopen(argv[1],"r");
		if(fp == NULL)
		{
			printf("There is no file with the name %s\n",argv[1]);
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

		        EH_InsertEntry(info, record);
		    }
		}
	}
	printf("You can add manually records or type exit\n");
	while(fgets(line,256,stdin) != NULL) {
		token = strtok(line,",\n");
		if(strcmp(token,"exit") == 0)
			break;
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

		EH_InsertEntry(info, record);
	}

	void* value1 = "Mose";
	EH_GetAllEntries(*info, value1);
	EH_CloseIndex(info);
	HashStatistics("EH_file");
	return 1;
}
