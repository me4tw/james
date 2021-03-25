/**@file james.c
@brief implementation of the "james" sourcecode tool


Look at the headerfile that this tool generates for
some usage information such as command syntax and
available commands.

This source code should build on all ANSI C89 environments,
but to help with possible issues in parallel makefiles, we
use file locking, and on POSIX it is fcntl() file locking.
You may need to add your own file locking lock() and
unlock() functions which may simply do nothing if you will
run the tool serially, not in parallel.

#Mechanical Usage Scenario(s)
Build and run the james tool locally, should be as simple
as "cc tools/james.c -o james" which should compile and
work without errors. Otherwise it can be run as automatic
on the repo, or some automated client that continously
syncs, runs, resyncs on the repo. That way you could sync
source code changes, and moments later sync again to pull
down the resultant "james.h" without needing to be able to
run the tool locally.

##Step 1
Delete "./include/james.h"

##Step 2
Run "./james include/james.h src/main.c" and so on for
all source files. Each time "./james" executable runs, it
will read the "include/james.h" file, loading into RAM all
data it finds, then it will process the "src/main.c" file,
or whichever src file it is being run on this time, and will
add to ram the things it finds. then it will re-generate the
"include/james.h" file as the output. So at the end of the
pre-build header-update step, all source files have been
able to contribute their results into the combined header
file.

##Step 3
Build source files as per normal, i.e. "cc -Iinclude src/main.c".
And this will be good because when they include the
"include/james.h" headerfile they will have access to all the
defines within it, and that one file was built up from the
input of all source files within the project, and the results
are then available to each source file invididually when they
then get compiled.

##Optional step 3 different
Commit the newly (re)generated james.h to the source code
repository so that the build server incorporates the latest
results. This way no custom tool or step has to be run on
the build server, just some client must periodically sync
the latest repo version, run the james tool on everything,
then re-sync to upload the new james.h version which has
changed.

*/
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define LINEBUF_LEN 250

void genericDie(long line) {
	printf("failed on line %ld\n", line);
	exit(__LINE__);
}

#ifdef _POSIX_SOURCE
int isdebug(void) {
	return 0;
}
#include <unistd.h>
void the_sleep(int n) {
	sleep(n);
}
#include <fcntl.h>
const char* fname = "/tmp/james.lck";
void lock(/*const char *fname*/void) {
	struct flock fl;
    int fd;
	
	memset(&fl,0,sizeof fl);
	
	fl.l_type   = F_WRLCK; /*F_RDLCK, F_WRLCK, F_UNLCK   */
    fl.l_whence = SEEK_SET;/*SEEK_SET, SEEK_CUR, SEEK_END*/
    fl.l_start  = 0;       /*Offset from l_whence        */
    fl.l_len    = 0;       /*length, 0 = to EOF          */
    fl.l_pid    = getpid();/*our PID                     */
	fd = open(fname, O_WRONLY);
    fcntl(fd, F_SETLKW, &fl);/* F_GETLK, F_SETLK, F_SETLKW*/
	close(fd);	
}
void unlock(/*const char *fname*/void) {
	struct flock fl;
    int fd;
	
	memset(&fl,0,sizeof fl);

    fl.l_type   = F_WRLCK;  /*F_RDLCK, F_WRLCK, F_UNLCK   */
    fl.l_whence = SEEK_SET; /*SEEK_SET, SEEK_CUR, SEEK_END*/
    fl.l_start  = 0;        /*Offset from l_whence        */
    fl.l_len    = 0;        /*length, 0 = to EOF          */
    fl.l_pid    = getpid(); /*our PID                     */

	/* get the file descriptor */
    fd = open(fname, O_WRONLY);

	/* tell it to unlock the region */
    fl.l_type   = F_UNLCK;
	/* set the region to unlocked */
    fcntl(fd, F_SETLK, &fl);
	close(fd);
}
#else

#ifdef _MSC_VER
#include <Windows.h>
void the_sleep(int n) {
	Sleep(n*1000);
}
#endif
#ifdef __BORLANDC__
/*
#if !defined( _Windows )
void _Cdecl sleep(unsigned __seconds);
void _Cdecl sound(unsigned __frequency);
#endif
*/
#include <dos.h>
void the_sleep(int n) {
	sleep(n);
}
#endif



#ifdef _MSC_VER
#include <debugapi.h>
int isdebug(void) {
	return IsDebuggerPresent();
}
#else
int isdebug(void) {
	return 0;
}
#endif

const char* jmslck = "james.lck";
char buffer [L_tmpnam];
int buffer_inited=0;
#define LFILE "C:\\temp\\james.lck";
#ifdef __LINUX__
#undef LFILE
#define LFILE "/tmp/james.lck"
#endif

void lock(/*const char *fname*/void) {
	FILE *f;
	int warned = 0;
	
	if(!buffer_inited)
	{
		tmpnam (buffer);
		buffer_inited = 1;
		int i;
		for(i=strlen(buffer);i>0;--i){
			if(buffer[i]=='/' || buffer[i]=='\\') {
				buffer[i]='\0';
				break;
			}
		}
		memcpy(buffer+strlen(buffer),jmslck,
			strlen(jmslck)+1);
	}
	
	while(1) {
		f = fopen(buffer,"r");
		if(f == NULL) {
			f = fopen(buffer,"w");
			fputc('!',f);
			fclose(f);
			return;
		} else {
			fclose(f);
			if(!warned) {
				printf("sleeping until %s is removed",
					buffer);
				/*
				puts("sleeping until " LFILE " is removed");
				*/
				warned=1;
			}
			the_sleep(1);
		}
	}
}
void unlock(void/*const char *fname*/) {
	remove(buffer);
}
#endif


int g_je_line = 0;
char g_je_filename[20];
char* g_je_vars[256];
void je_clear_vars(void) {
	int i;
	for (i = 0; i < 256; ++i) {
		if (g_je_vars[i] != NULL) {
			free(g_je_vars[i]);
			g_je_vars[i] = NULL;
		}
	}
}
void je_setvar(char varletter, const char* varValue, int opt_len) {
	unsigned char uc = (unsigned char)varletter;
	int i = uc;
	if (opt_len < 0) {
		opt_len = (int)strlen(varValue);
	}
	if (g_je_vars[i] != NULL) {
		free(g_je_vars[i]);
	}
	g_je_vars[i] = malloc(opt_len + 1);
	if (g_je_vars[i] == NULL) {
		printf("failed to allocate %d bytes\n",opt_len);
		exit(__LINE__);
	}
	memcpy(g_je_vars[i], varValue, opt_len);
	g_je_vars[i][opt_len] = '\0';
}
const char* emptyString = "";
const char* je_getvar(char varLetter) {
	unsigned char uc = (unsigned char)varLetter;
	int i = uc;
	if (g_je_vars[i] == NULL) {
		return emptyString;
	}
	else {
		return g_je_vars[i];
	}
}
/**
@param linebuf
if it starts with an at-sign, then it is at-sign, filename-override, colon, linenumber-override, dollar sign, then text.
Else it is just text. Within text, at-sign is replaced with filename, hash sign is replaced with line number.
@everynms.h:18$EVERYNMS_INSTANCE_@_#
@param opt_modedfname
if not null and linebuf had an override, this will store what that override was, must already be allocated
@param opt_modedlineno
If not null and linebuf had a line override, this will store what the line override was
*/
void james_expand(char* linebuf,char *opt_modedfname, int *opt_modedlineno) {
	char temp[300];
	char* writeToThis = linebuf;
	int r, w;
	int coped = 0;
	char mod_filename[sizeof g_je_filename];
	int saved_line = -1;
	char saved_filename[sizeof g_je_filename];
	int saved = 0;
	int beenxed = 0;

	if (linebuf[0] == '@') {
		int x;
		/*--attempt to read--*/
		for (x = 0; x < 100 && linebuf[x] != '\0'; ++x) {
			if (linebuf[x] == ':') {
				beenxed = x;
				memcpy(saved_filename, g_je_filename,
					sizeof g_je_filename);
				saved = 1;
				if (beenxed < sizeof g_je_filename) {
					memcpy(g_je_filename, linebuf + 1, beenxed - 1);
					g_je_filename[beenxed - 1] = '\0';
				}
				else {
					memcpy(g_je_filename, linebuf + 1, sizeof(g_je_filename) - 1);
					g_je_filename[sizeof g_je_filename - 1] = '\0';
				}
			}
			else if (beenxed && linebuf[x] == '$') {
				char* num = &linebuf[beenxed + 1];
				saved_line = g_je_line;
				g_je_line = strtol(num, NULL, 10);
				/*--advance pointer and continue--*/
				linebuf = &linebuf[x + 1];
				break;
			}
		}
	}
	for (r = 0; r < (int)strlen(linebuf); ++r) {
		int isARealVar = 0;
		const char* str = NULL;
		if (linebuf[r] == '$') {
			str = je_getvar(linebuf[r + 1]);
			if (strlen(str) > 0) {
				isARealVar = 1;
			}
		}
		if (linebuf[r] == '@'
			|| (linebuf[r] == '#' && r > 0) 
			|| isARealVar) {
			if (!coped) {
				int h;
				for (h = 0; h < (int)strlen(g_je_filename); ++h) {
					mod_filename[h] = toupper(g_je_filename[h]);
					if (!isalnum(mod_filename[h])) {
						mod_filename[h] = '_';
					}
				}
				mod_filename[h] = '\0';
				coped = 1;
				memcpy(temp, linebuf, r);
				w = r;
			}
			if (linebuf[r] == '@') {
				memcpy(&temp[w], mod_filename, strlen(mod_filename));
				w += strlen(mod_filename);
			}
			else if (linebuf[r] == '#') {
				char itoabuf[30];
				sprintf(itoabuf, "%d", g_je_line);
				memcpy(&temp[w], itoabuf, strlen(itoabuf));
				w += strlen(itoabuf);
			}
			else if (linebuf[r] == '$' && isARealVar) {
				if (w + strlen(str) < sizeof temp) {
					memcpy(&temp[w], str, strlen(str)+1);
					w += strlen(str);
				}
				else {
					printf("ran out of temp space with %s and %s\n", temp, str);
					exit(__LINE__);
				}
				++r;
			}
		}
		else if (coped) {
			temp[w] = linebuf[r];
			++w;
		}
	}
	if (coped) {
		int n;
		temp[w] = '\0';
		n = strlen(temp);
		if (n > LINEBUF_LEN - 1) {
			n = LINEBUF_LEN - 1;
		}
		memcpy(writeToThis, temp, n);
		writeToThis[n] = '\0';
	}
	else if (beenxed) {
		memmove(writeToThis, linebuf, strlen(linebuf) + 1);
	}
	if (saved) {
		if (opt_modedfname != NULL) {
			memcpy(opt_modedfname, g_je_filename, strlen(g_je_filename) + 1);

		}
		if (opt_modedlineno != NULL) {
			*opt_modedlineno = g_je_line;
		}
		memcpy(g_je_filename, saved_filename,
			sizeof saved_filename);
		if (saved_line > -1) {
			g_je_line = saved_line;
		}
	}
	else {
		if (opt_modedfname != NULL) {
			opt_modedfname[0] = '\0';
		}
		if (opt_modedlineno != NULL) {
			*opt_modedlineno = -1;
		}
	}
}

