#include <stdio.h>
#include <string.h>

#include "bpltree_err.h"

static char  G_info[ERR_INFO_LEN] = "";

#define BPLT_ERR_CNT    6

static char *G_bplt_err[] = {"No error",
                             "Duplicate key",
                             "Invalid number",
                             "Composite keys unsupported with numerical trees",
                             "Field position must be given for one key only",
                             "Invalid field position"
                            };
static short G_last_error = BPLT_ERR_NONE;

extern short bpltree_err(void) {
  return G_last_error;
}

extern void bpltree_err_reset(void) {
  G_last_error = BPLT_ERR_NONE;
  G_info[0] = '\0';
}

extern void bpltree_err_seterr(short code, char *info) {
  if ((code >= 0) && (code < BPLT_ERR_CNT)) {
    G_last_error = code;
    if (info) {
      strncpy(G_info, info, ERR_INFO_LEN);
    } else {
      G_info[0] = '\0';
    }
  }
}

extern char *bpltree_err_msg(void) {
  return G_bplt_err[G_last_error]; 
}

extern char *bpltree_err_info(void) {
  return G_info; 
}
