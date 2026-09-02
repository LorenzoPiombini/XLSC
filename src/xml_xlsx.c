#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "xml_xlsx.h"
#include "os_operations.h"

static const struct Format built_in_formats[] = {
    { 0, "General",		0},
    { 1, "0",			0},
    { 2, "0.00",		0},
    { 3, "#,##0",		0},
    { 4, "#,##0.00",	0},
    { 9, "0%",			0},
    {10, "0.00%",		0},
    {11, "0.00E+00",	0},
    {12, "# ?/?",		0},
    {13, "# ?\?/??",		0},
    {14, "mm-dd-yy",	1},
    {15, "d-mmm-yy",	1},
    {16, "d-mmm",		1},
    {17, "mmm-yy",		1},
    {18, "h:mm AM/PM",	1},
    {19, "h:mm:ss AM/PM",1},
    {20, "h:mm",		1},
    {21, "h:mm:ss",		1},
    {22, "m/d/yy h:mm",	1},
    {37, "#,##0 ;(#,##0)",0},
    {38, "#,##0 ;[Red](#,##0)",0},
    {39, "#,##0.00;(#,##0.00)", 0},
    {40, "#,##0.00;[Red](#,##0.00)",0},
    {45, "mm:ss",		1},
    {46, "[h]:mm:ss",	1},
    {47, "mmss.0",		1},
    {48, "##0.0E+0",	0},
    {49, "@",			0}
};

#define BUILT_IN_FORMAT_SIZE (sizeof(built_in_formats) / sizeof(struct Format))

static const struct Format *get_built_in_formats(int id);
static char *strstrnnt(const char *str, const char *find, size_t size, size_t *cursor);
static int is_number_date(int id, struct Format *f);


static int is_number_date(int id, struct Format *f)
{
	if(id >=164){
		
	}

	const struct Format  *b = get_built_in_formats(id);
	return b ? b->is_date : 0;
}

static char *strstrnnt(const char *str, const char *find, size_t size, size_t *cursor)
{
	int len = strlen(find);
	for(size_t i = (*cursor == 0) ? 0 : *cursor; i + len <= size;i++)
		if(memcmp(str + i,find,len) == 0) {*cursor = i; return (char *)(str + i);}

	return NULL;
}

static const struct Format *get_built_in_formats(int id)
{
	for(size_t i = 0; i < BUILT_IN_FORMAT_SIZE; i++){
		if(built_in_formats[i].type == id) return &built_in_formats[i];
	}
	return NULL;
}

int get_shared_strings(char *file_path,struct shared_string *shs)
{
	uint8_t *file_content = NULL; 
	long long size = read_file(file_path,&file_content);
	if(size == -1) return -1;

	char digits[11] = {0};
	char **shared_string = NULL;
	
	size_t cursor = 0;
	char *count = strstrnnt((const char*)file_content,"uniqueCount",size,&cursor);
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
	while((t = strstrnnt((const char*)file_content,"<t",size,&cursor))){
		*t = '@';
		while(*t && *t != '>') t++;
		t++;/*skip >*/
		size_t sz = size - (t - (char*)file_content);
		size_t nest_curs = 0;
		char *end = strstrnnt(t,"</",sz,&nest_curs);
		if(!end) goto failed;

		int size_st = end-t;

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
	

	return 0;
}

int get_formats_number(char *file_path,struct Format **format, struct Xf **xfs)
{
	uint8_t *file_content = NULL; 
	long long size = read_file(file_path,&file_content);
	if(size == -1) return -1;

	/*get count of custom number format*/

	char digits[11] = {0};
	size_t cursor = 0;
	char *num_fmts = strstrnnt((const char *)file_content,"<numFmts count=",size,&cursor);
	if(!num_fmts) goto get_xfs; /*NO special format */

	while(*num_fmts != '"') num_fmts++;
	num_fmts++;

	char *end = num_fmts;
	while(((end - (char*)file_content) < size) && *end != '"') end++;
	int dig_len = (end - (char*)file_content) - (num_fmts - (char *)file_content);
	strncpy(digits,num_fmts,dig_len);

	errno = 0;
	long format_number_count = strtol(digits,NULL,10);
	if (errno == EINVAL || errno == ERANGE) goto failed;

	*format = malloc(sizeof **format * format_number_count);
	if(!*format)goto failed;

	memset(*format,0,sizeof **format * format_number_count);

	/*get the index and format code*/
	while(format_number_count > 0){
		num_fmts = strstrnnt((const char *)file_content,"<numFmt ",size,&cursor);
		if(!num_fmts) goto failed;
		*num_fmts= '\0';

		while(*num_fmts != '"') num_fmts++;
		num_fmts++;

		char *end_id = num_fmts;
		while(*end_id != '"') end_id++;
		
		int s = (end_id - (char*)file_content) - (num_fmts - (char *) file_content);
		memset(digits,0,11);
		strncpy(digits,num_fmts,s);

		errno = 0;
		long number = strtol(digits,NULL,10);
		if (errno == EINVAL || errno == ERANGE) goto failed;

		(*format)->type = (int)number;

		num_fmts = strstrnnt((const char*)file_content,"formatCode",size,&cursor);
		if(!num_fmts) goto failed;

		*num_fmts= '\0';

		while(*num_fmts != '"') num_fmts++;
		num_fmts++;
		int i = 0;
		while(*num_fmts != '"') (*format)->format_code[i++] = *num_fmts++;

		format_number_count--;
	}

get_xfs:

	char *cell_xfs = strstrnnt((const char*)file_content,"<cellXfs ",size,&cursor);
	if(!cell_xfs) goto failed;
	*cell_xfs= '\0';

	char *c =  strstrnnt((const char*)file_content,"count",size,&cursor);
	while(*c != '"') c++;
	c++;

	char *c_end = c;
	while(*c_end != '"') c_end++;
	memset(digits,0,11);
	int n_d = (c_end - (char *)file_content) - (c - (char *) file_content);
	strncpy(digits,c,n_d);
	
	errno = 0;
	long xfs_record_n = strtol(digits,NULL,10);
	if (errno == EINVAL || errno == ERANGE) goto failed;

	*xfs = malloc(sizeof **xfs * xfs_record_n);
	if(!*xfs) goto failed; 

	char *xf = NULL;
	int j = 0;
	memset(*xfs,0,sizeof **xfs * xfs_record_n);
	while((xf = strstrnnt((const char*)file_content,"<xf",size,&cursor))){
		*xf = '\0';
		char *n_fmt_id = strstrnnt((const char*)file_content,"numFmtId",size,&cursor);
		if(!n_fmt_id) goto failed;

		while(*n_fmt_id != '"') n_fmt_id++;
		n_fmt_id++;
		char *end_n_fmt_id = n_fmt_id;
		while(*end_n_fmt_id != '"') end_n_fmt_id++;
		int n_d = (end_n_fmt_id - (char*)file_content) - (n_fmt_id - (char *) file_content);
		memset(digits,0,11);
		strncpy(digits,n_fmt_id,n_d);

		if(j < xfs_record_n){
			errno = 0;
			((struct Xf *)(*xfs) + j++)->num_fmt_id = (int) strtol(digits,NULL,10);
			if (errno == EINVAL || errno == ERANGE) goto failed;

			
		}
		
	}


	free(file_content);
	return 0;

failed:
	if(format){
		if(*format) free(*format);
	}

	if(xfs){
		if(*xfs) free(*xfs);
	}
	free(file_content);
	return -1;
}