struct list {
	char *name;
	struct list_item *head;
	struct list *next_list;
};

struct list_item {
	char *value;
	char fname[sizeof g_je_filename];
	int lineno;
	struct list_item *next;
};

struct list * first_list;
void apply_listitem_fn_ln(struct list_item* x, const char* opt_fn, const int* opt_ln) {
	if (opt_fn != NULL && opt_fn[0] != '\0') {
		memcpy(x->fname, opt_fn,
			strlen(opt_fn) + 1);
	}
	else {
		memcpy(x->fname, g_je_filename,
			strlen(g_je_filename) + 1);
	}
	if (opt_ln != NULL && *opt_ln >= 0) {
		x->lineno = *opt_ln;
	}
	else {
		x->lineno = g_je_line;
	}
}
void add_list_item(const char* listname, const char* value,
	const char *opt_fn, const int *opt_ln){
	struct list * x;
	x = first_list;
	/*--initial insert in total--*/
	if (x == NULL) {
		first_list = malloc(sizeof(struct list));
		if (first_list == NULL) {
			printf("failed to malloc\n");
			unlock();
			exit(__LINE__);
		}
		first_list->next_list = NULL;
		first_list->name = malloc(strlen(listname) + 1);
		if (first_list->name == NULL) {
			printf("failed to malloc\n");
			unlock();
			exit(__LINE__);
		}
		memcpy(first_list->name, listname,
			strlen(listname) + 1);
		first_list->head = malloc(sizeof(struct list_item));
		if (first_list->head == NULL) {
			printf("failed to malloc\n");
			unlock();
			exit(__LINE__);
		}
		first_list->head->next = NULL;
		first_list->head->value = malloc(strlen(value) + 1);
		if (first_list->head->value == NULL) {
			printf("failed to malloc\n");
			unlock();
			exit(__LINE__);
		}
		memcpy(first_list->head->value, value,
			strlen(value) + 1);
		/*--apply the filename and linenumber--*/
		apply_listitem_fn_ln(first_list->head, opt_fn, opt_ln);
		
		

		return;
	}
	while(1) {
		if(x->name==NULL
			|| strcmp(x->name,listname)==0) {
			if(x->head == NULL) {
				x->head = malloc(sizeof(struct list_item));
				if (x->head == NULL) {
					printf("failed to malloc\n");
					unlock();
					exit(__LINE__);
				}
				x->head->value = malloc(strlen(value)+1);
				if (x->head->value == NULL) {
					printf("failed to malloc\n");
					unlock();
					exit(__LINE__);
				}
				memcpy(x->head->value,value,strlen(value)
					+1);
				/*memcpy(x->head->fname, g_je_filename,
					strlen(g_je_filename) + 1);
				x->head->lineno = g_je_line;*/
				apply_listitem_fn_ln(x->head, opt_fn, opt_ln);
				x->head->next = NULL;
			} else {
				struct list_item *y;
				y = x->head;
				
				while(1) {
					
					if(y->value == NULL
						|| strcmp(y->value,value)==0) {
						/*--item already exists--*/
						return;
					}
					
					if(y->next == NULL) {
						y->next = malloc(
							sizeof(struct list_item));
						if (y->next == NULL) {
							printf("failed to malloc\n");
							unlock();
							exit(__LINE__);
						}
						y->next->value = malloc(strlen(value)
							+1);
						if (y->next->value == NULL) {
							printf("failed to malloc\n");
							unlock();
							exit(__LINE__);
						}
						memcpy(y->next->value,value,
							strlen(value)+1);
						apply_listitem_fn_ln(y->next, opt_fn, opt_ln);
						/*
						memcpy(y->next->fname, g_je_filename,
							strlen(g_je_filename) + 1);
						y->next->lineno = g_je_line;*/
						y->next->next = NULL;
						return;
					}
					
					y = y->next;
				}
				
			}
		} else {
			
			if(x->next_list == NULL) {
				x->next_list = malloc(sizeof(struct list));
				if (x->next_list == NULL) {
					printf("failed to malloc\n");
					unlock();
					exit(__LINE__);
				}
				x->next_list->name = malloc(strlen(listname)
					+1);
				if (x->next_list->name == NULL) {
					printf("failed to malloc\n");
					unlock();
					exit(__LINE__);
				}
				memcpy(x->next_list->name,listname,
				 strlen(listname)+1);
				x->next_list->head = NULL;
				x->next_list->next_list = NULL;
			}
			x = x->next_list;
			
		}
	}
}

void shuffle_trim(char *linebuf) {
	int i=0;
	while(isspace(linebuf[i])
		&& linebuf[i] != '\0') {
		++i;
	}
	if(linebuf[i]=='\0') {
		linebuf[0]='\0';
	} else {
		memmove(&linebuf[0],&linebuf[i],
			(strlen(linebuf)-i)+1);
	}
	i = strlen(linebuf);
	if (i > 0) {
		--i;
		while (isspace(linebuf[i]) && i > 0) {
			linebuf[i] = '\0';
			--i;
		}
	}
}


/*--must be rest on exit line_is_james--*/
int g_jatl_i=0;
char g_jatl_listname[50];
void JAMES_ADD_TO_LIST(char *linebuf){
	shuffle_trim(linebuf);
	switch(g_jatl_i){

			
		case 0:
			james_expand(linebuf,NULL,NULL);
			if(strlen(linebuf) 
				> (sizeof g_jatl_listname ) - 1) {
				printf("error %s is too long a list name\n",
					linebuf);
			} else {
				memcpy(g_jatl_listname, linebuf,
					strlen(linebuf) + 1);
			}
			break;
			
		default:
		{
			int ln;
			char fn[sizeof g_je_filename];
			james_expand(linebuf,&fn[0],&ln);
			add_list_item(g_jatl_listname, linebuf,&fn[0],&ln);
		}
			
			break;
	}
	g_jatl_i = g_jatl_i+1;
}

