#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "btplus.h"

static char *G_btplus_words[] = {
    "add",
    "autolist",
    "autotree",
    "bye",
    "del",
    "display",
    "find",
    "get",
    "gettime",
    "help",
    "hush",
    "id",
    "ins",
    "list",
    "noid",
    "notrc",
    "quit",
    "rem",
    "scan",
    "scantime",
    "search",
    "show",
    "stop",
    "trc",
    NULL};

extern int btplus_search(char *w) {
  int start = 0;
  int end = BTPLUS_COUNT - 1;
  int mid;
  int pos = BTPLUS_NOT_FOUND;
  int comp;

  if (w) {
    while(start<=end){
      mid = (start + end) / 2;
      if ((comp = strcasecmp(G_btplus_words[mid], w)) == 0) {
         pos = mid;
         start = end + 1;
       } else if ((mid < BTPLUS_COUNT - 1)
               && ((comp = strcasecmp(G_btplus_words[mid+1], w)) == 0)) {
         pos = mid+1;
         start = end + 1;
      } else {
        if (comp < 0) {
           start = mid + 1;
        } else {
           end = mid - 1;
        }
      }
    }
  }
  return pos;
}

extern char *btplus_keyword(int code) {
  if ((code >= 0) && (code < BTPLUS_COUNT)) {
    return G_btplus_words[code];
  } else {
    return (char *)NULL;
  }
}

