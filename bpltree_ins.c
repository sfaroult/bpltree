/* ----------------------------------------------------------------- *
 *
 *                         bpltree_ins.c
 *
 *  Insertion into a b+tree
 *
 * ----------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>

#include "bpltree.h"
#include "bpltree_err.h"
#include "debug.h"


static short split_position(short target_pos, char *new_up, char leaf) {
    // Reminder: the split position is the 
    // position of the key that moves up.
    // Everything smaller remains in the left node,
    // everything bigger or equal goes to a newly
    // created right sibling node.
    // ------
    // Beware: for leaves, the array is numbered
    // from 0 to MAX - 1, but for internal nodes
    // as there is one more pointer than keys
    // it's numbered from 1 to MAX.
    short maxkeys = bpltree_maxkeys();

    assert(new_up);
    *new_up = 0;
    if (maxkeys % 2) {
      // Odd maximum number of keys.
      // Split position is in the middle
      return ((maxkeys + 1) / 2 - (short)leaf);
    } else {
      // Even maximum number of keys.
      // We aim to have the same number of keys
      // in the left node and in the right node
      // after the split - it all depends on
      // where the key to insert should go
      // Suppose we have 2p nodes. Take default
      // as p+1
      short pos = (maxkeys / 2) + 1 - (short)leaf;
      // So, left = p keys, key p+1 goes up, p - 1
      // keys to migrate to the right. But but but ...
      // This is all well if the target position is
      // greater than p+1, because it would be 
      // nicely balanced.
      // If the key to insert is smaller (or equal to)
      // the key at split_pos, then it will go to the left
      // node and we should have p-1 keys on the left
      // and p keys on the right.
      if (!leaf && (target_pos == pos)) {
        *new_up = 1;
      }
      if (target_pos <= pos) {
        pos--;
      }
      return pos;
    }
    return -1; 
}

static NODE_T  *split_node(NODE_T *n, short split_pos, short indent) {
   // Splits n at position split_pos (moves everything
   // from split_pos + 1 to the end into a new node to the
   // right) and returns a pointer to the new sibling node.
   // indent is just for indenting debugging messages.
   NODE_T *new_n = NULL;
   short   i;

   assert(n
          && (n->keycnt == bpltree_maxkeys())
          && (split_pos > 0)
          && (split_pos < n->keycnt));
   debug(indent, "splitting node %hd", n->id);
   if (n->parent == NULL) {
     debug(indent, "splitting the root");
     // We are splitting the root. We need a new root and
     // it will necessarily be an internal node.
     n->parent = new_node(NULL, 0);
     debug(indent, "new root node %hd", (n->parent)->id);
     bpltree_setroot(n->parent);
   }
   new_n = new_node(n->parent,
                    (_is_leaf(n) ? 1 : 0)); // New sibling, same parent
   assert(new_n);
   debug(indent, "created new node %hd, parent %hd",
         new_n->id, (new_n->parent)->id);
   // Copy nodes 
   debug(indent, "splitting at %hd", split_pos);
   if (debugging()) {
     debug_no_nl(indent, "before split: ");
     bpltree_show_node(n, 0);
   }
   // (dest, src, size)
   if (!_is_leaf(n)) {
     (void)memmove(&(new_n->node.internal.k[1]),
                   &(n->node.internal.k[split_pos+1]),
                   sizeof(REDIRECT_T) * (n->keycnt - split_pos));
     // Blank out what was moved for safety
     (void)memset(&(n->node.internal.k[split_pos+1]), 0,
                  sizeof(REDIRECT_T) * (n->keycnt - split_pos));
   } else {
     // For leaves the value at split_pos remains
     // in the first node, and numbering is from zero.
     (void)memmove(&(new_n->node.leaf.k[0]),
                   &(n->node.leaf.k[split_pos+1]),
                   sizeof(KEY_POS_T) * (n->keycnt - split_pos - 1));
     (void)memset(&(n->node.leaf.k[split_pos+1]), 0,
                  sizeof(KEY_POS_T) * (n->keycnt - split_pos - 1));
     // Set the "next" pointer
     new_n->node.leaf.next = n->node.leaf.next;
     n->node.leaf.next = new_n;
   }
   // Adjust key counts
   new_n->keycnt = n->keycnt;
   n->keycnt = split_pos + (_is_leaf(n) ? 1 : 0);
   if (debugging()) {
     debug_no_nl(indent, "-> %hd keys in node %d ", n->keycnt, n->id);
     bpltree_show_node(n, 0);
   }
   new_n->keycnt -= (split_pos + (_is_leaf(n) ? 1 : 0));
   if (debugging()) {
     debug_no_nl(indent, "-> %hd keys in node %d ", new_n->keycnt, new_n->id);
     bpltree_show_node(new_n, 0);
   }
   if (!_is_leaf(n)) {
     // Change parent pointer in the new node
     debug(indent, "updating parent pointer in children of new node");
     // Note that at this point the 'smaller' node isn't known
     for (i = 1; i <= new_n->keycnt; i++) {
       (new_n->node.internal.k[i].bigger)->parent = new_n;
     }
   }
   return new_n;
}

static short insert_in_node(NODE_T *n,
                            char   *key,
                            off_t   val,
                            NODE_T *smaller,
                            NODE_T *bigger,
                            short   indent) {
  short ret;

  // Physically insert into a node
  debug(indent, ">> insert_in_node");
  assert(key);
  if (n == NULL) {
    debug(indent, "need to create a new root (internal node)");
    NODE_T *root = new_node(NULL, 0);  // Can no longer be a leaf
    root->keycnt = 1;
    root->node.internal.k[0].bigger = smaller; 
    root->node.internal.k[1].key = key;
    root->node.internal.k[1].bigger = bigger; 
    bpltree_setroot(root);
    if (debugging()) {
      debug_no_nl(indent, "new root is node %hd ", root->id);
      bpltree_show_node(root, 0);
    }
  } else {
    short pos = find_pos(n, key, 0, indent);
    if (debugging()) {
      if (bpltree_numeric()) {
        debug_no_nl(indent, "inserting key %d at pos %hd in node %hd ",
                    *((int*)key), pos, n->id);
      } else {
        debug_no_nl(indent, "inserting key %s at pos %hd in node %hd ",
                    key, pos, n->id);
      }
      bpltree_show_node(n, 0);
    }
    if (pos >= 0) {
      if (n->keycnt == bpltree_maxkeys()) {
        // Must split
        // Contrary to what happens in internal nodes,
        // a key (and its associated position) are ALWAYS
        // inserted inside a leaf node.
        char   *key_up;
        char    new_up = 0; // Flag
        short   split_pos = split_position(pos, &new_up, n->is_leaf);
        if (new_up) {
          // The key that will go up is the one being inserted
          key_up = key;
        } else {
          if (_is_leaf(n)) {
            key_up = key_duplicate(n->node.leaf.k[split_pos].key);
          } else {
            key_up = n->node.internal.k[split_pos].key;
          }
        }
        NODE_T *new_n = split_node(n, split_pos, indent);
        if (!new_up) {
          // A key that already was in the node (at split_pos)
          // moves up.
          // The left-most smaller pointer of the new node
          // is what was bigger than the key that moves
          // and smaller than the key that followed, now the
          // first one in the new node
          debug(indent, "moving up the key already at %hd",
                        split_pos);
          if (!_is_leaf(n)) {
            new_n->node.internal.k[0].bigger
                    = n->node.internal.k[split_pos].bigger;
            (new_n->node.internal.k[0].bigger)->parent = new_n;
          }
          // Clear what refers to the promoted value (key already saved)
          if (!_is_leaf(n)) {
            n->node.internal.k[split_pos].key = NULL;
            n->node.internal.k[split_pos].bigger = NULL;
            (n->keycnt)--;
          } // else keep the record in the leaf
          // Move up the key at split_pos in the left sibling (n)
          if (insert_in_node(n->parent, key_up, 0,
                             n, new_n, indent+2) >= 0) {
            if (pos <= split_pos) {
              // Insert into n
              ret = insert_in_node(n, key, val, smaller, bigger, indent+2);
              debug(indent, "<< insert_in_node (%hd)", ret);
              return ret;
            } else {
              // Insert into new_sibling
              ret = insert_in_node(new_n, key, val,
                                   smaller, bigger, indent+2);
              debug(indent, "<< insert_in_node (%hd)", ret);
              return ret;
            } 
          } 
        } else {
          // The new key is the one that goes up.
          // Necessarily an internal node.
          // The left-most smaller pointer of the new node
          // is what was bigger than the key that is inserted
          debug(indent, "moving up the new key");
          if (bigger) {
            new_n->node.internal.k[0].bigger = bigger;
            if (new_n->node.internal.k[0].bigger) {
              (new_n->node.internal.k[0].bigger)->parent = new_n;
            }
          }
          ret= insert_in_node(n->parent, key_up, val,
                              n, new_n, indent+2);
          debug(indent, "<< insert_in_node (%hd)", ret);
          return ret;
        }
      } else {
        // There is still room in the node
        if (pos <= n->keycnt) {
          debug(indent, "shifting %d elements from %hd to %hd in node %hd",
                (1 + n->keycnt - pos),
                pos, pos + 1, n->id);
          // (dest, src, size)
          if (!_is_leaf(n)) {
            (void)memmove(&(n->node.internal.k[pos+1]),
                          &(n->node.internal.k[pos]),
                          sizeof(REDIRECT_T) * (1 + n->keycnt - pos));
          } else {
            (void)memmove(&(n->node.leaf.k[pos+1]),
                          &(n->node.leaf.k[pos]),
                          sizeof(KEY_POS_T) * (1 + n->keycnt - pos));
          }
        }
        if (!_is_leaf(n)) {
          n->node.internal.k[pos].key = key;
          if (!n->node.internal.k[pos-1].bigger) {
            n->node.internal.k[pos-1].bigger = smaller;
            if (smaller) {
              smaller->parent = n;
            }
          }
          n->node.internal.k[pos].bigger = bigger;
        } else {
          n->node.leaf.k[pos].key = key;
          n->node.leaf.k[pos].pos = val;
        }
        if (bigger) {
          // Adjust
          bigger->parent = n;
        }
        (n->keycnt)++;
        if (debugging()) {
          debug_no_nl(indent, "updated node %d ", n->id);
          bpltree_show_node(n, 0);
        }
      }
    } else {
      debug(indent, "<< insert_in_node (-1)");
      return -1;
    }
  } 
  debug(indent, "<< insert_in_node (0)");
  return 0;
}

static short insert_key(NODE_T       *n,
                        char         *key,
                        unsigned long val,
                        short         indent) {
    // -1 if there is something wrong, 0 if OK
    short pos = 1;
    int   cmp = -1;
    short ret;

    debug(indent, ">> insert_key");
    assert(key && n);
    // Find the leaf node where the key should be stored
    if (debugging()) {
      debug_no_nl(indent, "searching node %hd: ", n->id);
      bpltree_show_node(n, 0);
    }
    if (!_is_leaf(n)) {
      while ((pos <= n->keycnt)
             && ((cmp = bpltree_keycmp(key,
                                     n->node.internal.k[pos].key,
                                     KEYSEP)) > 0)) {
        pos++;
      }
    } else {
      pos = 0;
      while ((pos < n->keycnt)
             && ((cmp = bpltree_keycmp(key,
                                     n->node.leaf.k[pos].key,
                                     KEYSEP)) > 0)) {
        pos++;
      }
    }
    if (cmp == 0) {
      // We've found it in the tree
      debug(indent, "** found at position %hd", pos);
      debug(indent, "duplicates not allowed");
      if (bpltree_numeric()) {
         char buff[20];
         snprintf(buff, 20, "%d", *((int *)key));
         bpltree_err_seterr(BPLT_ERR_DUPL, buff);
      } else {
         bpltree_err_seterr(BPLT_ERR_DUPL, key);
      }
      debug(indent, "<< insert_key (-1)");
      return -1;
    }
    // We have found a key that is greater (or equal) or we have reached
    // the end of the node
    if (!_is_leaf(n)) {
      ret = insert_key(n->node.internal.k[pos-1].bigger,
                       key, val, indent+2);
      debug(indent, "<< insert_key (%hd)", ret);
      return ret;
    } else {
      char *k;
      debug(indent, "should go in this leaf node");
      k = key_duplicate(key);
      ret = insert_in_node(n, k, val, NULL, NULL, indent);
      debug(indent, "<< insert_key (%hd)", ret);
      return ret;
    }
    debug(indent, "<< insert_key (-1)");
    return -1;
}

static int insert_from_root(char *key, unsigned long val, int indent) {
   NODE_T *n;
   int     ret;

   debug(indent, ">> insert_from_root");
   if (!bpltree_root()) {
     debug(indent, "insert_from_root() - creating root");
     n = new_node(NULL, 1);  // The first root ever created is a leaf
     bpltree_setroot(n);
   }
   ret = insert_key(bpltree_root(), key, val, indent);
   /*
   if (debugging()) {
     if (bpltree_check(bpltree_root(), (char *)NULL)) {
       bpltree_display(bpltree_root(), 0);
       assert(0);  // Force exit
     }
   }
   */
   debug(indent, "<< insert_from_root (%hd)", ret);
   return ret;
}

extern int bpltree_insert(char *key, unsigned long keyval) {
    int     val;
    short   ret = -1;

    debug(0, ">> bpltree_insert");
    if (bpltree_numeric()) {
      if (sscanf(key, "%d", &val) == 0) {
        bpltree_err_seterr(BPLT_ERR_INVNUM, key);
        debug(0, "<< bpltree_insert (%hd)", ret);
        return ret;
      }
    } 
    if (bpltree_numeric()) {
      ret = insert_from_root((char *)&val, keyval, 0);
    } else {
      ret = insert_from_root(key, keyval, 0);
    }
    debug(0, "<< bpltree_insert (%hd)", ret);
    return ret;
}