struct alias_positional {
	char varLetter;
	int posNum;
	struct alias_positional* next;
};
struct alias_plus_macroline {
	char* line;
	struct alias_plus_macroline* next;
};
struct alias_plus {
	char* name;
	int numPositionals;
	struct alias_positional* head;
	struct alias_plus* next;
	int alsoPlusNum;
	char** alsoPlus;
	char* define_output_name;
	struct alias_plus_macroline* macro_lines;
};
/*
void fail2malloc(void *p,long line) {
	if (p == NULL) {
		puts("failed to malloc");
		exit(__LINE__);
	}
}*/
const char* f2m_str = "failed to malloc";
#define fail2malloc(p,line) \
if (p == NULL) { \
	puts(f2m_str); \
	exit(__LINE__); \
}
struct alias_plus* alias_plus_head = NULL;
struct alias_plus* alias_plus_newx(const char* name, int len) {
	struct alias_plus* x;
	x = malloc(sizeof (struct alias_plus));
	fail2malloc(x, __LINE__);
	x->name = malloc(len + 1);
	fail2malloc(x->name, __LINE__);
	memcpy(x->name, name, len + 1);
	x->name[len] = '\0';
	x->numPositionals = 0;
	x->head = NULL;
	x->next = NULL;
	x->alsoPlusNum = 0;
	x->alsoPlus = NULL;
	x->define_output_name = NULL;
	x->macro_lines = NULL;
	return x;
}
struct alias_plus* alias_plus_get(const char* name, int len);
void alias_plus_addline(const char* name, int len, const char *line, int linelen) {
	struct alias_plus* x = alias_plus_get(name, len);
	struct alias_plus_macroline* z;
	if (x == NULL) {
		genericDie(__LINE__);
		return;
	}

	z = malloc(sizeof(struct alias_plus_macroline));
	fail2malloc(z,__LINE__);
	z->next = NULL;
	z->line = malloc(linelen + 1);
	fail2malloc(z, __LINE__);
	memcpy(z->line, line, linelen);
	z->line[linelen] = '\0';

	if (x->macro_lines == NULL) {
		x->macro_lines = z;
	}
	else {
		struct alias_plus_macroline* y = x->macro_lines;
		while (y!= NULL && y->next != NULL) {
			y = y->next;
		}
		if (y != NULL) {
			y->next = z;
		}
	}

}
void alias_plus_new(const char* name, int len) {
	if (len == -1) {
		len = strlen(name);
	}
	if (alias_plus_head == NULL) {
		alias_plus_head = alias_plus_newx(name, len);
	}
	else {
		struct alias_plus* x = alias_plus_head;
		while (x != NULL) {
			if (strlen(x->name)==len
				&& strncmp(x->name, name, len) == 0) {
				return;
			}
			if (x->next == NULL) {
				x->next = alias_plus_newx(name, len);
				return;
			}
			x = x->next;
		}
	}
}
struct alias_plus * alias_plus_get(const char* name, int len) {
	struct alias_plus* x = NULL;
	if (len == -1) {
		len = strlen(name);
	}
	x = alias_plus_head;
	while (x != NULL) {
		if (strlen(x->name)==len 
			&& strncmp(x->name, name, len) == 0) {
			return x;
		}
		if (x->next == NULL) {
			return NULL;
		}
		x = x->next;
	}
	return NULL;
}
void alias_plus_define_positional(const char* name, int len, char varLetter) {
	struct alias_plus* x = alias_plus_get(name,len);
	struct alias_positional* y = NULL;
	int pos;
	if (x == NULL) {
		alias_plus_new(name, len);
		x = alias_plus_get(name, len);
	}
	if (x == NULL) {
		return;
	}
	pos = x->numPositionals;
	y = x->head;
	if (x->head == NULL) {
		x->head = malloc(sizeof(struct alias_positional));
		fail2malloc(x->head, __LINE__);
		x->head->varLetter = varLetter;
		pos++;
		x->head->posNum = pos;
		x->head->next = NULL;
	}
	else {
		while (y != NULL) {
			if (y->varLetter == varLetter) {
				return;
			}
			if (y->next == NULL) {
				y->next = malloc(sizeof(struct alias_positional));
				fail2malloc(y->next, __LINE__);
				y->next->next = NULL;
				y->next->varLetter = varLetter;
				++pos;
				y->next->posNum = pos;
				break;
			}
			y = y->next;
		}
	}

	x->numPositionals = pos;
}
void alias_plus_free_macrolines(struct alias_plus_macroline* head) {
	struct alias_plus_macroline* next;
	while (head != NULL) {
		next = head->next;
		free(head);
		head = next;
	}
}
struct ap_invocation {
	char* name;
	char* fname;
	int lineno;
	int argc;
	char** argv;
	struct ap_invocation* next;
};
struct ap_invocation* ap_inv_head = NULL;
struct ap_invocation* ap_inv_create(const char* name, int opt_namelen) {
	struct ap_invocation* ret;
	if (opt_namelen < 0) {
		opt_namelen = (int)strlen(name);
	}
	ret = malloc(sizeof(struct ap_invocation));
	fail2malloc(ret,__LINE__);
	ret->argc = 0;
	ret->argv = NULL;
	ret->fname = NULL;
	ret->next = NULL;
	ret->name = malloc(opt_namelen + 1);
	fail2malloc(ret->name,__LINE__);
	memcpy(ret->name, name, opt_namelen + 1);
	return ret;
}
int ap_inv_testifalready(struct ap_invocation* ap) {
	struct ap_invocation* x = ap_inv_head;
	while (x != NULL) {
		if (strcmp(ap->name, x->name) == 0) {
			if (strcmp(ap->fname, x->fname) == 0) {
				if (ap->lineno == x->lineno) {
					if (ap->argc == x->argc) {
						int i;
						int different = 0;
						for (i = 0; i < ap->argc; ++i) {
							if (strcmp(ap->argv[i], x->argv[i]) != 0) {
								different = 1;
								break;
							}
						}
						if (!different) {
							return 1;
						}
					}
				}
			}
		}
		x = x->next;
	}
	return 0;
}
void ap_inv_free(struct ap_invocation* in) {
	struct ap_invocation* nex = NULL;
	while (in != NULL) {
		if (in->name != NULL) {
			free(in->name);
		}
		if (in->fname != NULL) {
			free(in->fname);
		}
		if (in->argv != NULL) {
			int i;
			for (i = 0; i < in->argc; ++i) {
				if (in->argv[i] != NULL) {
					free(in->argv[i]);
				}
			}
			free(in->argv);
		}
		/*if (in->next != NULL) {
			nex = in->next;
		}*/
		nex = in->next;
		if (ap_inv_head == in) {
			ap_inv_head = NULL;
		}
		free(in);
		in = nex;
	}
}
/*--must be rest on exit line_is_james--*/
int g_jiap_i = 0;
int g_jiap_argno = 0;
//char g_jiap_aliasname[100];
//int g_jiap_aliaslen = 0;
struct ap_invocation* g_jiap_apinv = NULL;
void JAMES_INVOKE_ALIAS_PLUS(char* linebuf) {
	switch (g_jiap_i) {
	case 0:
		g_jiap_argno = 0;
		if (g_jiap_apinv != NULL) {
			ap_inv_free(g_jiap_apinv);
			g_jiap_apinv = NULL;
		}
		shuffle_trim(linebuf);
		/*g_jiap_aliaslen = strlen(linebuf);
		if (g_jiap_aliaslen > sizeof g_jiap_aliasname-1) {
			g_jiap_aliaslen = sizeof g_jiap_aliasname-1;
		}
		memcpy(g_jiap_aliasname, linebuf, g_jiap_aliaslen);
		g_jiap_aliasname[g_jiap_aliaslen] = '\0';*/
		g_jiap_apinv = ap_inv_create(linebuf, -1);
			break;
	case 1:
	{
		char expander[sizeof g_je_filename];
		int expanded;
		james_expand(linebuf, &expander[0], &expanded);
		g_jiap_apinv->lineno = expanded;
		g_jiap_apinv->fname = malloc(strlen(expander) + 1);
		fail2malloc(g_jiap_apinv->fname,__LINE__);
		memcpy(g_jiap_apinv->fname, expander, strlen(expander) + 1);
	}
		break;
	case 2:
		g_jiap_apinv->argc = strtol(linebuf, NULL, 10);
		if (g_jiap_apinv->argc < 0) {
			genericDie(__LINE__);
		}
		g_jiap_apinv->argv = malloc(sizeof(char*) * g_jiap_apinv->argc);

		break;

	default:
		if (g_jiap_argno > g_jiap_apinv->argc) {
			printf("expected only %d but got at least %d args for alias plus invocation %s %s %d\n",
				g_jiap_apinv->argc, g_jiap_argno, g_jiap_apinv->name, g_jiap_apinv->fname, g_jiap_apinv->lineno);
			genericDie(__LINE__);
		}
		shuffle_trim(linebuf);
		g_jiap_apinv->argv[g_jiap_argno] = malloc(strlen(linebuf)+1);
		fail2malloc(g_jiap_apinv->argv[g_jiap_argno],__LINE__);
		memcpy(g_jiap_apinv->argv[g_jiap_argno], linebuf, strlen(linebuf) + 1);
		++g_jiap_argno;

		if (g_jiap_argno == g_jiap_apinv->argc) {
			if (ap_inv_testifalready(g_jiap_apinv)) {
				ap_inv_free(g_jiap_apinv);
				g_jiap_apinv = NULL;
			}
			else {
				if (ap_inv_head == NULL) {
					ap_inv_head = g_jiap_apinv;
				}
				else {
					struct ap_invocation* x = ap_inv_head;
					while (x->next != NULL) {
						x = x->next;
					}
					x->next = g_jiap_apinv;
				}
				g_jiap_apinv = NULL;
			}
		}

		break;
	}
	g_jiap_i = g_jiap_i + 1;
}
/*--must be rest on exit line_is_james--*/
int g_jap_i = 0;
int g_jap_plusnum = 0;
int g_jap_plus_i = 0;
char g_jap_aliasname[100];
int g_jap_aliaslen=0;
void JAMES_ALIAS_PLUS(char* linebuf) {
	
	switch (g_jap_i) {
	case 3:
	{
		struct alias_plus* x = alias_plus_get(g_jap_aliasname, g_jap_aliaslen);
		if (x == NULL) {
			genericDie(__LINE__);
		}
		shuffle_trim(linebuf);
		x->define_output_name = malloc(strlen(linebuf) + 1);
		fail2malloc(x->define_output_name, __LINE__);
		memcpy(x->define_output_name, linebuf, strlen(linebuf) + 1);

		/*--we have to clear all macro lines or it will just add up?--*/
		alias_plus_free_macrolines(x->macro_lines);
		x->macro_lines = NULL;

	}

		break;

	case 2:
	{
		struct alias_plus* x = alias_plus_get(g_jap_aliasname, g_jap_aliaslen);
		if (x == NULL) {
			genericDie(__LINE__);
		}
		x->alsoPlus[g_jap_plus_i] = malloc(strlen(linebuf) + 1);
		fail2malloc(x->alsoPlus[g_jap_plus_i], __LINE__);
		memcpy(x->alsoPlus[g_jap_plus_i], linebuf, strlen(linebuf));
		x->alsoPlus[g_jap_plus_i][strlen(linebuf)] = '\0';
		shuffle_trim(x->alsoPlus[g_jap_plus_i]);
	}
		++g_jap_plus_i;
		if (g_jap_plus_i < g_jap_plusnum) {
			/*--remain in state 2--*/
			--g_jap_i;
		}
		break;
	case 1:
		g_jap_plusnum = (int)strtol(linebuf, NULL, 10);
		if (g_jap_plusnum < 0) {
			genericDie(__LINE__);
		}
		g_jap_plus_i = 0;
		if(g_jap_plusnum>0){
			struct alias_plus* x = alias_plus_get(g_jap_aliasname, g_jap_aliaslen);
			if (x == NULL) {
				genericDie(__LINE__);
			}
			x->alsoPlusNum = g_jap_plusnum;
			x->alsoPlus = malloc(sizeof(char*) * g_jap_plusnum);
			fail2malloc(x->alsoPlus, __LINE__);
		}
		break;
	case 0:
		g_jap_plusnum = 0;
		g_jap_aliasname[0]='\0';
		g_jap_aliaslen = 0;
		je_clear_vars();
	{
		const char* start = linebuf;
		int varPositional = 1;
		int posOfOpenParen;
		int startlen=0;
		for (posOfOpenParen = 0; posOfOpenParen < (int)strlen(linebuf); posOfOpenParen++) {
			if (linebuf[posOfOpenParen] == '(') {
				startlen = posOfOpenParen;
				while (start[0] != '(' && start[0] != '\0' 
					&& isspace(start[0]) && startlen > 0) {
					++start;
					--startlen;
				}
				alias_plus_new(start, startlen);
				if (startlen+1 > sizeof g_jap_aliasname) {
					printf("alias %s is too long\n", start);
					exit(__LINE__);
				}
				memcpy(g_jap_aliasname, start, startlen);
				g_jap_aliasname[startlen + 1] = '\0';
				g_jap_aliaslen = startlen;
//				alias_plus_new(linebuf, posOfOpenParen);
				break;
			}
		}
		if (posOfOpenParen < (int)strlen(linebuf)) {
			int i = posOfOpenParen;
			while (i < (int)strlen(linebuf)) {
				char val;
				while (linebuf[i] != '$'
					&& linebuf[i] != ')'
					&& linebuf[i] != '\0') {
					++i;
				}
				val = linebuf[i];
				if (val == ')') {
					break;
				}
				++i;
				val = linebuf[i];
				if (val == ')') {
					break;
				}

				alias_plus_define_positional(start, startlen, val);

				while (linebuf[i] != ','
					&& linebuf[i] != ')'
					&& linebuf[i] != '\0') {
					++i;
				}
			}
		}
	}
		break;
	default:

		shuffle_trim(linebuf);
		alias_plus_addline(g_jap_aliasname, g_jap_aliaslen, linebuf, strlen(linebuf));

		break;
	}
	g_jap_i = g_jap_i + 1;
}

