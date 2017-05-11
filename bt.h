#ifndef BT_HEADER

#define BT_HEADER

#define BT_NOT_FOUND	-1
#define BT_ADD	  0
#define BT_AUTOLIST	  1
#define BT_AUTOTREE	  2
#define BT_BYE	  3
#define BT_DEL	  4
#define BT_DISPLAY	  5
#define BT_FIND	  6
#define BT_HELP	  7
#define BT_HUSH	  8
#define BT_ID	  9
#define BT_INS	 10
#define BT_LIST	 11
#define BT_NOID	 12
#define BT_NOTRC	 13
#define BT_QUIT	 14
#define BT_REM	 15
#define BT_SEARCH	 16
#define BT_SHOW	 17
#define BT_STOP	 18
#define BT_TRC	 19

#define BT_COUNT	20

extern int   bt_search(char *w);
extern char *bt_keyword(int code);

#endif
