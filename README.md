# EXCEL data parser

A small C library for extracting data from `.xlsx` spreadsheets.

## The Idea
---
Extract every cell from every sheet in a workbook and hand back the
data in a neutral form. The library does no interpretation — it resolves
shared strings, styles, and date serials, then gets out of the way. You
know the shape of your own spreadsheet, so the analysis is yours to write.