const char* headerfile_footer = "\n#endif /*james_h*/\n";
const char *header = "/**@file james.h\n\
@brief externally generated header from \"james\" program\n\
\n\
\n\
It is up to the build process to delete this file at the\n\
start of the compilation or else old stale values will\n\
perpetuate and the output will not reflect the current\n\
fresh state of the the source files in the project.\n\
\n\
For more information, inspect tools\\james.c which is\n\
the source code for the james tool which executed against\n\
each normal source file in the repo in order to build up\n\
this james.h generated header file using james-commands\n\
found in the source files, which start with forward-slash,\n\
asterix,hash and end with hash,asterix,forward-slash (much\n\
like normal block comments but with a hash character on the\n\
inside of the opening and closing tags), followed by one\n\
item per line, the first of which must be a recognised james\n\
command name such as JAMES_ADD_TO_LIST.\n\
\n\
The output of the james tool is standard c-preprocessor\n\
defines in the james.h generated header file which means\n\
so long as one person runs the james tool and checks in\n\
the changed headerfile then the rest of the build\n\
environment can be totally normal without needing to use\n\
a custom preprocessor or build wrapper, and the fact that\n\
the commands look like block comments to the normal compiler\n\
and that the source files are not generated or changed by\n\
the tool but just include standard define symbols whose\n\
contents are updated offline by the tool, means that this\n\
has the least impact in the build process and that\n\
\"technical debt\" is kept as low as possible.\n\
\n\
##Example\n\
####file: mod_joys.c\n\
~~~{.c}\n\
/ * # (without spaces)\n\
JAMES_ADD_TO_LIST\n\
BUILT_IN_MODULES\n\
mod_joys_init\n\
mod_duplicatetest\n\
# * / (without spaces)\n\
~~~\n\
####file: mod_snd.c\n\
~~~{.c}\n\
/ * # (without spaces)\n\
JAMES_ADD_TO_LIST\n\
BUILT_IN_MODULES\n\
mod_sound_init\n\
mod_duplicatetest\n\
# * / (without spaces)\n\
~~~\n\
####file: mod_misc.c\n\
~~~{.c}\n\
/ * # (without spaces)\n\
JAMES_ADD_TO_LIST\n\
BUILT_IN_MODULES\n\
mod_keyboard\n\
mod_mouse\n\
mod_timer\n\
mod_duplicatetest\n\
# * / (without spaces)\n\
~~~\n\
###Results of this in generated james.h\n\
~~~{.c}\n\
#define BUILT_IN_MODULES \\\n\
mod_joys_init,\\\n\
mod_duplicatetest,\\\n\
mod_sound_init,\\\n\
mod_keyboard,\\\n\
mod_mouse,\\\n\
mod_timer\n\
~~~\n\
You should be able to imagine the power that this\n\
enables. There are other commands too such as string\n\
hashing and a switch+enum equivalent to function\n\
pointers.\n\
\n\
#Tips\n\
On a data value line of a james command block, you\n\
can use the @ character to give the current source\n\
file name as all-caps and underscores for special\n\
characters, and the $ character will give the line\n\
number of the source code where this james command\n\
was first started. If the @ occurs within double\n\
quotes then it will not be upper-cased and will\n\
include all original characters. It is length limited\n\
and cannot be too long so keep filenames shorter.\n\
Does not include the folder or directory path.\n\
*/\n\
#ifndef james_h\n\
#define james_h\n\
\n\
#ifndef STRINGIFY\n\
/**\n\
Ensure that the STRINGIFY() macro is defined so we\n\
can use them as # is a reserved character\n\
*/\n\
#define STRINGIFY(X) #X\n\
#endif\n\
#ifndef CONCAT\n\
/**\n\
Ensure that the CONCAT() macro is defined so we can\n\
use them as ## are two reserved characters\n\
*/\n\
#define CONCAT(X,Y) X ## Y\n\
#endif\n\
#ifndef QUOTE\n\
/**\n\
Ensure that the QUOTE() macro is defined as well\n\
*/\n\
#define QUOTE(X) X\n\
#endif\n\
\n\
\n";
const char * s_generated_list = "/** Generated List ";
const char * s_startlist_define = "#define ";
int jchashchar(int sum, char c) {
	if (isalnum(c)) {
		return sum + c;
	}
	return sum;
}
int jchash(const char *str){
	int i;
	int h=0;
	for(i=0;i<(int)strlen(str);++i){
		const char c = str[i];
		h = jchashchar(h, c);
	}
	return h;
}

