#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "xml_xlsx.h"
#include "os_operations.h"



int main()
{
	struct shared_string a = {0};
	if(get_shared_strings("../d.test/xl_sharedStrings.xml",&a) == -1) goto clean;

	for(long i = 0; i < (long)a.count; i++)
		printf("[%s]\n",&a.s[a.index[i]]);

	printf("found %ld strings\n",a.count);


	struct Formats fn = {0};
	struct Xfs xfs = {0};
	if(get_formats_number("../d.test/xl_styles.xml",&fn,&xfs) == -1) goto clean;

	for(int i = 0; i < xfs.count; i++){
		printf("numFmtId=%d, is_date? %s.\n",xfs.xfs[i].num_fmt_id,xfs.xfs[i].is_date ? "yes":"no");
	}
	printf("found %d xf records\n",xfs.count);

	struct Cells cells = {0};
	get_sheet_cell("../d.test/xl_worksheets_sheet1.xml",&cells,&xfs);

	
	for(int i = 0; i < cells.count;i++){
		if(cells.c[i].type == CELL_EMPTY) continue;

		printf("cell '%s', found. ",cells.c[i].ref);
		switch(cells.c[i].type){
		case CELL_STR:
			printf("value is '%s'\n",&a.s[a.index[cells.c[i].value.index_sh_str]]);
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
	
clean:

	if(cells.c) free(cells.c);
	if(fn.f) free(fn.f);
	if(xfs.xfs) free(xfs.xfs);
	if(a.s) free(a.s);
	if(a.index) free(a.index);
	return 0;

}
