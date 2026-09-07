#ifndef _XML_XLSX_H
#define _XML_XLSX_H

#include <stdint.h>
enum Format_type{
	FNUM_GENERAL			= 0,
	FNUM_INT 				= 1,
	FNUM_FLOAT 				= 2, 
	FNUM_INT_SEP			= 3,
	FNUM_FLOAT_SEP 			= 4,
	FNUM_INT_PERC 			= 9, 	/* 0% */
	FNUM_FLOAT_PERC 		= 10, 	/* 0.00% */
	FNUM_SCIENTIFIC 		= 11, 	/* 0.00E+00 */
	FNUM_FRACTION 			= 12, 	/* # ?/? */
	FNUM_FRACTION_2 		= 13, 	/* # ??/?? */
	FNUM_DATE_MM_DD_YY 		= 14,
	FNUM_DATE_D_MMM_YY 		= 15,
	FNUM_DATE_D_MMM 		= 16,
	FNUM_DATE_MMM_YY 		= 17,
	FNUM_INT_PAREN       	= 37,   /* #,##0 ;(#,##0)             */
	FNUM_INT_PAREN_RED   	= 38,   /* #,##0 ;[Red](#,##0)        */
	FNUM_FLOAT_PAREN     	= 39,   /* #,##0.00;(#,##0.00)        */
	FNUM_FLOAT_PAREN_RED 	= 40,   /* #,##0.00;[Red](#,##0.00)   */
	FNUM_TIME_MS        	= 45,   /* mm:ss        */
    FNUM_TIME_ELAPSED   	= 46,   /* [h]:mm:ss    */
    FNUM_TIME_MSS       	= 47,   /* mmss.0       */
    FNUM_TEXT           	= 49,   /* @            */
};


struct Format{
	int type;
	char format_code[30];
	uint8_t is_date;
};

struct Formats{
	struct Format *f;
	int count;
};

/*to be expanded*/
struct Xf{
	int num_fmt_id;
	int is_date;
	int font_id;
	int fill_id;
	int border_id;
};

struct Xfs{
	struct Xf *xfs;
	int count;
};


struct shared_string{
	char *s;
	int *index;
	size_t count;
};

enum Cell_type{
	CELL_EMPTY,
	CELL_NUM,
	CELL_FLOAT,
	CELL_DATE,
	CELL_BOOL,
	CELL_STR,
	CELL_LINE_STR
};

struct Cell{
	char ref[12];
	int type;
	union{
		char *s;
		long num;
		double d;
		long date;
		int index_sh_str;
	}value;
};

struct Cells{
	struct Cell *c;
	int count;
};

struct Sheet{
	char *name;
	uint8_t hidden;
	struct Cells cells;
};

struct Workbook{};

int get_shared_strings(uint8_t *file_content, uint64_t size,struct shared_string *shs);
int get_sheet_cell(uint8_t *file_content,uint64_t size,struct Cells *c,struct Xfs *styles,struct shared_string *s);
int get_formats_number(uint8_t *file_content,uint64_t size,struct Formats *fn, struct Xfs *xfs);
int open_WorkBook(char *file_name, struct Cells *c);
void close_WorkBook(void);
#endif