struct alias_plus* g_tiap_ap = NULL;
int test_invocate_alias_plus(const char* linebuf) {
	const char* start = linebuf;
	const char* openParen = NULL;
	const char* closeParen = NULL;
	while (start[0] != '\0' && isspace(start[0])) {
		start++;
	}
	openParen = start;
	while (openParen[0] != '\0' && openParen[0]!='(') {
		openParen++;
	}
	closeParen = openParen;
	while (closeParen[0] != '\0' && closeParen[0] != ')') {
		closeParen++;
	}
	if (isalpha(start[0]) 
		&& openParen[0] == '(' 
		&& closeParen[0] == ')') {
		int startlen = openParen - start;
		struct alias_plus* ap = alias_plus_get(start, startlen);
		if (ap != NULL) {
			g_tiap_ap = ap;
			return 1;
		}
	}
	g_tiap_ap = NULL;
	return 0;
}
void defered_invoke_alias_plus(const char** varsubs, int num_varsubs, int lineno, const char* fname, struct alias_plus* x)
{
	struct ap_invocation* ap_inv = ap_inv_create(x->name, -1);
	int j;
	ap_inv->argc = num_varsubs;
	ap_inv->lineno = lineno;
	ap_inv->fname = malloc(strlen(fname) + 1);
	fail2malloc(ap_inv->fname, __LINE__);
	memcpy(ap_inv->fname, fname, strlen(fname) + 1);
	ap_inv->argv = malloc(sizeof(char*) * num_varsubs);
	fail2malloc(ap_inv->argv,__LINE__);
	for (j = 0; j < num_varsubs; ++j) {
		ap_inv->argv[j] = malloc(strlen(varsubs[j]) + 1);
		fail2malloc(ap_inv->argv[j], __LINE__);
		memcpy(ap_inv->argv[j], varsubs[j], strlen(varsubs[j]) + 1);
	}
	if (!ap_inv_testifalready(ap_inv)) {
		if (ap_inv_head == NULL) {
			ap_inv_head = ap_inv;
		}
		else {
			struct ap_invocation* y = ap_inv_head;
			while (y->next != NULL) {
				y = y->next;
			}
			y->next = ap_inv;
		}
	}
	else {
		ap_inv_free(ap_inv);
	}
}
void ap_inv_applyvars(struct alias_plus* x, const char** varsubs, int num_varsubs, FILE* j, const char *fname, int lineno) {
	int i;
	struct alias_positional* ps = x->head;
	je_clear_vars();
	
		
	i = 0;
	while (ps != NULL) {
		++i;
		if (i > num_varsubs) {
			if (j != NULL) {
				fprintf(j, " %s:%d error not enough parameters supplied, we expected %d but only got %d\n",
					fname, lineno, x->numPositionals, num_varsubs);
			}
			printf(" %s:%d error not enough parameters supplied, we expected %d but only got %d\n",
				fname, lineno, x->numPositionals, num_varsubs);
			exit(__LINE__);
			return;
		}
		je_setvar(ps->varLetter, varsubs[ps->posNum - 1], -1);
		ps = ps->next;
	}
	
}

void ap_inv_proc_alsos(struct ap_invocation* z) {
	struct alias_plus* x;
	int line_jamescmdhash;
	const int hash_ADDTOLIST = jchash("JAMES_ADD_TO_LIST");
	const int hash_ALIASPLUS = jchash("JAMES_ALIAS_PLUS");
	const int hash_INVOKEALIASPLUS = jchash("JAMES_INVOKE_ALIAS_PLUS");

	/*--also duplicate these in the parse_Src and also reset it after this too--*/
	g_jatl_i = 0;
	g_jap_i = 0;
	g_jiap_i = 0;


	x = alias_plus_get(z->name,-1);

	if (x == NULL) {
		genericDie(__LINE__);
		return;
	}

	if (x->alsoPlusNum > 0) {
		int i;
		char save_fn[sizeof g_je_filename+1];
		int save_ln;
		/*--the funcs can modify the lines but we want them to remain--*/
		char temp[257];

		line_jamescmdhash = jchash(x->alsoPlus[0]);
		ap_inv_applyvars(x, z->argv, z->argc, NULL,z->fname,z->lineno);
		memcpy(save_fn, g_je_filename, sizeof g_je_filename);
		save_ln = g_je_line;

		memcpy(g_je_filename, z->fname, strlen(z->fname) + 1);
		g_je_line = z->lineno;
		
		for (i = 1; i < x->alsoPlusNum; ++i) {
			if (strlen(x->alsoPlus[i]) > sizeof temp - 1) {
				printf("error,\"%s\" is too long (max %d) (%ld)\n", x->alsoPlus[i], sizeof temp - 1, __LINE__);
				genericDie(__LINE__);
				return;
			}
			memcpy(temp, x->alsoPlus[i], strlen(x->alsoPlus[i]) + 1);
			if (line_jamescmdhash == hash_ADDTOLIST) {
				JAMES_ADD_TO_LIST(temp);
			}
			else if (line_jamescmdhash == hash_ALIASPLUS) {
				JAMES_ALIAS_PLUS(temp);
			}
			else if (line_jamescmdhash == hash_INVOKEALIASPLUS) {
				JAMES_INVOKE_ALIAS_PLUS(temp);
			}
			else {
				printf("error, %d is unrecognised hash (%s) (%ld)\n", line_jamescmdhash, x->alsoPlus[0], __LINE__);
				genericDie(__LINE__);
				return;
			}
		}
		

		memcpy(g_je_filename, save_fn, sizeof g_je_filename);
		g_je_line = save_ln;

	}


	/*--also duplicate these in the parse_Src and also reset it before this too--*/
	g_jatl_i = 0;
	g_jap_i = 0;
	g_jiap_i = 0;

}
void ap_inv_render(struct ap_invocation* z, FILE* j) {
	struct alias_plus* x;
	int num_varsubs;
	char** varsubs;
	char* fname;
	int lineno;

	if (j == NULL) {
		return;
	}
	x = alias_plus_get(z->name, -1);
	if (x == NULL) {
		printf(   "Error we can't find the alias_plus called \"%s\"\n",z->name);
		fprintf(j,"Error we can't find the alias_plus called \"%s\"\n",z->name);
		genericDie(__LINE__);
		return;
	}
	num_varsubs = z->argc;
	varsubs = z->argv;
	fname = z->fname;
	lineno = z->lineno;

	ap_inv_applyvars(x,varsubs,num_varsubs,j,fname,lineno);

	{
		char temp[350];
		char saved[sizeof g_je_filename];
		int savline;
		memcpy(temp, x->define_output_name, strlen(x->define_output_name) + 1);

		savline = g_je_line;
		memcpy(saved, g_je_filename, sizeof g_je_filename);
		g_je_line = lineno;
		memcpy(g_je_filename, fname, strlen(fname) + 1);
		james_expand(temp, NULL, NULL);
		fprintf(j, "\n/**\ninvocation of alias_plus \"%s\" from %s:%d\n*/\n#define %s \\\n",
			x->name, fname, lineno, &temp[0]);
		if (isdebug) {
			fflush(j);
		}

		/*--now output all macro lines if any--*/
		{
			struct alias_plus_macroline* ml = x->macro_lines;
			while (ml != NULL) {
				if (strlen(ml->line) > sizeof temp) {
					fprintf(j, "/*error %s is too long (max chars %d)*/\n", ml->line, sizeof temp);
					printf("/*error %s is too long (max chars %d)*/\n", ml->line, sizeof temp);
					exit(__LINE__);
					return;
				}
				memcpy(temp, ml->line, strlen(ml->line) + 1);
				james_expand(temp, NULL, NULL);
				fputs(temp, j);
				if (ml->next != NULL) {
					fputs(" \\\n", j);
				}
				else {
					fputc('\n', j);
				}
				if (isdebug()) {
					fflush(j);
				}
				ml = ml->next;
			}
		}

		g_je_line = savline;
		memcpy(g_je_filename, saved, sizeof g_je_filename);
	}
}
void call_alias_plus(const char* linebuf, const char* fname, int lineno, FILE*j,int *opt_read_upto) {
	struct alias_plus* x = g_tiap_ap;
	const char* v = linebuf;
	const char* ve = NULL;
	char** varsubs = malloc(sizeof(char*)*x->numPositionals);
	int i;
	int num_varsubs;
	while (*v != '\0' && *v != '(') {
		++v;
	}
	++v;

	/*--to enable free loops below--*/
	for (i = 0; i < x->numPositionals; ++i) {
		varsubs[i] = NULL;
	}

	i = 0;
	while (*v != '\0' && i < x->numPositionals) {
		while (*v != '\0' && isspace(*v)) {
			++v;
		}
		ve = v;
		while (*ve != '\0' && *ve != ',' 
			&& !isspace(*ve) && *ve != ')') {
			if (*ve == '"') {
				char preve = *ve;
				++ve;
				while (*ve != '"' || preve == '\\') {
					preve = *ve;
					++ve;
				}
			}
			else {
				++ve;
			}
		}
		{
			int ln = (ve - v);
			varsubs[i] = malloc(ln + 1);
			fail2malloc(varsubs[i],__LINE__);
			memcpy(varsubs[i], v, ln);
			varsubs[i][ln] = '\0';
			++i;
		}
		v = ve+1;
	}
	num_varsubs = i;

	defered_invoke_alias_plus(varsubs, num_varsubs, lineno, fname, x);
	

	
	
	
	
	
	/*
	for (i = 0; i < x->numPositionals; ++i) {
		je_setvar(,varsubs[i], -1);
	}*/

	if (opt_read_upto != NULL) {
		if (ve != NULL) {
			*opt_read_upto = ve - linebuf;
		}
		else {
			*opt_read_upto = strlen(linebuf);
		}
	}
	/*--free ram--*/
	for (i = 0; i < x->numPositionals; ++i) {
		if (varsubs[i] != NULL) {
			free(varsubs[i]);
		}
	}
	free(varsubs);
}

