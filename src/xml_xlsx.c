#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "xml_xlsx.h"
#include "os_operations.h"

static int date_1904 = 0;
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

static int set_date_property(char *file_path,int *date_par);
static const struct Format *get_built_in_formats(int id);
static char *strstrnnt(const char *str, const char *find, size_t size, size_t *cursor);
static int is_number_date(int id);
static int is_date_char_present(char *code);
static void decode_excel_entities(char *s);
static long convert_excel_time_to_c_system(int nday);


static long convert_excel_time_to_c_system(int nday)
{
	uint32_t seconds = 60*60*24;
	long long _70_years_days = (365 * 70) + (70/4) + 1 ;
	long long n = nday - _70_years_days ;	
	
	return  n * seconds;
}


static void decode_excel_entities(char *s)
{
	char *w = s, *r = s;
	while(*r){
		if(*r != '&'){
			*w++ = *r++;
			continue;
		}

		if(strncmp(r,"&amp;",5) == 0){
			*w++ = '&';
			r += 5;
		}
	}
}

static int is_date_char_present(char *code)
{
	char *p = code;
	for(; *p;p++){
		switch(*p){
		case 'y':
		case 'Y':
		case 'd':
		case 'D':
		case 'm':
		case 'M':
			return 1;
		default:
			break;
		}
	}
	return 0;
}

static int is_number_date(int id)
{
	const struct Format  *b = get_built_in_formats(id);
	return b ? b->is_date : 0;
}

static char *strstrnnt(const char *str, const char *find, size_t size, size_t *cursor)
{
	int len = strlen(find);
	size_t i = 0;
	if(cursor){
		i = (*cursor == 0) ? 0 : *cursor; 
	}
	for(; i + len <= size;i++){
		if(memcmp(str + i,find,len) == 0){
			if(cursor){
				*cursor = i; 
			}
			return (char *)(str + i);
		}
	}	
	return NULL;
}

static const struct Format *get_built_in_formats(int id)
{
	for(size_t i = 0; i < BUILT_IN_FORMAT_SIZE; i++){
		if(built_in_formats[i].type == id) return &built_in_formats[i];
	}
	return NULL;
}

int get_shared_strings(uint8_t *file_content, uint64_t size,struct shared_string *shs)
{
	char digits[11] = {0};
	char *shared_string = NULL;
	
	size_t cursor = 0;
	char *count = strstrnnt((const char*)file_content,"uniqueCount",size,&cursor);
	if(!count) goto failed;

	while((uint64_t)(count - (char*)file_content) < size && *count != '"') count++;
	count++;
	char *end = count;
	while((uint64_t)(end - (char*)file_content) < size && *end != '"') end++;
	int dig_len = (end - (char*)file_content) - (count - (char *)file_content);
	strncpy(digits,count,dig_len);

	errno = 0;
	long strings_count = strtol(digits,NULL,10);
	if (errno == EINVAL || errno == ERANGE) goto failed;
	
	shared_string = malloc(size);
	if(!shared_string) goto failed;
	memset(shared_string,0,size);

	int *offset = malloc(strings_count * sizeof *offset);
	if(!offset)  goto failed;
	memset(offset,0,sizeof *offset * strings_count);

	char *si = NULL;
	int w = 0, scount = 0;
	while(scount < strings_count && 
			(si = strstrnnt((const char*)file_content,"<si>",size,&cursor))){
		*si = '\0';

		size_t cur = cursor;
		char *si_close = strstrnnt((const char*)file_content,"</si>",size,&cursor);
		if(!si_close) goto failed;
		*si_close = '\0';

		offset[scount] = w;
		char *t = NULL;
		while((t = strstrnnt((const char*)file_content,"<t",size,&cur))){
			if(cur > cursor) break;
			*t = '\0';
			while(*t != '>') t++;
			t++;
			while(*t != '<') {
				shared_string[w++] = *t++;
			}
		}


		w++;
		decode_excel_entities(&shared_string[offset[scount]]);
		scount++;
	}

	shs->s = shared_string;
	shs->index = offset;
	shs->count = scount;

	return 0;

failed:
	if(shared_string) 	free(shared_string);
	if(offset) 			free(offset);
	return -1;
}

