#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "xml_xlsx.h"
#include "os_operations.h"

static char *strstrnnt(const char *str, const char *find, size_t size);

static char *strstrnnt(const char *str, const char *find, size_t size)
{
	int len = strlen(find);
	for(size_t i = 0; i + len <= size; i++)
		if(memcmp(str + i,find,len) == 0) return (char *)(str + i);

	return NULL;
}

int get_shared_strings(char *file_path,struct shared_string *shs)
{
	uint8_t *file_content = NULL; 
	long long size = read_file(file_path,&file_content);
	if(size == -1) return -1;

	char digits[11] = {0};
	char **shared_string = NULL;
	char *count = strstrnnt(file_content,"uniqueCount",size);
	if(!count) goto failed;

	while((count - (char*)file_content) < size && *count != '"') count++;
	count++;
	char *end = count;
	while((end - (char*)file_content) < size && *end != '"') end++;
	int dig_len = (end - (char*)file_content) - (count - (char *)file_content);
	strncpy(digits,count,dig_len);

	errno = 0;
	long strings_count = strtol(digits,NULL,10);
	if (errno == EINVAL || errno == ERANGE) goto failed;
	
	shared_string = malloc(strings_count * sizeof *shared_string);
	if(!shared_string) goto failed;
	memset(shared_string,0,sizeof *shared_string * strings_count);

	char *t = NULL;
	int c = 0;
	while((t = strstrnnt((char*)file_content,"<t",size))){
		*t = '@';
		while(*t && *t != '>') t++;
		t++;/*skip >*/
		size_t sz = size - (t - (char*)file_content);
		char *end = strstrnnt(t,"</",sz);
		if(!end) goto failed;

		int size_st = end-t;

		char st[size_st+1];
		shared_string[c] = malloc(size_st+1);  
		if(!shared_string[c]) goto failed;
		memset(shared_string[c],0,size_st+1);

		strncpy(shared_string[c],t,size_st);
		c++;
	}

	shs->s = shared_string;
	shs->count = c;

	free(file_content);
	return 0;

failed:
	if(file_content) free(file_content);
	if(shared_string){
		for(long i = 0; i < (long)strings_count; i++)
			if(shared_string[i]) free(shared_string[i]);
		free(shared_string);
	}
	return -1;
}

int get_sheet_cell(char *file_path,struct Cell *c)
{
	

}

int get_formats_numeber(char *file_path,struct Format **format)
{
	uint8_t *file_content = NULL; 
	long long size = read_file(file_path,&file_content);
	if(size == -1) return -1;

	char digits[11] = {0};
	/*get count of custom number format*/
	char *num_fmts = strstrnnt(file_content,"<numFmts count=",size);
	if(!num_fmts) return -1;

	while(*num_fmts != '=') num_fmts++;
	num_fmts++;

	char *end = num_fmts;
	while(end && (end - (char*)file_content) < size && *end != '"') end++;
	int dig_len = (end - (char*)file_content) - (num_fmts - (char *)file_content);
	strncpy(digits,count,dig_len);

	errno = 0;
	long format_number_count = strtol(digits,NULL,10);
	if (errno == EINVAL || errno == ERANGE) goto failed;

	format = malloc(sizeof *format * format_number_count);
	if(!format) goto failed;
	
	memset(format,0,sizeof *format * format_number_count);
	









}