/**
@param j_opt
optional parameter to the james.h file handle (append mode)
@param s
required parameter to the source code file handle (read mode)
@param src_c_opt
optional parameter to allow source overrides
@return
0 on okay, otherwise the line number of where the error was tested
*/
int parse_src(FILE* j_opt, FILE* s, const char * const src_c_opt);

int parse_src(FILE* j, FILE* s, const char * const src_c) {
	int line;
	char c;
	char c_m1;
	char c_m2;
	char c_m3;
	char linebuf[LINEBUF_LEN];
	int line_whitespace_only_sofar;
	int line_ispreproc;
	int line_jamescmdhash;
	int line_havejamescmd;
	int line_maybe_james;
	int line_maybe_blockcomment;
	int line_maybe_linecomment;
	int line_is_linecomment;
	int line_is_blockcomment;
	int line_is_james;
	int line_backslash_might_be_last;
	int line_in_string;
	int james_linebuf_w;
	int line_james_written_overrides_yet;
	int line_callaliasplus_readupto;
	int line_needtostartjamesleadin_forecho;
	char echobuf[100];
	int echobuf_w;

	/*--duplicate in ap_inv_proc_alsos--*/
	const int hash_ADDTOLIST = jchash("JAMES_ADD_TO_LIST");
	const int hash_ALIASPLUS = jchash("JAMES_ALIAS_PLUS");
	const int hash_INVOKEALIASPLUS = jchash("JAMES_INVOKE_ALIAS_PLUS");
	const int test_collisions[] = { hash_ADDTOLIST,
		hash_ALIASPLUS ,
		hash_INVOKEALIASPLUS,
		-12345 };

	{
		int x;
		int end;
		int y;
		for (x = 0;; ++x) {
			if (test_collisions[x] == -12345) {
				end = x;
				break;
			}
		}
		for (y = 0; y < end; ++y) {
			for (x = 0; x < end; ++x) {
				if (x == y) {
					continue;
				}
				if (test_collisions[x] == test_collisions[y]) {
					printf("hash command %d collides with hash \
command %d (value: %d)\n",
						x, y, test_collisions[y]);
					return __LINE__;
				}
			}
		}
	}

	line = 1;
	g_je_line = line;
	c = fgetc(s);
	c_m1 = '\0';
	c_m2 = '\0';
	c_m3 = '\0';
	/*--all these need to be reset on each new line--*/
	line_whitespace_only_sofar = 1;
	line_ispreproc = 0;
	line_jamescmdhash = 0;
	line_havejamescmd = 0;
	line_maybe_james = 0;
	line_maybe_blockcomment = 0;
	line_maybe_linecomment = 0;
	line_is_linecomment = 0;
	line_is_blockcomment = 0;
	line_is_james = 0;
	line_backslash_might_be_last = 0;
	line_in_string = 0;
	james_linebuf_w = 0;
	line_james_written_overrides_yet = 0;
	line_callaliasplus_readupto = 0;
	line_needtostartjamesleadin_forecho = 0;
	while (!feof(s)) {
		if (c == '\r' || c == '\n' && c_m1 != '\r') {
			if (line_is_james) {
				/*--copy to james.h for memory--*/
				if (j != NULL) {
					fputc(c, j);
					if (isdebug()) {
						fflush(j);
					}
				}
				line_james_written_overrides_yet = 0;
			}
			line++;
			//g_je_line = line;
			/*--keep this in sync with above--*/
			line_whitespace_only_sofar = 1;
			if (!line_is_james) {
				line_jamescmdhash = 0;
			}
			line_maybe_james = 0;
			line_maybe_blockcomment = 0;
			line_callaliasplus_readupto = 0;
			line_maybe_linecomment = 0;
			if (line_backslash_might_be_last) {
				if (line_is_linecomment) {
					line_is_linecomment = 1;
				}
				else if (line_ispreproc) {
					line_ispreproc = 1;
				}
				else if (line_in_string) {
					line_in_string = 1;
				}
			}
			else {
				line_is_linecomment = 0;
				line_ispreproc = 0;
				line_in_string = 0;
			}
			line_backslash_might_be_last = 0;
			james_linebuf_w = 0;
			/*
			line_is_blockcomment = 0;
			line_is_james=0;
			*/
			if (line_is_james && !line_havejamescmd) {
				if (line > g_je_line+1) {
					line_havejamescmd = 1;
				}
			}
			else if (line_is_james && line_havejamescmd) {

				if (line_jamescmdhash == hash_ADDTOLIST) {
					/*--we can make use of linebuf--*/
					JAMES_ADD_TO_LIST(linebuf);
				}
				else if (line_jamescmdhash == hash_ALIASPLUS) {
					JAMES_ALIAS_PLUS(linebuf);
				}
				else if (line_jamescmdhash == hash_INVOKEALIASPLUS) {
					JAMES_INVOKE_ALIAS_PLUS(linebuf);
				}
				else {
					printf("line %d: error, command that \
hashes to %d is not found (%ld)\n",
line,line_jamescmdhash,__LINE__);
					if (j != NULL) {
						fprintf(j, "\n/*line %d: error, command that hashes\
 to %d is not found (%ld)*/\n",
							line, line_jamescmdhash, __LINE__);
					}
					unlock();
					return __LINE__;
				}
			}
			/*--don't process the newline char anyfurther--*/

			/*--loop continue must be copied down below as well--*/
			c_m3 = c_m2;
			c_m2 = c_m1;
			c_m1 = c;
			c = fgetc(s);
			continue;
		}


		if (line_is_blockcomment) {
			if (c == '/' && c_m1 == '*') {
				line_is_blockcomment = 0;
			}
		}
		if (line_is_james) {
			/*--copy to james.h for memory--*/
			if (j != NULL) {
				if (line_needtostartjamesleadin_forecho) {
					line_needtostartjamesleadin_forecho = 0;
					if (line_jamescmdhash == hash_ADDTOLIST
						|| line_jamescmdhash == hash_ALIASPLUS
						|| line_jamescmdhash == hash_INVOKEALIASPLUS
						) {
						/*--dont echo these please--*/
					}
					else {
						echobuf_w=0;
						/*fputc('\n', j);
						fputc('/', j);
						fputc('*', j);
						fputc('#', j);
						if (isdebug()) {
							fflush(j);
						}*/
						echobuf[echobuf_w++]='\n';
						echobuf[echobuf_w++] = '/';
						echobuf[echobuf_w++] = '*';
						echobuf[echobuf_w++] = '#';
						echobuf[echobuf_w] = '\0';
					}
				}
				if (c != '#'
					&& !(c == '*' && c_m1=='#')
					&& !(c == '/' && c_m1 == '*' && c_m2=='#')
					&& !line_james_written_overrides_yet
					&& line > (g_je_line+1)
				) {
					if (src_c != NULL) {
						if (line_jamescmdhash == hash_ADDTOLIST
							||line_jamescmdhash == hash_ALIASPLUS
							|| line_jamescmdhash == hash_INVOKEALIASPLUS) {
							/*--dont add these for alias plus--*/
						}
						else {
							if (strlen(src_c) + 10 + echobuf_w < sizeof echobuf) {
								sprintf(echobuf+echobuf_w, "@%s:%d$", src_c, g_je_line);
							}
							//fprintf(j, "@%s:%d$", src_c, g_je_line);
						}
					}
					line_james_written_overrides_yet = 1;
				}
				if (line_jamescmdhash == hash_ADDTOLIST
					|| line_jamescmdhash == hash_ALIASPLUS
					|| line_jamescmdhash == hash_INVOKEALIASPLUS
					) {
					/*--dont echo these please--*/
				}
				else {
					/*--default is to echo--*/
					if (echobuf_w < sizeof echobuf) {
						echobuf[echobuf_w++] = c;
						echobuf[echobuf_w] = '\0';
					}
					/*
					fputc(c, j);
					if (isdebug()) {
						fflush(j);
					}*/
				}
			}
			if (c == '/' && c_m1 == '*' && c_m2 == '#') {
				line_is_james = 0;
				/*--also duplicate these at the ap_inv_proc--*/
				g_jatl_i = 0;
				g_jap_i = 0;
				g_jiap_i = 0;
				line_havejamescmd = 0;
				if (line_jamescmdhash == hash_ADDTOLIST
					|| line_jamescmdhash == hash_ALIASPLUS
					|| line_jamescmdhash == hash_INVOKEALIASPLUS)
				{
					/*--no echo for these please--*/
				}
				else {
					if (strnlen(echobuf, sizeof echobuf_w + 1) > 0) {
						fputs(echobuf, j);
					}
				}
				echobuf[0] = '\0';
				echobuf_w = 0;

			}
		}


		if (c == '\\') {
			line_backslash_might_be_last = 1;
		}
		else if (!isspace(c)) {
			line_backslash_might_be_last = 0;
		}

		if (!line_is_linecomment
			&& !line_is_blockcomment
			&& !line_is_james
			&& c == '"' && c_m1 != '\\') {
			if (line_in_string) {
				line_in_string = 0;
			}
			else {
				line_in_string = 1;
			}
		}

		/*--must come before initials--*/
		if (line_maybe_linecomment) {
			if (c == '/') {
				line_is_linecomment = 1;
				line_maybe_blockcomment = 0;
				line_maybe_james = 0;
			}
			else {
				line_maybe_linecomment = 0;
			}
		}

		if (line_maybe_blockcomment == 1
			|| line_maybe_james == 1) {
			if (c == '*') {
				line_maybe_james = 2;
				line_maybe_blockcomment = 2;
			}
			else {
				line_maybe_james = 0;
				line_maybe_blockcomment = 0;
			}
		}
		else if (line_maybe_blockcomment == 2
			|| line_maybe_james == 2) {
			if (c == '#') {
				line_maybe_james = 0;
				line_is_james = 1;
				line_maybe_blockcomment = 0;
				/*--only at the start of each block, so that
				it can be used multiple times as the same
				value--*/
				g_je_line = line;
				if (j != NULL) {
					/*--write to james.h for memory--*/
					/*fputc('\n', j);
					fputc('/', j);
					fputc('*', j);
					fputc('#', j);
					if (isdebug()) {
						fflush(j);
					}*/
					line_needtostartjamesleadin_forecho = 1;
				}
			}
			else {
				line_is_blockcomment = 1;
				line_is_james = 0;
				line_maybe_blockcomment = 0;
				line_maybe_james = 0;
			}
		}

		/*--these are the initials--*/
		if (c == '/' && line_whitespace_only_sofar) {
			line_maybe_james = 1;
line_maybe_blockcomment = 1;
line_maybe_linecomment = 1;
line_ispreproc = 0;
		}
		else if (c == '#' && line_whitespace_only_sofar) {
		line_ispreproc = 1;
		}



		if (c == 'J' && c_m1 == '#'
			&& line_ispreproc && line_is_james == 0) {
			line_is_james = 5;
		}
		else if (c == 'A' && c_m1 == 'J'
			&& line_ispreproc && line_is_james == 5) {
			line_is_james = 4;
		}
		else if (c == 'M' && c_m1 == 'A'
			&& line_ispreproc && line_is_james == 4) {
			line_is_james = 3;
		}
		else if (c == 'E' && c_m1 == 'M'
			&& line_ispreproc && line_is_james == 3) {
			line_is_james = 2;
		}
		else if (c == 'S' && c_m1 == 'E'
			&& line_ispreproc && line_is_james == 2) {
			line_is_james = 1;
		}
		else if (line_is_james != 1) {
			line_is_james = 0;
		}



		if (line_is_james && !line_havejamescmd &&
			(c == '(' || c == '\r' || c == '\n')) {
			if (line > g_je_line) {
				line_havejamescmd = 1;
			}
		}
		else if (line_is_james && !line_havejamescmd) {
			if (line > g_je_line) {
				line_jamescmdhash = jchashchar(line_jamescmdhash, c);
			}
		}

		if (line_havejamescmd) {
			if (line_jamescmdhash == hash_ADDTOLIST) {

			}
			else if (line_jamescmdhash == hash_ALIASPLUS) {

			}
			else if (line_jamescmdhash == hash_INVOKEALIASPLUS) {

			}
			else {
				printf("line %d: error, command that hashes\
 to %d is not found (%ld)\n",
					line, line_jamescmdhash, __LINE__);
				if (j != NULL) {
					fprintf(j, "\n/*line %d: error, command that hashes\
 to %d is not found (%ld)*/\n",
						line, line_jamescmdhash, __LINE__);
				}
				unlock();
				return __LINE__;
			}
		}


		if (!isspace(c)) {
			line_whitespace_only_sofar = 0;
		}

		if (james_linebuf_w < sizeof(linebuf) - 1) {
			linebuf[james_linebuf_w] = c;
			linebuf[james_linebuf_w + 1] = '\0';
			++james_linebuf_w;
		}

		if (!line_is_james
			&& test_invocate_alias_plus(linebuf + line_callaliasplus_readupto)) {

			if (j != NULL) {

				//alias_plus_render
				call_alias_plus(linebuf, src_c, line, j, &line_callaliasplus_readupto);
			}
		}

		/*--loop continue must be copied up above as well--*/
		c_m3 = c_m2;
		c_m2 = c_m1;
		c_m1 = c;
		c = fgetc(s);
	}

	/*--now run all alsos from all alias_pluses from all alias_plus_invocations--*/
	{
		struct ap_invocation* y = ap_inv_head;
		while (y != NULL) {
			ap_inv_proc_alsos(y);
			y = y->next;
		}
	}
	

	/*--after having read into ram all partially built
	lists, write out into james.h--*/
	if (j != NULL) {
		fputs("\n/*--ram dump--*/\n\n", j);
	}
	if (j != NULL) {
		/*--write lists--*/
		struct list* x = first_list;
		while (x != NULL) {
			struct list_item* y;
			fprintf(j, "\n/*#\nJAMES_ADD_TO_LIST\n%s\n", x->name);
			y = x->head;
			while (y != NULL) {
				if (strnlen(y->fname, sizeof g_je_filename) > 0) {
					fprintf(j, "@%s:%d$%s\n", y->fname, y->lineno, y->value);
				}
				else {
					fprintf(j, "%s\n", y->value);
				}
				y = y->next;
			}
			fprintf(j, "#*/\n");

			x = x->next_list;
		}

		x = first_list;
		while (x != NULL) {
			struct list_item* y;

			/*--generate linked doxygen documentation--*/
			fputs("/** The tool in james.c was used to create this list from\nthe source file(s) ", j);
			{
				int printed = 0;
				y = x->head;
				while (y != NULL) {
					if (y->fname != NULL && strlen(y->fname) > 0) {
						/*--ensure it is unique / distinct list--*/
						struct list_item* z = x->head;
						int already = 0;
						while (z != NULL && z != y) {
							if (z->fname != NULL && strcmp(z->fname, y->fname) == 0) {
								already = 1;
								break;
							}
							z = z->next;
						}
						if (!already) {
							if (printed) {
								fputs(", ", j);
							}
							printed = 1;
							fputs(y->fname, j);
						}
					}
					y = y->next;
				}
			}
			fputs("*/\n", j);

			fprintf(j, "#define %s \\\n", x->name);
			y = x->head;
			while (y != NULL) {
				fprintf(j, "%s%s\n",
					y->value,
					y->next != NULL ? ",\\" : "");
				y = y->next;
			}
			fprintf(j, "\n");
			x = x->next_list;
		}
	}
	if (j != NULL) {

		/*--write alias pluses--*/
		struct alias_plus* x = alias_plus_head;
		struct alias_plus_macroline* ml = NULL;
		int i;
		while (x != NULL) {
			if (strlen(x->name) > 0) {
				fprintf(j, "\n/*#\nJAMES_ALIAS_PLUS\n%s(", x->name);
				//for (i = 0; i < x->numPositionals; ++i) {
				{
					struct alias_positional* ap = x->head;
					while (ap != NULL) {
						if (ap != x->head) {
							fputs(", ",j);
						}
						fputc('$', j);
						fputc(ap->varLetter, j);
						ap = ap->next;
					}
				}
				//}
				fprintf(j, ")\n%d\n", x->alsoPlusNum);
				for (i = 0; i < x->alsoPlusNum; ++i) {
					fputs(x->alsoPlus[i], j);
					fputc('\n', j);
				}
				fputs(x->define_output_name,j);
				fputc('\n', j);
				ml = x->macro_lines;
				while (ml != NULL) {
					fputs(ml->line, j);
					fputc('\n', j);
					ml = ml->next;
				}
				fputs("#*/\n", j);
				if (isdebug()) {
					fflush(j);
				}
			}
			x = x->next;
		}
	}


	/*--write alias_plus_invocations--*/
	if (j != NULL) {
		struct ap_invocation* x = ap_inv_head;
		while (x != NULL) {
			int i;
			fprintf(j,"\n/*#\nJAMES_INVOKE_ALIAS_PLUS\n%s\n@%s:%d$\n%d\n",
				x->name, x->fname, x->lineno, x->argc);
			for (i = 0; i < x->argc; ++i) {
				fputs(x->argv[i],j);
				fputc('\n', j);
			}
			fputs("#*/\n",j);
			x = x->next;
		}
	}

	/*--render all alias_plus_invocations--*/
	struct ap_invocation* z = ap_inv_head;
	while (z != NULL) {
		ap_inv_render(z, j);
		z = z->next;
	}
	


	/*--finish the headerfile off--*/
	if (j != NULL) {
		fwrite(headerfile_footer, 1, strlen(headerfile_footer), j);
	}

	return 0;
}


