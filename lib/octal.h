/**@file octal.h
@brief octal functions
*/
#ifndef octal_h
#define octal_h
/**
Does not output any leading 0 like C wants for numeric
literals (but not for string escapes).
@param buf4
should be ab uffer of at least 4 bytes, will be null
terminated
@param octet
the byte to convert into octal digits
@return the buf4 parameter is returned as well
*/
char * octet2octal(char *buf4, unsigned char octet);
/**
Uses only standard library functions, and octet2octal(),
makes a nice string allocated on the heap from the input
data, along with any escapes needed.
@param data
the binary data to convert
@param sz
how many bytes the data has for us to read
@param opt_strlength
if not NULL, we will write the number of output characters
here so that it saves you from calling strlen() on the
returned string.
@return a heap-allocated string that holds the data values
with all non-ascii stuff escaped, also newlines are
escaped to "\n" and so on for use in source code.
*/
char* data2cstr(unsigned char *data, long sz,
	long *opt_strlength);
#endif

