/* ---------------------------------------------------------------- *
 *
 *             bpltree_search.c
 *
 *  Search in a unique B-tree
 *
 * ---------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "bpltree.h"
#include "bpltree_err.h"
#include "debug.h"

#define  MAX_FIELDS    32

static void find_smallest_loc(NODE_T *n, KEYLOC_T *locptr) {
    // Return the position of the smallest key in the index 
    // No need for recursion
    // debug(2, ">> find_smallest_loc");
    if (n && locptr) {
      while (!_is_leaf(n)) {
        n = n->node.internal.k[0].bigger;
      }
      // Got it
      locptr->n = n;
      locptr->pos = 0;
    }
    // debug(2, "<< find_smallest_loc");
}

static void find_key_loc(NODE_T *n, char *key, KEYLOC_T *locptr, short lvl) {
    short     i = 1;
    int       cmp = -1;

    // debug(lvl, ">> find_key_loc");
    // Find the leaf node where the key should be stored
    if (n && key && locptr) {
      debug(lvl, "searching node %hd", n->id);
      if (_is_leaf(n)) {
        i = 0;
        while ((i < n->keycnt)
               && ((cmp = bpltree_keycmp(key,
                                         n->node.leaf.k[i].key,
                                         KEYSEP)) > 0)) {
          i++;
        }
        if (cmp == 0) {
          // We've found it in the tree
          debug(lvl, "** found at position %hd", i);
          locptr->n = n;
          locptr->pos = i;
          // debug(lvl, "<< find_key_loc");
          return;
        } else {
          // Not found
          locptr->n = NULL;
          locptr->pos = 0;
          // debug(lvl, "<< find_key_loc");
          return;
        }
      } else {
        // Internal node
        while ((i <= n->keycnt)
               && ((cmp = bpltree_keycmp(key,
                                         n->node.internal.k[i].key,
                                         KEYSEP)) > 0)) {
          i++;
        }
        if (cmp > 0) { // Key searched is bigger than
                       // last key in the node
          find_key_loc(n->node.internal.k[n->keycnt].bigger,
                       key, locptr, lvl+2);
        } else {
          find_key_loc(n->node.internal.k[i-1].bigger, key, locptr, lvl+2);
        }
      }
    }
    // debug(lvl, "<< find_key_loc");
}

extern KEYLOC_T bpltree_find_key(char *key) {
    KEYLOC_T  loc = {NULL, 0};

    // debug(0, ">> bpltree_find_key");
    if (key) {
      debug(0, "looking for key location");
      find_key_loc(bpltree_root(), key, &loc, 0);
    } else {
      debug(0, "looking for smallest key location");
      find_smallest_loc(bpltree_root(), &loc);
    }
    // debug(0, "<< bpltree_find_key");
    return loc;
}

// The following function is merely to display the search path
static char search_tree(char *key, NODE_T *t, short lvl) {
   // Returns 1 if found, 0 if not
   char ret = 0;
   int  i;
   int  cmp = -1;

   if (key && t) {
     for (i = 0; i < lvl; i++) {
       putchar(' ');
     }
     printf("[node %hd] ", t->id);
     if (!_is_leaf(t)) {
       i = 1;
       while ((i <= t->keycnt)
              && ((cmp = bpltree_keycmp(key,
                                        t->node.internal.k[i].key,
                                        KEYSEP)) > 0)) {
         if (i > 1) {
           putchar(',');
         }
         if (bpltree_numeric()) {
           printf("%d", (int)(t->node.internal.k[i].key));
         } else {
           printf("%s", t->node.internal.k[i].key);
         }
         i++;
       }
       putchar('\n');
       return search_tree(key, t->node.internal.k[i-1].bigger, lvl+2);
     } else {
       // Leaf
       printf("LEAF-");
       i = 0;
       while ((i < t->keycnt)
              && ((cmp = bpltree_keycmp(key,
                                        t->node.leaf.k[i].key,
                                        KEYSEP)) > 0)) {
         if (i > 0) {
           putchar(',');
         }
         if (bpltree_numeric()) {
           printf("%d", (int)(t->node.leaf.k[i].key));
         } else {
           printf("%s", t->node.leaf.k[i].key);
         }
         i++;
       }
       if (cmp == 0) {
         printf("\n*** FOUND (");
         if (bpltree_numeric()) {
           printf("%d", (int)(t->node.leaf.k[i].key));
         } else {
           printf("%s", t->node.leaf.k[i].key);
         }
         printf(", %ld) ***\n", (long)t->node.leaf.k[i].pos);
         ret = 1;
       } else {
         printf("\n*** NOT FOUND ***\n");
       }
     }
   } else {
     printf("\n*** NOT FOUND ***\n");
   }
   return ret;
}

extern void   bpltree_search(char *key) {
  int     numkey;
  NODE_T *root = bpltree_root();
  char   *p;

  if (key) {
    printf("Search path:\n");
    if (bpltree_numeric()) {
      numkey = atoi(key);
      (void)search_tree((char *)&numkey, root, 0);
    } else {
      while ((p = strchr(key, ':')) != NULL) {
        *p = bpltree_filesep();
      }
      (void)search_tree(key, root, 0);
    }
  }
}

#define BUFFER_SIZE    2048

extern int bpltree_get(char *key, FILE *fp, char show_data) {
  int        numkey;
  int        numkey2;
  long       offset;
  char      *p;
  int        len;
  char      *low_key = NULL;
  char      *high_key = NULL;
  KEYLOC_T   loc;
  NODE_T    *n;
  char       buffer[BUFFER_SIZE];
  short      i;
  int        count = 0;

  if (key && fp) {
    // Support of range scans: a, b - a to b, inclusive
    //                         ,b   - smaller than b or equal
    //                         a,   - greater than b or equal
    //  Search for one key is implemented as from key to key ...
    if (*key == ',') {
      // Upper-bound scan
      key++;
      while (*key && isspace(*key)) {
        key++;
      }
      if (*key == '\0') {
        printf("No key specified\n");
        return -1;
      }
      if (bpltree_numeric()) {
        if (sscanf(key, "%d", &numkey) != 1) {
          printf("Numeric value expected\n");
          return -1;
        }
        high_key = (char *)&numkey;
      } else {
        high_key = key;
      }
    } else {
      if ((p = strchr(key, ',')) != NULL) {
        len = strlen(key);
        if (key[len - 1] == ',') {
          // Lower-bound scan
          *p = '\0';
          if (bpltree_numeric()) {
            if (sscanf(key, "%d", &numkey) != 1) {
              printf("Numeric value expected\n");
              return -1;
            }
            low_key = (char *)&numkey;
          } else {
            low_key = key;
          }
         } else {
          // Bound scan
          *p++ = '\0';
          while (isspace(*p)) {
            p++;
          }
          if (bpltree_numeric()) {
            if (sscanf(key, "%d", &numkey) != 1) {
              printf("Numeric values expected\n");
              return -1;
            }
            if (sscanf(key, "%d", &numkey2) != 1) {
              printf("Numeric values expected\n");
              return -1;
            }
            low_key = (char *)&numkey;
            high_key = (char *)&numkey2;
          } else {
            low_key = key;
            high_key = p;
          }
        }
      } else {
        // Single key search
        if (bpltree_numeric()) {
          if (sscanf(key, "%d", &numkey) != 1) {
            printf("Numeric value expected\n");
            return -1;
          }
          low_key = (char *)&numkey;
          high_key = (char *)&numkey;
        } else {
          low_key = key;
          high_key = key;
        }
      }
    }
    if (bpltree_numeric()) {
      if (low_key) {
         if (high_key) {
           debug(0, "range scan from %d to %d",
                   (int)low_key, (int)high_key);
         } else {
           debug(0, "range scan from %d to greatest",
                   (int)low_key);
         }
      } else {
           debug(0, "range scan from smallest to %d",
                   (int)high_key);
      }
    } else {
      debug(0, "range scan from %s to %s",
            (low_key ? low_key : "smallest"),
            (high_key ? high_key : "greatest"));
    }
    loc = bpltree_find_key(low_key);
    if (loc.n) {
      n = loc.n;
      i = loc.pos;
      offset = 0;
      if (high_key) {
        while ((offset >= 0)
               && (bpltree_keycmp(high_key,
                                  n->node.leaf.k[i].key,
                                  KEYSEP) >= 0)) {
          offset = n->node.leaf.k[i].pos;
          (void)fseek(fp, offset, SEEK_SET);
          if (fgets(buffer, BUFFER_SIZE, fp)) {
            len = strlen(buffer);
            while (len && isspace(buffer[len-1])) {
              len--;
            }
            buffer[len] = '\0';
            if (show_data) {
              printf("%s\n", buffer);
            }
            count++;
          } else {
            perror("File reading:");
            return -1;
          }
          i++;
          if (i == n->keycnt) {
            i = 0;
            if ((n = n->node.leaf.next) == NULL) {
              offset = -1;
            }
          }
        }
      } else {
        // No upper bound - read all entries
        while (offset >= 0) {
          offset = n->node.leaf.k[i].pos;
          (void)fseek(fp, offset, SEEK_SET);
          if (fgets(buffer, BUFFER_SIZE, fp)) {
            len = strlen(buffer);
            while (len && isspace(buffer[len-1])) {
              len--;
            }
            buffer[len] = '\0';
            if (show_data) {
              printf("%s\n", buffer);
            }
            count++;
          } else {
            perror("File reading:");
            return -1;
          }
          i++;
          if (i == n->keycnt) {
            i = 0;
            if ((n = n->node.leaf.next) == NULL) {
              offset = -1;
            }
          }
        }
      }
    }
  }
  return count;
}

static short prepare_scan(char *key, short *fields) {
    // Modifies the key
    short  ret = 0;
    char  *k;
    char  *k2;
    char  *p;
    char  *q;
    char   sep = bpltree_filesep();
    char   strsep[2];
    short  i = 0;
    short  n;
    short  def_n = -1;
    int    len;
    int    len2;

    // We number from 0 to n-1 internally
    if (key && fields) {
     /*
      if (bpltree_numeric()) {
        debug(0, "prepare_scan - key = %d", *((int *)key));
      } else {
        debug(0, "prepare_scan - key = %s", key);
      }
      */
      strsep[0] = sep;
      strsep[1] = '\0';
      k = key;
      if (strchr(k, ':')) {
        while ((p = strchr(k, ':')) != NULL) {
          // debug(0, "prepare_scan - part %hd of the key", i + 1);
          if (bpltree_numeric()) {
            bpltree_err_seterr(BPLT_ERR_NUMKO, NULL);
            return -1;
          }
          *p = '\0';  // Temporary
          // debug(0, "prepare_scan - processing = %s", k);
          if ((q = strchr(k, '@')) != NULL) {
            if (fields[i] != -1) {
              // Fields must be specified only once
              bpltree_err_seterr(BPLT_ERR_FIELDSPEC, NULL);
              return -1;
            }
            *q++ = '\0';
            if (sscanf(q, "%hd", &n) == 1) {
              // debug(0, "prepare_scan - field %hd", n);
              fields[i++] = n - 1;
              def_n = n - 1;
            } else {
              bpltree_err_seterr(BPLT_ERR_INVSPEC, NULL);
              return -1;
            }
            // Now suppress the '@' bit
            k2 = p + 1;  // Beginning of next field in the
                         // composite key
            len = strlen(key);
            len2 = strlen(k2);
            // (below) replace ':' with the file separator
            strncat(key, strsep, BUFFER_SIZE - len);
            // (below) concatenate the remainder of the key
            // File separator is a single char
            (void)memcpy(&(key[len+1]), k2, len2);
          } else {
            // No position provided
            def_n++;
            if (fields[i] == -1) {
              // debug(0, "prepare_scan - field %hd (def)", def_n + 1);
              fields[i++] = def_n;
            }
            *p = sep;  // Restore
          }
          // Restart from after the separator (always one char)
          k = p + 1;
        }
      }
      // debug(0, "prepare_scan - part %hd of the key", i + 1);
      // debug(0, "prepare_scan - processing = %s", k);
      if ((q = strchr(key, '@')) != NULL) {
        if (fields[i] != -1) {
          // Fields must be specified only once
          bpltree_err_seterr(BPLT_ERR_FIELDSPEC, NULL);
          return -1;
        }
        *q++ = '\0';
        if (sscanf(q, "%hd", &n) == 1) {
          // debug(0, "prepare_scan - field %hd", n);
          fields[i++] = n - 1;
          def_n = n - 1;
        } else {
          bpltree_err_seterr(BPLT_ERR_INVSPEC, NULL);
          return -1;
        }
      } else {
        def_n++;
        if (fields[i] == -1) {
          // debug(0, "prepare_scan - field %hd", def_n + 1);
          fields[i++] = def_n;
        }
      }
      // debug(0, "prepare_scan - key now is  [%s]", key);
    }
    return ret;
}

