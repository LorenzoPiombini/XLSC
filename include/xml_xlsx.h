#ifndef _XML_XLSX_H
#define _XML_XLSX_H

#include <stdint.h>

enum Cell_type{
	CELL_EMPTY,
	CELL_NUM,
	CELL_DATE,
	CELL_BOOL,
	CELL_STR,
	CELL_LINE_STR
};

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
	char *format_code;
	uint8_t is_date;
};


/*to be expanded*/
struct Xf{
	int num_fmt_id;
	int is_date;
	int font_id;
	int fill_id;
	int border_id;
};

struct shared_string{
	char **s;
	size_t count;
};
struct Cell{
	int type;
	union{
		char *s;
		uint32_t date;
		long num;
		double d;
	}value;
	struct Format value_format;
};

int get_shared_strings(char *file_path,struct shared_string *shs);
int get_sheet_cell(char *file_path,struct Cell *c);
int get_formats_number(char *file_path,struct Format **format, struct Xf **xfs);

#endif
