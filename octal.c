/**@file octal.c
@brief octal functions
*/
#include "octal.h"
/*
char * octet2octal(char *buf4, unsigned char octet) {
	const int d1 = (octet & 0xc0)>>6;
	const int d2 = (octet & 0x38)>>3;
	const int d3 = (octet & 0x07);
	buf[0]='0'+d1;
	buf[1]='0'+d2;
	buf[2]='0'+d3;
	buf[3]='\0';
	return buf4;
}
*/
char * octet2octal(char *buf4, unsigned char octet) {
	const int d1 = (octet & 0xc0);
	const int d2 = (octet & 0x38);
	const int d3 = (octet & 0x07);
	/*--
	the compiler should combine '0' and 0xc0 for us
	and this saves us some shifts
	--*/
	buf[0]='0'+d1-0xc0;
	buf[1]='0'+d2-0x38;
	buf[2]='0'+d3;
	buf[3]='\0';
	return buf4;
}

char* data2cstr(unsigned char *data, long sz,
	long *opt_strlength)
{
	long strsz = 4096;
	char * str = malloc(strsz);
	long w=0;
	long r;
	if(str == NULL) {
		fputs("Cannot allocate ram\r\n",stderr);
		exit(__LINE__);
	}
	for(r=0;r<sz;++r) {
		switch(data[r]){
		case ' ':
			str[w]=' ';
			++w;
			break;
		case '\r':
			str[w]='\\';
			++w;

			str[w]='r';
			++w;
			break;
		case '\n':
			str[w]='\\';
			++w;

			str[w]='n';
			++w;
			break;
		case '\t':
			str[w]='\\';
			++w;

			str[w]='t';
			++w;
			break;
		case '\0':
			str[w]='\\';
			++w;

			str[w]='0';
			++w;
			break;
		case '"':
			str[w]='\\';
			++w;

			str[w]='"';
			++w;
			break;
		case '\\':
			str[w]='\\';
			++w;

			str[w]='\\';
			++w;
			break;
		default:
			if(isprint(data[r])) {
				str[w] = data[r];
				++w;
			} else {
				char buf[4];
				octet2octal(buf,data[r]);

				str[w]='\\';
				++w;

				str[w]=buf[0];
				++w;

				str[w]=buf[1];
				++w;

				str[w]=buf[2];
				++w;
			}
		}
		if(strsz - w < 16) {
			char *newstr = realloc(str, strsz + 4096);
			if(newstr == NULL) {
				newstr = malloc(strsz + 4096);
				if(newstr == NULL) {
					fputs("cannot allocate enough ram for expansion\r\n",stderr);
					exit(__LINE__);
				}
				memcpy(newstr,str,strsz);
				free(str);
			}
			str = newstr;
			strsz += 4096;
		}
	}
}


