#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "xml_xlsx.h"
#include "os_operations.h"

struct Format built_in_formats[164] = {0};

static void init_built_in_formats(struct Format *f);
static char *strstrnnt(const char *str, const char *find, size_t size, size_t *cursor);

static char *strstrnnt(const char *str, const char *find, size_t size, size_t *cursor)
{
	int len = strlen(find);
	for(size_t i = (*cursor == 0) ? 0 : *cursor; i + len <= size;i++)
		if(memcmp(str + i,find,len) == 0) {*cursor = i; return (char *)(str + i);}

	return NULL;
}

static void init_built_in_formats(struct Format *f)
{
	for(int i = 0; i < 50; i++){
		switch(i){
		case FNUM_GENERAL:
		case FNUM_INT:
		case FNUM_FLOAT:
		case FNUM_INT_SEP:
		case FNUM_FLOAT_SEP:
		case FNUM_INT_PERC:
		case FNUM_FLOAT_PERC:
		case FNUM_SCIENTIFIC:
		case FNUM_FRACTION:
		case FNUM_FRACTION_2:
		case FNUM_DATE_MM_DD_YY:
		case FNUM_DATE_D_MMM_YY:
		case FNUM_DATE_D_MMM:
		case FNUM_DATE_MMM_YY:
		case FNUM_INT_PAREN:
		case FNUM_INT_PAREN_RED:
		case FNUM_FLOAT_PAREN:
		case FNUM_FLOAT_PAREN_RED:
		case FNUM_TIME_MS:
		case FNUM_TIME_ELAPSED:
		case FNUM_TIME_MSS:
		case FNUM_TEXT:
			f->type = i;
		default:
			continue;
		}
	}
}

int get_shared_strings(char *file_path,struct shared_string *shs)
{
	uint8_t *file_content = NULL; 
	long long size = read_file(file_path,&file_content);
	if(size == -1) return -1;

	char digits[11] = {0};
	char **shared_string = NULL;
	
	size_t cursor = 0;
	char *count = strstrnnt(file_content,"uniqueCount",size,&cursor);
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
	while((t = strstrnnt((char*)file_content,"<t",size,&cursor))){
		*t = '@';
		while(*t && *t != '>') t++;
		t++;/*skip >*/
		size_t sz = size - (t - (char*)file_content);
		size_t nest_curs = 0;
		char *end = strstrnnt(t,"</",sz,&nest_curs);
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

int get_formats_number(char *file_path,struct Format **format, struct Xf **xfs)
{
	uint8_t *file_content = NULL; 
	long long size = read_file(file_path,&file_content);
	if(size == -1) return -1;

	init_built_in_formats(built_in_formats);
	/*get count of custom number format*/

	char digits[11] = {0};
	size_t cursor = 0;
	char *num_fmts = strstrnnt(file_content,"<numFmts count=",size,&cursor);
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
		num_fmts = strstrnnt(file_content,"<numFmt ",size,&cursor);
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

		num_fmts = strstrnnt(file_content,"formatCode",size,&cursor);
		if(!num_fmts) goto failed;

		*num_fmts= '\0';

		while(*num_fmts != '"') num_fmts++;
		num_fmts++;
		int i = 0;
		while(*num_fmts != '"') (*format)->format_code[i++] = *num_fmts++;

		format_number_count--;
	}

get_xfs:

	char *cell_xfs = strstrnnt(file_content,"<cellXfs ",size,&cursor);
	if(!cell_xfs) goto failed;
	*cell_xfs= '\0';

	char *c =  strstrnnt(file_content,"count",size,&cursor);
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
	while((xf = strstrnnt((char*)file_content,"<xf",size,&cursor))){
		*xf = '\0';
		char *n_fmt_id = strstrnnt((char*)file_content,"numFmtId",size,&cursor);
		if(!n_fmt_id) goto failed;

		while(*n_fmt_id != '"') n_fmt_id++;
		*n_fmt_id++;
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
