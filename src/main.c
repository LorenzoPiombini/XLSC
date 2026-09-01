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
		printf("%s\n",a.s[i]);

	printf("found %ld strings\n",a.count);


	struct Format *f = NULL;
	struct Xf *xfs = NULL;
	if(get_formats_number("../d.test/xl_styles.xml",&f,&xfs) == -1) goto clean;


clean:

	if(f) free(f);
	if(xfs) free(xfs);
	if(a.s){
		for(long i = 0; i < (long)a.count; i++)
			if(a.s[i]) free(a.s[i]);
		free(a.s);
	}
	return -1;

}
