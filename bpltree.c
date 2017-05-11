#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "bpltree.h"
#include "bpltree_err.h"
#include "btplus.h"
#include "debug.h"

#define LINE_LEN          2048
#define MAX_FIELDS          32
#define FIELD_DSC          3 * MAX_FIELDS
#define KEY_MAXLEN         250
#define OPTIONS      "s:xenqdk:f:" 

#define SHOW_NOTHING         0
#define SHOW_TREE            1
#define SHOW_LIST            2

static char G_extended = 0;
static char G_id = 1;
static char G_echo = 0;
static char G_prompt = 1;

static KEY_POS_T read_key(FILE *input, char *fields) {
    char       buffer[KEY_MAXLEN +1];
    char       idxkey[KEY_MAXLEN +1];
    char      *fielddsc;
    char      *buff;
    char      *p;
    int        len;
    KEY_POS_T  keypos;
    char      *q;
    char      *f[MAX_FIELDS];
    char       sep[2];
    char       keysep[2];
    int        i;
    int        j;
    int        k;

    keypos.pos = ftello(input);
    keypos.key = NULL;
    if (fgets(buffer, KEY_MAXLEN + 1, input) != NULL) {
      p = buffer;
      while (isspace(*p)
             && ((fields == NULL) || (*p != bpltree_filesep()))) {
        p++;
      }
      if (fields) {
        fielddsc = strdup(fields);
        assert(fielddsc);
        buff = strdup(buffer);
        assert(buff);
        idxkey[0] = '\0';
        sep[0] = bpltree_filesep();
        sep[1] = '\0';
        keysep[0] = KEYSEP;
        keysep[1] = '\0';
        // First split
        q = strsep(&buff, sep);
        i = 0;
        while (q && (i < MAX_FIELDS)) {
          f[i] = q;
          i++;
          q = strsep(&buff, sep);
        }
        free(buff);
        // Rebuild a key
        q = strsep(&fielddsc, ",");
        j = 0;
        while (q) {
          if (j) {
            strncat(idxkey, keysep, KEY_MAXLEN + 1 - strlen(idxkey));
          }
          k = atoi(q);
          if ((k > 0) && (k < i)) {
            strncat(idxkey, f[k-1], KEY_MAXLEN + 1 - strlen(idxkey));
          }
          j++;
          q = strsep(&fielddsc, ",");
        }
        free(fielddsc);
        p = idxkey;
        len = strlen(p);
        while (len && isspace(p[len-1])) {
          len--;
        }
      } else {
        // Default - what comes first in the line
        if ((q = strchr(p, bpltree_filesep())) != NULL) {
          *q = '\0';
          len = strlen(p);
        } else {
          bpltree_setfilesep('\n');
          len = strlen(p);
          while (len && isspace(p[len-1])) {
            len--;
          }
        }
      }
    }
    if (len) {
      p[len] = '\0';
      keypos.key = strdup(p);
    }
    return keypos;
}

static void  list_leaf(NODE_T *n) {
    short i;

    if (n && _is_leaf(n)) {
      for (i = 1; i <= n->keycnt; i++) {
        if (n->node.leaf.k[i].key) {
          if (bpltree_numeric()) {
            printf("(%d", *((int*)(n->node.leaf.k[i].key)));
          } else {
            printf("(%s", n->node.leaf.k[i].key);
          }
        }
        printf(", %lu)\n", (unsigned long)n->node.leaf.k[i].pos);
        list_leaf(n->node.leaf.next);
      }
    }
}

static void list(void) {
   NODE_T *n = bpltree_root();

   while (n && !_is_leaf(n)) {
     n = n->node.internal.k[0].bigger;
   }
   list_leaf(n);
}