int old_parse(int doit, FILE* j) {
	char linebuf[250];
	int nextline_start_a_list = 0;
	int nextline_in_list = 0;
	char listname[100 + 1];


	if (!doit) {
		return 0;
	}

	while (!feof(j)) {
		fgets(linebuf, sizeof linebuf, j);
		if (nextline_start_a_list) {
			nextline_in_list = 1;
			nextline_start_a_list = 0;
			if (strcmp(linebuf, s_startlist_define)
				== 0) {
				int end = strlen(s_startlist_define);
				int r, w;
				int start;
				for (; end < (int)strlen(linebuf); ++end) {
					if (linebuf[end] == '\\') {
						break;
					}
				}
				if (end > 100) {
					printf("list name too long %s\n",
						linebuf);
					unlock();
					return __LINE__;
				}
				start = strlen(s_startlist_define);
				while (linebuf[start] == ' '
					&& start < (int)strlen(linebuf)) {
					start++;
				}
				if (linebuf[end] == '\\') {
					--end;
				}
				while (linebuf[end] == ' ' && end > 1) {
					--end;
				}
				for (w = 0, r = start; r < end; ++r) {
					if (w > sizeof listname) {
						printf("trying to write too \
much to list name %s\n", linebuf);
						unlock();
						return __LINE__;
					}
					listname[w++] = linebuf[r];
					listname[w] = '\0';
				}
			}
		}
		else if (nextline_in_list) {

			int i;
			char* start;
			for (i = strlen(linebuf); i >= 0; --i) {
				if (linebuf[i] == '\\') {
					nextline_in_list = 1;
				}
				else if (isalpha(linebuf[i])) {
					nextline_in_list = 0;
				}
			}

			for (i = strlen(linebuf); i >= 0; --i) {
				if (linebuf[i] == ','
					|| isalnum(linebuf[i])) {
					if (linebuf[i] == ',') {
						i--;
					}
					linebuf[i] = '\0';
					start = &linebuf[0];
					while (isspace(start[0])
						&& start[0] != '\0') {
						++start;
					}

					add_list_item(listname, start,NULL,NULL);

				}
			}

		}
		else {

			if (strncmp(linebuf, s_generated_list,
				strlen(s_generated_list)) == 0) {
				nextline_start_a_list = 1;
			}

		}
	}
	/*--after having read into ram all partially built
	lists, write out into james.h--*/
	if (j != NULL) {
		struct list* x = first_list;
		while (x != NULL) {
			struct list_item* y;
			fprintf(j, "\n/*#/n%s\n", x->name);
			y = x->head;
			while (y != NULL) {
				if (strnlen(y->fname,sizeof g_je_filename) > 0) {
					fprintf(j, "@%s:%d$%s\n", y->fname, y->lineno, y->value);
				}
				else {
					fprintf(j, "%s\n", y->value);
				}
				y = y->next;
			}
			fprintf(j, "#*/\n");
			x = x->next_list;
		}

		x = first_list;
		while (x != NULL) {
			struct list_item* y;
			fprintf(j, "#define %s \\\n", x->name);
			y = x->head;
			while (y != NULL) {
				fprintf(j, "%s%s\n",
					y->value,
					y->next!=NULL?",\\":"");
				y = y->next;
			}
			fprintf(j, "\n");
			x = x->next_list;
		}
	}
	return 0;
}

