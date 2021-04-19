Adam Hultman
30/10/2019

Did not include subdirectory functionality.

To compile:
	make

To run:

./<function> <img dir> [params...]
e.g. ./diskinfo test.img

The design of this simple file system relies on various structures which are filled by iterating over the map of the image byte-style. diskget relies on disklist to find file on image. Much of the data is copied with multiple lines of memcpy. The compilation and execution of the programs are split up into functions of the same file "p3.c".
