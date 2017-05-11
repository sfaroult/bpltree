#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "bt.h"

static char *G_bt_words[] = {
    "add",
    "autolist",
    "autotree",
    "bye",
    "del",
    "display",
    "find",
    "help",
    "hush",
    "id",
    "ins",
    "list",
    "noid",
    "notrc",
    "quit",
    "rem",
    "search",
    "show",
    "stop",
    "trc",
    NULL};

extern int bt_search(char *w) {
  int start = 0;
  int end = BT_COUNT - 1;
  int mid;
  int pos = BT_NOT_FOUND;
  int comp;

  if (w) {
    while(start<=end){
      mid = (start + end) / 2;
      if ((comp = strcasecmp(G_bt_words[mid], w)) == 0) {
         pos = mid;
         start = end + 1;
       } else if ((mid < BT_COUNT)
               && ((comp = strcasecmp(G_bt_words[mid+1], w)) == 0)) {
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

extern char *bt_keyword(int code) {
  if ((code >= 0) && (code < BT_COUNT)) {
    return G_bt_words[code];
  } else {
    return (char *)NULL;
  }
}

