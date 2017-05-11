/* ----------------------------------------------------------------- *
 *
 *                         bpltree_op.c
 *
 *  Everything but insert and delete.
 *
 * ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>

#include "bpltree.h"
#include "debug.h"

#define DEFAULT_SEP  '\t'

static short   G_maxkeys = DEF_MAX_KEYS;
static float   G_fillrate = DEF_FILL_RATE;
static NODE_T *G_root = NULL;
static char    G_numeric = 0;
static char    G_sep = DEFAULT_SEP;

extern void bpltree_setfilesep(char sep) {
  G_sep = sep;
}

extern char bpltree_filesep(void) {
  return G_sep;
}

extern void bpltree_setmaxkeys(short n) {
  G_maxkeys = n;
}

extern short bpltree_maxkeys(void) {
  return G_maxkeys;
}

extern float bpltree_fillrate(void) {
  return G_fillrate;
}

extern void bpltree_setroot(NODE_T *n) {
  G_root = n;
  if (n) {
    n->parent = NULL;
  }
}

extern NODE_T *bpltree_root(void) {
  return G_root;
}

extern void bpltree_setnumeric(void) {
  G_numeric = 1;
}

extern char bpltree_numeric(void) {
  return G_numeric;
}

extern int bpltree_keycmp(char *k1, char *k2, char sep) {
  // Returns 0 if k1 == k2,
  //        a value > 0 if k1 > k2
  //        a value < 0 if k1 < k2
  int cmp;

  if (G_numeric) {
    if (*((int *)k1) == *((int *)k2)) {
      cmp = 0;
    } else if (*((int *)k1) > *((int *)k2)) {
      cmp = 1;
    } else {
      cmp = -1;
    }
  } else {
    int   len;
    int   sep1_cnt = 0;
    int   sep2_cnt = 0;
    char *sep1 = k1;
    char *sep2 = k2;
    while ((sep1 = strchr(sep1, sep)) != NULL) {
      sep1_cnt++;
      sep1++;
    }
    while ((sep2 = strchr(sep2, sep)) != NULL) {
      sep2_cnt++;
      sep2++;
    }
    if (sep1_cnt == sep2_cnt) {
      len = (strlen(k1) > strlen(k2) ? strlen(k1) : strlen(k2));
    } else {
      if (sep1_cnt < sep2_cnt) {
        len = strlen(k1);
      } else {
        len = strlen(k2);
      }
    }
    cmp = strncmp(k1, k2, len);
  }
  return cmp;
}

extern char bpltree_check(NODE_T *n, char *prev_key) {
  // Debugging - check that everything is OK in the tree
  static char *last_key;

  if (n) {
    int  i;

    assert((n->parent == NULL)
           && (n->keycnt <= bpltree_maxkeys())
           && (n->keycnt >= MIN_KEYS));
    if (prev_key == NULL) {
      last_key = prev_key;
    }   
    for (i = 0; i <= bpltree_maxkeys(); i++) {
      if (!n->is_leaf && n->node.internal.k[i].key) {
        if (i == 0) {
          // Should be null
          debug(0, "Slot %d in node %hd: key should be null", i, n->id);
          return 1;
        }
        if (last_key) {
          if (bpltree_keycmp(n->node.internal.k[i].key,
                             last_key, KEYSEP) < 0) {
            debug(0, "Slot %d in node %hd: misplaced key", i, n->id);
            return 1;  // Inconsistent, current key
                       // should be greater than the previous one
          }
        }
        if (i > n->keycnt) {
          // Should be null
          debug(0, "Slot %d in node %hd: key should be null", i, n->id);
          return 1;
        }
        last_key = n->node.internal.k[i].key;
      } else {
        if (!n->is_leaf && i && (i <= n->keycnt)) {
          debug(0, "Slot %d in node %hd: no value, keycnt is %hd",
                i, n->id, n->keycnt);
          return 1;
        }
      }
      if (!n->is_leaf && n->node.internal.k[i].bigger) {
        if (i <= n->keycnt) {  
          if ((n->node.internal.k[i].bigger)->parent != n) {
            debug(0, "Parent pointer of node %hd inconsistent",
                     (n->node.internal.k[i].bigger)->id);
            return 1;
          }
          if (bpltree_check(n->node.internal.k[i].bigger,
                          n->node.internal.k[i].key)) {
            return 1;
          }
        } else {
          debug(0, "Slot %d in node %hd: bigger pointer should be null",
                   i, n->id);
          return 1;
        }
      }
    }
  }                             /* End of if */
  return 0;
}                               /* End of bpltree_check() */

extern char *key_duplicate(char *key) {
    char *dupl = NULL;
    if (key) {
      if (G_numeric) {
        // A union would be more space efficient, but
        // would complicate the code unnecessarily for
        // what shouldn't be the most common case.
        int val = *((int *)key);

        if ((dupl = (char *)malloc(sizeof(int))) != NULL) {
          memcpy(dupl, &val, sizeof(int));
        }
      } else { 
        dupl = strdup(key);
      }
    }
    return dupl;
}

extern NODE_T *new_node(NODE_T *parent, char leaf) {
    static short last_id = 0;

    NODE_T *n =(NODE_T *)malloc(sizeof(NODE_T));
    assert(n);
    last_id++;
    n->parent = parent;
    n->id = last_id;
    n->keycnt = 0;
    n->is_leaf = leaf;
    if (leaf) {
      n->node.leaf.k = (KEY_POS_T *)calloc((1 + G_maxkeys),
                                            sizeof(KEY_POS_T));
      assert(n->node.leaf.k);
    } else {
      n->node.internal.k = (REDIRECT_T *)calloc((1 + G_maxkeys),
                                                 sizeof(REDIRECT_T));
      assert(n->node.internal.k);
    }
    return n;
}

