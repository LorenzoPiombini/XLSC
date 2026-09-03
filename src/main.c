#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
clean:

	if(fn.f) free(fn.f);
	if(xfs.xfs) free(xfs.xfs);
	if(a.s) free(a.s);
	if(a.index) free(a.index);
	return -1;

}