extern void  bpltree_show_node(NODE_T *n, short indent) {
   short i;

   assert(n);
   if (G_id) {
     printf("%3d-", n->id);
   }
   putchar('[');
   if (!n->is_leaf) {
     if (!G_extended) {
       for (i = 1; i <= n->keycnt; i++) {
         if (i > 1) {
           putchar(' ');
         }
         if (bpltree_numeric()) {
           printf("%d", *((int*)(n->node.internal.k[i].key)));
         } else {
           printf("%s", n->node.internal.k[i].key);
         }
         if (i < n->keycnt) {
           putchar(',');
         }
       }
     } else {
       // Internal node, not extended
       for (i = 0; i <= bpltree_maxkeys(); i++) {
         if (n->node.internal.k[i].key) {
           if (bpltree_numeric()) {
             printf("%d", *((int*)(n->node.internal.k[i].key)));
           } else {
             printf("%s", n->node.internal.k[i].key);
           }
         } else {
           if (i) {
             putchar('*');
           }
         }
         if (n->node.internal.k[i].bigger) {
           if (G_id) {
             printf("<%hd>", (n->node.internal.k[i].bigger)->id);
           } else {
             putchar(':');
           }
         } else {
           putchar('~');
         }
       }
       if (n->parent) {
         printf("(^%hd)", (n->parent)->id);
       }
     }
   } else {
     // Leaf node
     short max_shown = (G_extended ? bpltree_maxkeys() : n->keycnt);
     for (i = 0; i < max_shown; i++) {
       if (i > 0) {
         if (indent) {
           for (short k = 0; k < indent; k++) {
             putchar(' ');
           }
         }
         if (G_id) {
           for (short k = 0; k < 4; k++) {
             putchar(' ');
           }
         }
         putchar(' ');  // For the square bracket
       }
       if (bpltree_numeric()) {
         printf("%d", *((int*)(n->node.leaf.k[i].key)));
       } else {
         printf("%s", n->node.leaf.k[i].key);
       }
       printf("\t%010lu", (unsigned long)(n->node.leaf.k[i].pos));
       if ((i < max_shown-1) || G_extended) {
         putchar('\n');
       }
     }
     if (G_extended && n->parent) {
       if (indent) {
         for (short k = 0; k <= (indent + (G_id ? 4 : 0)); k++) {
           putchar(' ');
         }
       }
       printf("(^%hd)", (n->parent)->id);
     }
     if (G_extended && _is_leaf(n)) {
       if (n->node.leaf.next) {
         printf("(->%hd)", (n->node.leaf.next)->id);
       } else {
         printf("(->*)");
       }
     }
   }
   printf("]\n");
   fflush(stdout);
}

extern void  bpltree_display(NODE_T *n, int blanks) {
  if (n) {
    int  i;

    for (i = 1; i <= blanks; i++) {
      putchar(' ');
    }
    bpltree_show_node(n, (n->is_leaf ? blanks : 0));
    if (!n->is_leaf) {
      for (i = 0; i <= n->keycnt; i++) {
        bpltree_display(n->node.internal.k[i].bigger, blanks + 3);
      }
    }
  }                             /* End of if */
  fflush(stdout);
}                               /* End of bpltree_display() */


static void usage(char *prog) {
   fprintf(stdout, "Usage: %s [flags] [text file]\n", prog);
   fprintf(stdout, "The text file is indexed if present.\n");
   fprintf(stdout, "  Flags:\n");
   fprintf(stdout,
           "    -k <n>       : store at most <n> keys per node (default %d)\n",
           bpltree_maxkeys());
   fprintf(stdout,
       "    -x           : extended display - show links and empty slots\n");
   fprintf(stdout,
       "    -q           : quiet; don't display tree after changes\n");
   fprintf(stdout, "    -e           : echo value added/removed\n");
   fprintf(stdout,
       "    -n           : numeric values (incompatible with -f with several fields)\n");
   fprintf(stdout, "    -s <sep>     : field separator (single character)\n");
   fprintf(stdout, "                   in the file (defaults to tab)\n");
   fprintf(stdout,
       "    -f n[,m..]   : fields to index in the file, separated\n");
   fprintf(stdout, "                   by commas (no space). Leftmost is 1.\n");
   fprintf(stdout,
       "                   Multiple fields are incompatible with -n.\n");
}