static char *rebuild_line(char *line, short *fields) {
  // Not supposed to be multi threaded ...
  static char   good_bits[BUFFER_SIZE];
         char  *l;
         short  i;
         short  j;
         char  *chunks[MAX_FIELDS];
         char  *p;
         char  *q;
         char   sep = bpltree_filesep();
         char   strsep[2];
         int    len;

  assert(line && fields);
  l = strdup(line);
  assert(l);
  strsep[0] = sep;
  strsep[1] = '\0';
  p = l;
  i = 0;
  while ((q = strchr(p, sep)) != NULL) {
    *q++ = '\0';
    chunks[i++] = p;
    p = q;
  }
  // Don't forget the last one
  chunks[i] = p;
  // OK, now rebuild the line with only what we want
  good_bits[0] = '\0';
  len = 0;
  j = 0;
  while ((j < MAX_FIELDS) && (fields[j] >= 0)) {
    if (j <= i) {
      if (j > 0) {
        strncat(good_bits, strsep, BUFFER_SIZE - len);
      }
      strncat(good_bits, chunks[fields[j]], BUFFER_SIZE - len - 1);
      len = strlen(good_bits);
    }  // Else do nothing. Perhaps the last field is missing on a line
    j++;
  }
  free(l);
  while (len
         && (good_bits[len-1] != sep)
         && isspace(good_bits[len-1])) {
    len--;
  }
  good_bits[len] = '\0';
  return good_bits;
}