int get_sheet_cell(uint8_t *file_content,uint64_t size,struct Cells *cells,struct Xfs *styles,struct shared_string *shs)
{
	
	char digits[11] = {0};
	size_t cursor = 0;
	char *row = NULL;

	cells->c = malloc(12*47*sizeof *cells->c);
	if(!cells->c) goto failed;
	memset(cells->c,0,12*47*sizeof *cells->c);

	while((row = strstrnnt((const char*)file_content,"<row ",size,&cursor))){
		*row = '\0';
		size_t cur = cursor;
		char *cell = NULL;
		
		while((cell = strstrnnt((const char*)file_content,"<c ",size,&cur))){
			*cell = '\0';

			char *p = cell;
			while(*p != '>') p++; 

			if(*(p - 1) == '/')continue; /*cell is empty, skip it*/

			int sz = p - cell;
			char buf[sz+1];
			memset(buf,0,sz+1);
			cell++;
			memcpy(buf,cell,sz);
			/*get the reference 'the name of the cell' i.e. A1 B1*/
			char *r =strstrnnt((const char*)buf,"r=",sz,NULL); 
			if(r){
				*r = '\0';
				r += 3;
				for(int i = 0; *r != '"'; (cells->c + cells->count)->ref[i++]= *r++);
			}

			/*get the style of the data in the cell
			 * IMPORTANT FOR DATES*/
			char *s = strstrnnt((const char*)buf,"s=",sz,NULL);
			if(s){
				*s = '\0';
				s += 3;
				int k = 0;
				char *p = s;
				for(k = 0; *p != '"'; k++,p++);

				memset(digits,0,11);
				strncpy(digits,s,k);

				errno = 0;
				int index = (int)strtol(digits,NULL,10);
				if (errno == EINVAL || errno == ERANGE) goto failed;

				if(index >= styles->count) goto failed; 

				struct Xf style = styles->xfs[index];
				char *v = strstrnnt((const char*)file_content,"<v>",size,&cur);
				if(v){
					*v = '\0';
					v += 3;
					int k = 0;
					char *p = v;
					for(k = 0; *p != '<'; k++,p++);
					memset(digits,0,11);
					strncpy(digits,v,k);


					char *t = NULL;
					if(style.is_date){
						errno = 0;
						int number = (int)strtol(digits,NULL,10);
						if (errno == EINVAL || errno == ERANGE) goto failed;

						(cells->c + cells->count)->value.date = convert_excel_time_to_c_system(number);
						(cells->c + cells->count)->type = CELL_DATE;
					}else if((t = strstrnnt((const char*)buf,"t=",sz, NULL))){
						*t = '\0';
						t += 3;
						switch(*t){
						case 's':
						{
							errno = 0;
							/*
							(cells->c + cells->count)->value.index_sh_str = (int)strtol(digits,NULL,10);
							*/
							(cells->c + cells->count)->value.s = &shs->s[shs->index[(int)strtol(digits,NULL,10)]];
							if (errno == EINVAL || errno == ERANGE) goto failed;

							(cells->c + cells->count)->type = CELL_STR;
							cells->count++;
							continue;
							break;
						}
						default:
						break;
						}
					}else{
						switch(style.num_fmt_id){
						case FNUM_GENERAL:
						case FNUM_INT:			
						case FNUM_INT_SEP:
						{
							errno = 0;
							int number = (int)strtol(digits,NULL,10);
							if (errno == EINVAL || errno == ERANGE) goto failed;

							(cells->c + cells->count)->value.num = number;
							(cells->c + cells->count)->type = CELL_NUM;
							break;
						}
						case FNUM_FLOAT:
						case FNUM_FLOAT_SEP:
						{
							errno = 0;
							double number = strtod(digits,NULL);
							if (errno == EINVAL || errno == ERANGE) goto failed;

							(cells->c + cells->count)->value.d = number;
							(cells->c + cells->count)->type = CELL_FLOAT;
							break;

						}
						default:
						continue;
						break;
						}
					}
					cells->count++;
					continue;
				}
			}
			(cells->c + cells->count)->type = CELL_EMPTY;
		}	
	}

	return 0;
failed:
	if(cells->c) free(cells->c);
	return -1;
}

int get_formats_number(uint8_t *file_content,uint64_t size,struct Formats *fn, struct Xfs *xfs)
{

	/*get count of custom number format*/

	char digits[11] = {0};
	size_t cursor = 0;
	char *num_fmts = strstrnnt((const char *)file_content,"<numFmts count=",size,&cursor);
	if(!num_fmts) goto get_xfs; /*NO special format */

	while(*num_fmts != '"') num_fmts++;
	num_fmts++;

	char *end = num_fmts;
	while(((uint64_t)(end - (char*)file_content) < size) && *end != '"') end++;
	int dig_len = (end - (char*)file_content) - (num_fmts - (char *)file_content);
	strncpy(digits,num_fmts,dig_len);

	errno = 0;
	long format_number_count = strtol(digits,NULL,10);
	if (errno == EINVAL || errno == ERANGE) goto failed;

	fn->f = malloc(sizeof *fn->f * format_number_count);
	if(!fn->f)goto failed;

	memset(fn->f,0,sizeof *fn->f * format_number_count);

	fn->count = format_number_count;
	/*get the index and format code*/
	for(int i = 0; i < format_number_count; i++){
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

		fn->f[i].type = (int)number;

		num_fmts = strstrnnt((const char*)file_content,"formatCode",size,&cursor);
		if(!num_fmts) goto failed;

		*num_fmts= '\0';

		while(*num_fmts != '"') num_fmts++;
		num_fmts++;
		int j = 0;
		while(*num_fmts != '"') fn->f[i].format_code[j++] = *num_fmts++;
		if(number >=164) fn->f[i].is_date = is_date_char_present(fn->f[i].format_code);
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

	xfs->xfs = malloc(sizeof *(xfs->xfs) * xfs_record_n);
	if(!xfs->xfs) goto failed; 

	char *xf = NULL;
	int j = 0;
	memset(xfs->xfs,0,sizeof *(xfs->xfs) * xfs_record_n);
	xfs->count = xfs_record_n;
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
			((xfs->xfs) + j)->num_fmt_id = (int) strtol(digits,NULL,10);
			if (errno == EINVAL || errno == ERANGE) goto failed;

			if((xfs->xfs + j)->num_fmt_id >= 164){
				for(int k = 0; k < fn->count; k++){
					if((xfs->xfs + j)->num_fmt_id != fn->f[k].type) continue;

					(xfs->xfs + j)->is_date = fn->f[k].is_date; 
				}
			}else{
				(xfs->xfs + j)->is_date = is_number_date((xfs->xfs + j)->num_fmt_id); 

			}
			j++;
		}
	}

	return 0;

failed:
	if(fn->f) free(fn->f);
	if(xfs) if(xfs->xfs) free(xfs->xfs);
	return -1;
}