int main(int argc, char **argv) {
  KEY_POS_T keypos;
  int       ch;
  FILE     *fp;
  int       preloaded = 0;
  char      feedback = SHOW_TREE;
  int       maxkeys;
  char      read_cmd = 1;
  char      line[LINE_LEN];
  char      fname[FILENAME_MAX];
  char     *p;
  char     *q;
  char     *q2;
  int       len;
  int       kw;
  char      ok;   // Flag
  clock_t   begin;
  clock_t   end;
  char     *fields = NULL;
  char      sep;
  int       rows;

  while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
    switch (ch) {
      case 'x':
        G_extended = 1;
        break;
      case 'e':
        G_echo = 1;
        break;
      case 'd':
        debug_on();
        break;
      case 'q':
        feedback = SHOW_NOTHING;
        G_prompt = 0;
        break;
      case 'n':
        if (fields && strchr(fields, ',')) {
          printf("Multiple fields are incompatible with -n\n");
          exit(1);
        }
        bpltree_setnumeric();
        break;
      case 'f':
        // Quick check
        p = optarg;
        if (isdigit(*p)) {
          while (*p && (isdigit(*p) || (*p == ','))) {
            if ((*p == ',') && bpltree_numeric()) {
              printf("Multiple fields are incompatible with -n\n");
              exit(1);
            }
            p++;
          }
        }
        if (*p) {
          printf("Invalid field specification - comma-separated list of numbers expected\n");
          exit(1);
        }
        fields = strdup(optarg);
        break;
      case 's':
        if (!sscanf(optarg, "%c", &sep)) {
          printf("Invalid separator\n");
          exit(1);
        }
        bpltree_setfilesep(sep);
        break;
      case 'k':
        if (!sscanf(optarg, "%d", &maxkeys)) {
          printf("Invalid max number of keys - using %d\n", bpltree_maxkeys());
        }
        bpltree_setmaxkeys(maxkeys);
        break;
      case '?':
      default:
        usage(argv[0]);
        exit(1);
    }
  }
  argc -= optind;
  argv += optind;
  if (argc) {
    strncpy(fname, argv[0], FILENAME_MAX);
    if ((fp = fopen(fname, "r")) != NULL) {
      keypos = read_key(fp, fields);
      while (keypos.key != NULL) {
        if (bpltree_insert(keypos.key, keypos.pos)) {
          fprintf(stderr, "%s : %s\n",
                  bpltree_err_msg(), bpltree_err_info());
          bpltree_free();
          exit(1);
        }
        preloaded++;
        keypos = read_key(fp, fields);
      }
    }
  } else {
    perror(fname);
  } 
  if (fields) {
    free(fields);
    fields = NULL;
  }
  debug_off(); // In case it was turned-on for preload
  if (preloaded) {
    printf("Indexed rows: %d\n", preloaded);
  }
  printf("Enter \"help\" for available commands.\n");
  while (read_cmd) {
    if (G_prompt) {
      if (debugging()) {
        printf("DBG B+TREE> ");
      } else {
        printf("B+TREE> ");
      }
    }
    if (fgets(line, LINE_LEN, stdin) == NULL) {
      bpltree_free();
      printf("Goodbye\n");
      break;
    }
    p = line;
    while(isspace(*p)) {
      p++;
    }
    len = strlen(p);
    while (len && isspace(p[len-1])) {
      len--;
    }
    if (len) {
      p[len] = '\0';
      q = p;
      while(*q && !isspace(*q)) {
        q++;
      }
      if (isspace(*q)) {
        *q++ = '\0';
        while(isspace(*q)) {
          q++;
        }
      }
      switch((kw = btplus_search(p))) {
          case BTPLUS_ID:
              G_id = 1;
              break;
          case BTPLUS_NOID:
              G_id = 0;
              break;
          case BTPLUS_AUTOTREE:
              feedback = SHOW_TREE;
              break;
          case BTPLUS_AUTOLIST:
              feedback = SHOW_LIST;
              break;
          case BTPLUS_HUSH:
              feedback = SHOW_NOTHING;
              break;
          case BTPLUS_TRC :
              debug_on();
              break;
          case BTPLUS_NOTRC :
              debug_off();
              break;
          case BTPLUS_GET :
          case BTPLUS_GETTIME :
              begin = clock();
              rows = bpltree_get(q, fp, (kw == BTPLUS_GET));
              end = clock();
              if (rows >= 0) {
                if (rows == 0) {
                  printf("No data found - ");
                } else {
                  printf("%d line%s selected - ", rows, (rows > 1 ? "s" : ""));
                }
                printf("%lfs\n", (double)(end - begin) / CLOCKS_PER_SEC);
              }
              break;
          case BTPLUS_SCAN :
          case BTPLUS_SCANTIME :
              begin = clock();
              rows = bpltree_scan(q, fp, (kw == BTPLUS_SCAN));
              end = clock();
              if (rows >= 0) {
                if (rows == 0) {
                  printf("No data found - ");
                } else {
                  printf("%d line%s selected - ", rows, (rows > 1 ? "s" : ""));
                }
                printf("%lfs\n", (double)(end - begin) / CLOCKS_PER_SEC);
              }
              break;
          case BTPLUS_ADD :
          case BTPLUS_INS :
              if (G_echo) {
                printf("+%s\n", q);
                fflush(stdout);
              }
              // q supposed to be a comma-separated pair
              ok = 0;
              if ((q2 = strchr(q, ',')) != NULL) {
                unsigned long off;
                *q2++ = '\0';
                if (sscanf(q2, "%lu", &off) == 1) {
                  ok = 1;
                  if (bpltree_insert(q, off)) {
                    printf("%s\n", bpltree_err_msg());
                  } else {
                    if (feedback) {
                      if (feedback == SHOW_TREE) {
                         bpltree_display(bpltree_root(), 0);
                      } else {
                         list();
                      }
                      putchar('\n');
                    }
                  }
                }
              }
              if (!ok) {
                printf("Expected : %s key, <positive value>\n",
                       btplus_keyword(kw));
              }
              break;
          case BTPLUS_DEL :
          case BTPLUS_REM :
              if (G_echo) {
                printf("-%s\n", q);
                fflush(stdout);
              }
              if (bpltree_delete(q) == 0) {
                if (feedback) {
                  if (feedback == SHOW_TREE) {
                     bpltree_display(bpltree_root(), 0);
                  } else {
                     list();
                  }
                  putchar('\n');
                }
              } else {
                printf("Key not found\n");
              }
              break;
          case BTPLUS_FIND :
          case BTPLUS_SEARCH :
              bpltree_search(q);
              break;
          case BTPLUS_LIST :
              list();
              putchar('\n');
              break;
          case BTPLUS_SHOW :
          case BTPLUS_DISPLAY :
              bpltree_display(bpltree_root(), 0);
              putchar('\n');
              break;
          case BTPLUS_HELP :
              printf("Available commands:\n");
              printf(" help                       : display this\n");
              printf(" ins <key>,<val>\n");
              printf("  or add <key>,<val>        : insert a (key,val) pair\n");
              printf(" rem <key> or del <key>     : remove a key\n");
              printf(" get <key>[,<key>]          : retrieve info using the index\n");
              printf("                              ranges such as \",key\" or \"key,\" are supported");
              printf("                              composite keys are supported");
              printf(" gettime <key>[,<key>]      : retrieve info using the index but only show time taken\n");
              printf(" scan <key>[,<key>]         : retrieve info without using the index\n");
              printf("                              ranges such as \",key\" or \"key,\" are supported");
              printf("                              composite keys are supported");
              printf("                              field position (value@field#) is supported");
              printf(" scantime <key>[,<key>]     : retrieve info without using the index but only show time taken\n");
              printf(" gettime <key>              : retrieve info using the index but only show time taken\n");
              printf(" find <key> or search <key> : display search path\n");
              printf(" id                         : display id next to node (default)\n");
              printf(" noid                       : suppress id next to node\n");
              printf(" show or display            : display the tree\n");
              printf(" list                       : list ordered keys\n");
              printf(" hush                       : display nothing after change\n");
              printf(" autotree                   : show tree after change (default)\n");
              printf(" autolist                   : show ordered list after change\n");
              printf(" trc                        : display extensive trace\n");
              printf(" notrc                      : turn tracing off\n");
              printf(" bye, quit or stop          : quit the program\n");
              break;
          case BTPLUS_BYE :
          case BTPLUS_QUIT :
          case BTPLUS_STOP :
              read_cmd = 0;
              bpltree_free();
              printf("Goodbye\n");
              break;
          default:
              printf("Invalid command \"%s\" - try \"help\"\n", p);
              fflush(stdout);
              break;
      }     /* End of switch */
    }       /* End of if */
  }         /* End of while */
  if (fp) {
    fclose(fp);
  }
  return 0;
}           /* End of main() */
