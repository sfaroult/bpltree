#ifndef _BTPLUS_HEADER

#define _BTPLUS_HEADER

#define BTPLUS_NOT_FOUND	-1
#define BTPLUS_ADD	  0
#define BTPLUS_AUTOLIST	  1
#define BTPLUS_AUTOTREE	  2
#define BTPLUS_BYE	  3
#define BTPLUS_DEL	  4
#define BTPLUS_DISPLAY	  5
#define BTPLUS_FIND	  6
#define BTPLUS_GET	  7
#define BTPLUS_GETTIME	  8
#define BTPLUS_HELP	  9
#define BTPLUS_HUSH	 10
#define BTPLUS_ID	 11
#define BTPLUS_INS	 12
#define BTPLUS_LIST	 13
#define BTPLUS_NOID	 14
#define BTPLUS_NOTRC	 15
#define BTPLUS_QUIT	 16
#define BTPLUS_REM	 17
#define BTPLUS_SCAN	 18
#define BTPLUS_SCANTIME	 19
#define BTPLUS_SEARCH	 20
#define BTPLUS_SHOW	 21
#define BTPLUS_STOP	 22
#define BTPLUS_TRC	 23

#define BTPLUS_COUNT	24

extern int   btplus_search(char *w);
extern char *btplus_keyword(int code);

#endif
