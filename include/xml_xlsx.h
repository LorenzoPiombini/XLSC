#ifndef _XML_XLSX_H
#define _XML_XLSX_H


struct shared_string{
	char **s;
	size_t count;
};

enum {
	CELL_EMPTY,
	CELL_NUM,
	CELL_DATE,
	CELL_BOOL,
	CELL_STR,
	CELL_LINE_STR
} Cell_type;

enum {
	FNUME_GENERAL,
	FNUM_DATE_MM_DD_YY = 14,
	FNUM_DATE_D_MMM_YY = 15,
	FNUM_DATE_D_MMM = 16,
	FNUM_DATE_D_MMM_YY = 17,
	F_TOTAL = 164
}Format_type;

struct Format{
	int type;
	int indx;
	struct Format *custom;
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
int get_formats_numeber(char *file_path,struct Format *format);

#endif