extern int bpltree_scan(char *key, FILE *fp, char show_data) {
  // Search without using the index
  char      *p;
  int        len;
  char      *low_key = NULL;
  char      *high_key = NULL;
  char       buffer[BUFFER_SIZE];
  short      fields[MAX_FIELDS];
  char      *good_bits;
  char       show = 0;
  int        keylen;
  int        low_keylen;
  int        high_keylen;
  int        linelen;
  int        hcmp;
  int        lcmp;
  char       sep = bpltree_filesep();
  int        count = 0;

  if (key && fp) {
    (void)memset(fields, -1, sizeof(short) * MAX_FIELDS);
    rewind(fp);
    // Support of range scans: a, b - a to b, inclusive
    //                         ,b   - smaller than b or equal
    //                         a,   - greater than b or equal
    if (*key == ',') {
      // Upper-bound scan
      key++;
      while (*key && isspace(*key)) {
        key++;
      }
      if (*key == '\0') {
        printf("No key specified\n");
        return -1;
      }
      high_key = key;
      if (prepare_scan(high_key, fields) == -1) {
        printf("%s\n", bpltree_err_msg());
        return -1;
      }
    } else {
      if ((p = strchr(key, ',')) != NULL) {
        len = strlen(key);
        if (key[len - 1] == ',') {
          // Lower-bound scan
          *p = '\0';
          low_key = key;
          if (prepare_scan(low_key, fields) == -1) {
            printf("%s\n", bpltree_err_msg());
            return -1;
          }
        } else {
          // Bound scan
          *p++ = '\0';
          while (isspace(*p)) {
            p++;
          }
          low_key = key;
          high_key = p;
          if (prepare_scan(low_key, fields) == -1) {
            printf("%s\n", bpltree_err_msg());
            return -1;
          }
          if (prepare_scan(high_key, fields) == -1) {
            printf("%s\n", bpltree_err_msg());
            return -1;
          }
        }
      } else {
        // Single key search
        if (prepare_scan(key, fields) == -1) {
          printf("%s\n", bpltree_err_msg());
          return -1;
        }
        keylen = strlen(key);
       /*
        debug(0, "keylen = %d", keylen);
        if (debugging()) {
          int i;
          for (i = 0; i < MAX_FIELDS; i++) {
            if (i) {
              debug_no_nl(0, ",");
            }
            debug_no_nl(0, "%hd", fields[i]);
          }
          debug(0, "");
        }
        */
        while (fgets(buffer, BUFFER_SIZE, fp)) {
          p = buffer;
          while (*p && (*p != sep) && isspace(*p)) {
            p++;
          }
          good_bits = rebuild_line(p, fields);
          linelen = strlen(good_bits);
          //debug(0, "[%s] (%d)", good_bits, strlen(good_bits));
          if ((keylen && (strncmp(key, good_bits, linelen) == 0))
              || (!keylen && !*good_bits)) {
            // Found 
            len = strlen(p);
            while (isspace(p[len-1])) {
              len--;
            }
            p[len] = '\0';
            if (show_data) {
              printf("%s\n", p);
            }
            count++;
          }
        }
        return count;
      }
    }
    // Range scans here
    if (bpltree_numeric()) {
      if (low_key) {
        if (high_key) {
          debug(0, "range scan from %d to %d",
                  (int)low_key, (int)high_key);
        } else {
          debug(0, "range scan from %d to greatest",
                  (int)low_key);
        }
      } else {
        debug(0, "range scan from smallest to %d",
                 (int)high_key);
      }
    } else {
      debug(0, "range scan from %s to %s",
            (low_key ? low_key : "smallest"),
            (high_key ? high_key : "greatest"));
    }
    if (low_key) {
      low_keylen = strlen(low_key);
    }
    if (high_key) {
      high_keylen = strlen(high_key);
    }
    /*
    if (debugging()) {
      int i;
      for (i = 0; i < MAX_FIELDS; i++) {
        if (i) {
          debug_no_nl(0, ",");
        }
        debug_no_nl(0, "%hd", fields[i]);
      }
      debug(0, "");
    }
    */
    while (fgets(buffer, BUFFER_SIZE, fp)) {
      show = 0;
      p = buffer;
      while (*p && (*p != sep) && isspace(*p)) {
        p++;
      }
      good_bits = rebuild_line(p, fields);
      if (low_key) {
        lcmp = strncmp(good_bits, low_key, low_keylen);
        if (lcmp >= 0) {
          if (high_key) {
            // Bounded
            hcmp = strncmp(good_bits, high_key, high_keylen);
            if ((lcmp >= 0) && (hcmp <= 0)) {
              show = 1;
            } 
          } else {
            if (lcmp >= 0) {
              show = 1;
            }
          }
        }
      } else {
        hcmp = strncmp(good_bits, high_key, high_keylen);
        if (hcmp <= 0) {
          show = 1;
        }
      }
      if (show) {
        count++;
        if (show_data) {
          len = strlen(p);
          while (isspace(p[len-1])) {
            len--;
          }
          p[len] = '\0';
          printf("%s\n", p);
        }
      }
    }
  }
  return count;
}
