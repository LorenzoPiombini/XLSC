#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include "lz77.h"
#include "xml_xlsx.h"
#include "os_operations.h"


int main(int argc,char **argv)
{
	if(argc <= 1) return -1;
	struct Cells cells = {0};
	if(open_WorkBook(argv[1],&cells) == -1){
		fprintf(stderr,"cannot open workbook '%s'.\n",argv[1]);
		return -1;
	}

	for(int i = 0; i < cells.count;i++){
		if(cells.c[i].type == CELL_EMPTY) continue;

		printf("cell '%s', found. ",cells.c[i].ref);
		switch(cells.c[i].type){
		case CELL_STR:
			/*printf("value is '%s'\n",&a.s[a.index[cells.c[i].value.index_sh_str]]);*/
			printf("value is '%s'\n",cells.c[i].value.s);
			break;
		case CELL_DATE:
			struct tm *date = localtime((time_t*)&cells.c[i].value.date);
			printf("value is '%d/%d'\n",date->tm_mon+1,date->tm_mday);
			break;
		case CELL_NUM:
			printf("value is '%ld'\n",cells.c[i].value.num);
			break;
		case CELL_FLOAT:
			printf("value is '%2.f'\n",cells.c[i].value.d);
			break;
		default:
			printf("\n");
		}
	}

	printf("found %d cells\n",cells.count);
	close_WorkBook();
	return 0;

}