#include <time.h>
#define TIMBUFSZ 120
void gtimbuf(char* timbuf) {
	time_t t;
	struct tm tm;
	struct tm *tmp;
	time(&t);
	tmp = localtime(&t);
	memcpy(&tm, tmp, sizeof tm);
	strftime(timbuf, TIMBUFSZ, "%I:%M:%S %p %a %b %d %Y", &tm);
}
int main(int argc, char** argv){
	FILE *j;
	FILE *s;
	//int line;
	//char c;
	//char c_m1;
	//char c_m2;
	//char c_m3;
	const char * james_h;
	const char* src_c;
	//char linebuf[250];
	//int line_whitespace_only_sofar;
	//int line_ispreproc;
	//int line_isjames;
	//int line_jamescmdhash;
	//int line_havejamescmd;
	//int line_maybe_james;
	//int line_maybe_blockcomment;
	//int line_maybe_linecomment;
	//int line_is_linecomment;
	//int line_is_blockcomment;
	//int line_is_james;
	//int line_backslash_might_be_last;
	//int line_in_string;
	//int james_linebuf_w;
	char src_c_filename[100];
	
	const int hash_ADDTOLIST = jchash("JAMES_ADD_TO_LIST");
	
	if(argc != 3) {
		if (isdebug()) {
			james_h = "C:\\dev\\fsrc\\ginclude\\james.h";
			//remove(james_h);
			src_c = "C:\\dev\\fsrc\\lib\\everynms.h";
		}
		else {
			puts("usage: path\\to\\james.h source\\file.c");
			return 1;
		}
	}
	else {
		james_h = argv[1];
		src_c = argv[2];
	}
	
	
	lock();
	
	j = fopen(james_h,"r");
	if(j != NULL) {
		int subret;
		old_parse(0,j);
		subret = parse_src(NULL, j, NULL);
		fclose(j);
		if (subret) {
			unlock();
			return subret;
		}
	}
	remove(james_h);
	j = fopen(james_h,"r");
	
	if (j == NULL) {
		/*--first invocation, create and write header--*/
		j = fopen(james_h, "w");
		if (j == NULL) {
			printf("error cannot open '%s' for writing\n",
				james_h);
			unlock();
			return __LINE__;
		}
		fwrite(header, 1, strlen(header), j);
		{
			char timbuf[TIMBUFSZ+1];
			gtimbuf(&timbuf[0]);
			fprintf(j, "\n/*\nGenerated on %s\n*/\n\n", timbuf);
		}
		fclose(j);
	} else {
		/*-already exists, append--*/
		fclose(j);
	}
	j = fopen(james_h,"a");
	if(j == NULL) {
		printf("error cannot open '%s' for appending\n",
			james_h);
		unlock();
		return __LINE__;
	}
	s = fopen(src_c,"r");
	if(s == NULL) {
		printf("error cannot open '%s' for reading\n",
			src_c);
		unlock();
		return __LINE__;
	}

	/*--real parse--*/
	g_je_line = 0;
	{
		int z;
		for (z = strlen(src_c); z >= 0; --z) {
			if (src_c[z] == '/' || src_c[z] == '\\') {
				int n = strlen(src_c) - z;
				if (n + 2 > sizeof g_je_filename) {
					n = (sizeof g_je_filename)-2;
				}
				memcpy(g_je_filename , &src_c[z+1], n-1);
				g_je_filename[n] = '\0';
				break;
			}
		}
		memcpy(src_c_filename, g_je_filename, strlen(g_je_filename) + 1);
	}

	{
		int second_subret;
		second_subret = parse_src(j, s, src_c_filename);
		if (second_subret != 0) {
			fprintf(j,"\n/* (while processing file %s) */\n", src_c);
			printf( "\n/* (while processing file %s) */\n", src_c);
			unlock();
			/*--don't want to return 0, so that build system knows problem--*/
			return second_subret;
		}
	}
	

	fclose(j);
	fclose(s);
	
	unlock();
	return 0;
}
