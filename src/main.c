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
	long long size = 0;
	uint8_t *file_content = NULL; 
	struct F_unzip data  = {0};
	struct shared_string a = {0};
	struct Formats fn = {0};
	struct Xfs xfs = {0};
	struct Cells cells = {0};

	if((size = read_file(argv[1],&file_content)) == -1) return -1;
	if(unZIP(file_content,size,&data) == -1) return -1;

	uint8_t *fl = NULL;
	long long s = 0;
	if((s = browse_extracted_ZIP("xl/sharedStrings.xml",&data,&fl)) == -1){
		free(data.data);
		return 0;
	}

	if(get_shared_strings(fl,s,&a) == -1) goto clean;

	printf("found %ld strings\n",a.count);
	free(fl);
	fl = NULL;

	s = 0;
	if((s = browse_extracted_ZIP("xl/styles.xml",&data,&fl)) == -1) goto clean;

	if(get_formats_number(fl,s,&fn,&xfs) == -1) goto clean;

	printf("found %d xf records\n",xfs.count);
	free(fl);

	fl = NULL;
	s = 0;
	if((s = browse_extracted_ZIP("xl/worksheets/sheet1.xml",&data,&fl)) == -1) goto clean;

	get_sheet_cell(fl,s,&cells,&xfs,&a);

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
	free(fl);
	
clean:
	free(file_content);
	if(data.data) free(data.data);
	if(cells.c) free(cells.c);
	if(fn.f) free(fn.f);
	if(xfs.xfs) free(xfs.xfs);
	if(a.s) free(a.s);
	if(a.index) free(a.index);
	return 0;

}
