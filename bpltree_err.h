#ifndef BPLTREE_ERR_H

#define BPLTREE_ERR_H

#define ERR_INFO_LEN       250

#define BPLT_ERR_NONE       0
#define BPLT_ERR_DUPL       1
#define BPLT_ERR_INVNUM     2
#define BPLT_ERR_NUMKO      3
#define BPLT_ERR_FIELDSPEC  4
#define BPLT_ERR_INVSPEC    5 

extern short  bpltree_err(void);
extern void   bpltree_err_reset(void);
extern void   bpltree_err_seterr(short code, char *info);
extern char  *bpltree_err_msg(void);
extern char  *bpltree_err_info(void);

#endif