static void free_tree(NODE_T **root_ptr) {
    if (root_ptr && *root_ptr) {
      int i;

      if (!(*root_ptr)->is_leaf) {
        for (i = 0; i < (*root_ptr)->keycnt; i++) {
          free_tree(&((*root_ptr)->node.internal.k[i].bigger));
          if ((*root_ptr)->node.internal.k[i].key) {
            free((*root_ptr)->node.internal.k[i].key);
          }
        }
        free((*root_ptr)->node.internal.k);
      } else {
        for (i = 0; i < (*root_ptr)->keycnt; i++) {
          if ((*root_ptr)->node.leaf.k[i].key) {
            free((*root_ptr)->node.leaf.k[i].key);
          }
        }
        free((*root_ptr)->node.leaf.k);
      }
      free(*root_ptr);
      *root_ptr = NULL;
    }
}

extern void bpltree_free(void) {
    free_tree(&G_root);
}

extern NODE_T *left_sibling(NODE_T *n, short *sep_pos) {
   // Find the node on the left
   // sep_pos is the index of the key in the parent
   // that is greater than all keys in the left sibling
   // and smaller than all keys in the current node.
   NODE_T *l = NULL;
   NODE_T *p;

   if (n && sep_pos && ((p = n->parent) != NULL)) {
     if (n == p->node.internal.k[0].bigger) {
       *sep_pos = -1;
       return NULL;
     }
     assert(p->node.internal.k);
     // Where are we?
     *sep_pos = 1;
     while ((*sep_pos <= p->keycnt)
            && (p->node.internal.k[*sep_pos].bigger != n)) {
       (*sep_pos)++;
     }
     assert(*sep_pos <= p->keycnt);
     l = p->node.internal.k[(*sep_pos)-1].bigger;
   } 
   return l;
}

extern NODE_T *right_sibling(NODE_T *n, short *sep_pos) {
   // Find the node on the right
   // See comments in left_sibling
   NODE_T *r = NULL;
   NODE_T *p;

   if (n && sep_pos && ((p = n->parent) != NULL)) {
     assert(p->node.internal.k);
     *sep_pos = 0; 
     // Where are we?
     while ((*sep_pos <= p->keycnt)
            && (p->node.internal.k[*sep_pos].bigger != n)) {
       (*sep_pos)++;
     }
     assert(p->node.internal.k[*sep_pos].bigger == n);
     if (*sep_pos == p->keycnt) {
       *sep_pos = -1;
       r = NULL;
     } else {
       (*sep_pos)++;
       r = p->node.internal.k[*sep_pos].bigger;
     } 
  }
  return r;
}

extern NODE_T *find_node(NODE_T *tree, char *key) {
    // Find the leaf node where the key should be stored
    if (tree && key) {
       if (!_is_leaf(tree)) {
         short i = 1;
         int   cmp = -1;

         while ((i <= tree->keycnt)
                && ((cmp = bpltree_keycmp(key,
                                        tree->node.internal.k[i].key,
                                        KEYSEP)) > 0)) {
           i++;
         }
         if (cmp == 0) {
           // We've found it in the tree
           return NULL;
         }
       } else {
         // Leaf - can only insert there
         return tree;
       }
    }
    return NULL;
}

extern short find_pos(NODE_T *n, char *key, char present, short lvl) {
    // 'present' says whether the key is expected to be
    // present or absent
    short pos = -1;
    int   cmp = 1;

    if (n && key) {
      // Find where the key should go.
      // Note that we don't worry whether the node
      // is full or not.
      if (!_is_leaf(n)) {
        pos = 1;
        while ((pos <= n->keycnt)
               && ((cmp = bpltree_keycmp(key,
                                       n->node.internal.k[pos].key,
                                       KEYSEP)) > 0)) {
          pos++;
        }
        if (pos > 1) {
          if (G_numeric) {
            debug(lvl, "%d at pos %hd of internal node %hd (after %d)",
                  *((int*)key), pos, n->id,
                  *((int *)(n->node.internal.k[pos-1].key)));
          } else {
            debug(lvl, "%s at pos %hd of internal node %hd (after %s)",
                    key, pos, n->id, n->node.internal.k[pos-1].key);
          }
        } else {
          debug(lvl, "goes at pos 1 in internal node %hd", n->id);
        }
      } else {
        // Leaf
        pos = 0;
        while ((pos < n->keycnt)
               && ((cmp = bpltree_keycmp(key,
                                       n->node.leaf.k[pos].key,
                                       KEYSEP)) > 0)) {
          pos++;
        }
        if (pos > 0) {
          if (G_numeric) {
            debug(lvl, "%d at pos %hd of leaf node %hd (after %d)",
                  *((int*)key), pos, n->id,
                  *((int *)(n->node.leaf.k[pos-1].key)));
          } else {
            debug(lvl, "%s at pos %hd of leaf node %hd (after %s)",
                    key, pos, n->id, n->node.leaf.k[pos-1].key);
          }
        } else {
          debug(lvl, "goes at pos 0 in leaf node %hd", n->id);
        }
      }
      if (present) {
        // Expected to be found
        if (cmp) {   // Not here
          debug(lvl, "key was expected to be found and not here");
          return -1;
        }
      } else {
        // Not expected to be found
        // Not a problem if not unique, problem if unique
        if (cmp == 0) {
          debug(lvl, "key found and was NOT expected");
          return -1;
        }
      }
    }
    return pos;
}
