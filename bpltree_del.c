/* ----------------------------------------------------------------- *
 *
 *                         bpltree_del.c
 *
 *  Deletion from a B+tree
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

// Forward declaration
static short delete_key(NODE_T *n,
                        char   *key,
                        short   indent);

static NODE_T *leaf_with_greatest_key(NODE_T *subtree) {
    if (subtree) {
      if (!_is_leaf(subtree)) {
        return leaf_with_greatest_key(
                   subtree->node.internal.k[subtree->keycnt].bigger);
      } else {
        return subtree;
      }
    }
    return NULL;
}

static int borrow_from_left_internal(NODE_T *n, short indent) {
   if (n && !_is_leaf(n) && n->parent) {
      short   parent_pos;
      NODE_T *par = n->parent;
      NODE_T *l = left_sibling(n, &parent_pos);
      char   *k;

      // Check whether we can borrow a key from the left sibling
      if (l && (l->keycnt > MIN_KEYS)) {
        // Let's call K the key in the parent that
        // is greater than all keys in the left sibling
        // and smaller than all keys in the node that
        // underflows; its index in the parent will
        // be parent_pos (set when searching for the sibling 
        // nodes).
        // Move the greatest key of the left sibling
        // to the place of K in the parent, and
        // bring K as the first key in the current node.
        // --------------
        debug(indent, "borrowing from left node %hd", l->id);
        if (debugging()) {
          debug_no_nl(indent, "left node before borrowing: ");
          bpltree_show_node(l, 0);
          debug_no_nl(indent, "current node %hd before borrowing: ", n->id);
          bpltree_show_node(n, 0);
        }
        // Make room for K (dest, src, size)
        (void)memmove(&(n->node.internal.k[1]), &(n->node.internal.k[0]),
                      sizeof(REDIRECT_T) * (n->keycnt+1));
        // Add K
        n->node.internal.k[1].key = par->node.internal.k[parent_pos].key;
        // Values that precede K are the ones bigger
        // than the biggest key in the left node
        n->node.internal.k[0].key = NULL;
        n->node.internal.k[0].bigger = l->node.internal.k[l->keycnt].bigger;
        (n->node.internal.k[0].bigger)->parent = n;
        (n->keycnt)++;
        // Find the greatest key in the left sibling
        k = l->node.internal.k[l->keycnt].key;
        // Cleanup
        l->node.internal.k[l->keycnt].key = NULL;
        l->node.internal.k[l->keycnt].bigger = NULL;
        (l->keycnt)--;
        // Store k in the parent
        par->node.internal.k[parent_pos].key = k;
        if (debugging()) {
          debug_no_nl(indent, "left node after borrowing: ");
          bpltree_show_node(l, 0);
          debug_no_nl(indent, "current node %hd after borrowing: ", n->id);
          bpltree_show_node(n, 0);
        }
        return 0;
      } 
    }
    return -1;
}

static int borrow_from_left_leaf(NODE_T *n, short indent) {
   if (n && _is_leaf(n) && n->parent) {
      short   parent_pos;
      NODE_T *par = n->parent;
      NODE_T *l = left_sibling(n, &parent_pos);

      // Check whether we can borrow a key from the left sibling
      if (l && (l->keycnt > MIN_KEYS)) {
        // For leaf nodes, the parent key is also there.
        // If we borrow a leaf from left, the parent key
        // must be replaced with the last remaining value in 
        // the left node.
        debug(indent, "borrowing from left leaf node %hd", l->id);
        if (debugging()) {
          debug_no_nl(indent, "left node before borrowing: ");
          bpltree_show_node(l, 0);
          debug_no_nl(indent, "current node %hd before borrowing: ", n->id);
          bpltree_show_node(n, 0);
        }
        // Make room for the entry that comes from the left
        // (dest, src, size)
        (void)memmove(&(n->node.leaf.k[1]), &(n->node.leaf.k[0]),
                     sizeof(KEY_POS_T) * n->keycnt);
        // Add the entry (dest, src, size)
        (void)memcpy(&(n->node.leaf.k[0]),
                     &(l->node.leaf.k[l->keycnt-1]),
                     sizeof(KEY_POS_T));
        n->keycnt++;
        // Cleanup the left node
        if (l->node.leaf.k[l->keycnt-1].key) {
          // DON'T FREE THE KEY - the pointer was just moved 
          l->node.leaf.k[l->keycnt-1].key = NULL;
          l->node.leaf.k[l->keycnt-1].pos = 0;
        }
        l->keycnt--;
        // The parent now - move there what is now the last
        // entry in the left node
        if (par->node.internal.k[parent_pos].key) {
          free(par->node.internal.k[parent_pos].key);
        }
        par->node.internal.k[parent_pos].key
               = key_duplicate(l->node.leaf.k[l->keycnt-1].key);
        if (debugging()) {
          debug_no_nl(indent, "left node after borrowing: ");
          bpltree_show_node(l, 0);
          debug_no_nl(indent, "current node %hd after borrowing: ", n->id);
          bpltree_show_node(n, 0);
        }
        return 0;
      } 
    }
    return -1;
}

static int borrow_from_left(NODE_T *n, short indent) {
   if (n) {
     if (_is_leaf(n)) {
       return borrow_from_left_leaf(n, indent);
     } else {
       return borrow_from_left_internal(n, indent);
     }
   }
   return -1;
}

static int borrow_from_right_leaf(NODE_T *n, short indent) {
   if (n && n->parent) {
      short   parent_pos;
      NODE_T *par = n->parent;
      NODE_T *r = right_sibling(n, &parent_pos);
      if (r && (r->keycnt > MIN_KEYS)) {
        // Basically the same operation as with borrowing from
        // the left, except that the moved key is the one that
        // is copied to the parent
        // --------------
        debug(indent, "borrowing from right leaf node %hd", r->id);
        if (debugging()) {
          debug_no_nl(indent, "current node %hd before borrowing: ", n->id);
          bpltree_show_node(n, 0);
          debug_no_nl(indent, "right node before borrowing: ");
          bpltree_show_node(r, 0);
        }
        // Add the entry (dest, src, size)
        (void)memcpy(&(n->node.leaf.k[n->keycnt]),
                     &(r->node.leaf.k[0]),
                     sizeof(KEY_POS_T));
        (n->keycnt)++;
        // Remove from the right node
        (r->keycnt)--;
        // (dest, src, size)
        (void)memmove(&(r->node.leaf.k[0]), &(r->node.leaf.k[1]),
                     sizeof(KEY_POS_T) * r->keycnt);
        r->node.leaf.k[r->keycnt].key = NULL;
        r->node.leaf.k[r->keycnt].pos = 0;
        // Deal with the parent
        if (par->node.internal.k[parent_pos].key) {
          free(par->node.internal.k[parent_pos].key);
        }
        par->node.internal.k[parent_pos].key
               = key_duplicate(n->node.leaf.k[n->keycnt-1].key);
        if (debugging()) {
          debug_no_nl(indent, "current node %hd after borrowing: ", n->id);
          bpltree_show_node(n, 0);
          debug_no_nl(indent, "right node after borrowing: ");
          bpltree_show_node(r, 0);
        }
        return 0;
      } 
    }
    return -1;
}

static int borrow_from_right_internal(NODE_T *n, short indent) {
   if (n && n->parent) {
      short   parent_pos;
      NODE_T *par = n->parent;
      NODE_T *r = right_sibling(n, &parent_pos);
      char   *k;

      if (r && (r->keycnt > MIN_KEYS)) {
        // Let's call K the key in the parent that
        // is smaller than all keys in the right sibling
        // and greater than all keys in the node that
        // underflows; its index in the parent will
        // be parent_pos (set when searching for the sibling 
        // nodes).
        // Move the smallest key of the right sibling
        // to the place of K in the parent, and
        // add K as the last key in the current node.
        // --------------
        debug(indent, "borrowing from right node %hd", r->id);
        if (debugging()) {
          debug_no_nl(indent, "current node %hd before borrowing: ", n->id);
          bpltree_show_node(n, 0);
          debug_no_nl(indent, "right node before borrowing: ");
          bpltree_show_node(r, 0);
        }
        // Add K
        (n->keycnt)++;
        n->node.internal.k[n->keycnt].key
                 = par->node.internal.k[parent_pos].key;
        n->node.internal.k[n->keycnt].bigger = r->node.internal.k[0].bigger;
        if (n->node.internal.k[n->keycnt].bigger) {
          (n->node.internal.k[n->keycnt].bigger)->parent = n;
        }
        // Find the smallest key in the right sibling
        k = r->node.internal.k[1].key;
        // Cleanup the right sibling - dest, src, size
        (void)memmove(&(r->node.internal.k[0]), &(r->node.internal.k[1]),
                      sizeof(REDIRECT_T) * r->keycnt);
        r->node.internal.k[0].key = NULL;
        r->node.internal.k[r->keycnt].key = NULL;
        r->node.internal.k[r->keycnt].bigger = NULL;
        (r->keycnt)--;
        // Store k in the parent
        par->node.internal.k[parent_pos].key = k;
        if (debugging()) {
          debug_no_nl(indent, "current node %hd after borrowing: ", n->id);
          bpltree_show_node(n, 0);
          debug_no_nl(indent, "right node after borrowing: ");
          bpltree_show_node(r, 0);
        }
        return 0;
      } 
    }
    return -1;
}

static int borrow_from_right(NODE_T *n, short indent) {
   if (n) {
     if (_is_leaf(n)) {
       return borrow_from_right_leaf(n, indent);
     } else {
       return borrow_from_right_internal(n, indent);
     }
   }
   return -1;
}

static short merge_internal_nodes(NODE_T *left,
                                  NODE_T *right,
                                  NODE_T *par,  // parent of left and right
                                  short   lvl) {
   // Note that this function doesn't remove the parent key
   // but it returns its position
   short i = 0;

   assert(left && right && par && !_is_leaf(left));
   debug(lvl, "merging internal left node %hd with right node %hd",
              left->id, right->id);
   if (debugging()) {
      debug_no_nl(lvl, "left before merge: ");
      bpltree_show_node(left, 0);
      debug_no_nl(lvl, "right before merge: ");
      bpltree_show_node(right, 0);
   }
   assert((left->keycnt + 1 + right->keycnt) <= bpltree_maxkeys());
   while ((i < par->keycnt)
          && (par->node.internal.k[i].bigger != left) > 0) {
     i++;
   }
   assert((i < par->keycnt) && (par->node.internal.k[i+1].bigger == right));
   i++; // We have stopped just before the key between left and right
   left->node.internal.k[left->keycnt+1].key = par->node.internal.k[i].key;
   left->node.internal.k[left->keycnt+1].bigger
                 = right->node.internal.k[0].bigger;
   par->node.internal.k[i].key = NULL;
   par->node.internal.k[i].bigger = NULL;
   (left->keycnt)++;
   // (dest, src, size) 
   (void)memmove(&(left->node.internal.k[left->keycnt+1]),
                 &(right->node.internal.k[1]),
                 sizeof(REDIRECT_T) * right->keycnt);
   // Adjust parent pointer
   if (!_is_leaf(left)) {
     short k;
     for (k = left->keycnt - 1;
          k <= left->keycnt + right->keycnt;
          k++) {
       if (left->node.internal.k[k].bigger) {
         (left->node.internal.k[k].bigger)->parent = left;
       }
     }
   }
   left->keycnt += right->keycnt; 
   if (debugging()) {
      debug_no_nl(lvl, "left after merge: ");
      bpltree_show_node(left, 0);
   }
   // Free the right node (except keys, moved)
   debug(lvl, "removing internal right node %hd after merge", right->id);
   free(right->node.internal.k);
   free(right);
   return i;
}

static short merge_leaf_nodes(NODE_T *left,
                              NODE_T *right,
                              NODE_T *par,  // parent of left and right
                              short   lvl) {
   // Note that this function doesn't remove the parent key
   // but it returns its position
   // Simpler than with internal nodes, as the parent doesn't need
   // to be brought down.
   short i = 0;

   assert(left && right && par && _is_leaf(left));
   debug(lvl, "merging leaf left node %hd with right node %hd",
              left->id, right->id);
   if (debugging()) {
      debug_no_nl(lvl, "left before merge: ");
      bpltree_show_node(left, 0);
      debug_no_nl(lvl, "right before merge: ");
      bpltree_show_node(right, 0);
   }
   assert((left->keycnt + right->keycnt) <= bpltree_maxkeys());
   while ((i < par->keycnt)
          && (par->node.internal.k[i].bigger != left) > 0) {
     i++;
   }
   assert((i < par->keycnt) && (par->node.internal.k[i+1].bigger == right));
   i++; // We have stopped just before the key between left and right
   // (dest, src, size) 
   (void)memmove(&(left->node.leaf.k[left->keycnt]),
                 &(right->node.leaf.k[0]),
                 sizeof(REDIRECT_T) * right->keycnt);
   left->node.leaf.next = right->node.leaf.next;
   left->keycnt += right->keycnt; 
   if (debugging()) {
      debug_no_nl(lvl, "left after merge: ");
      bpltree_show_node(left, 0);
   }
   // Free the right node (except keys, moved)
   debug(lvl, "removing leaf right node %hd after merge", right->id);
   free(right->node.leaf.k);
   free(right);
   return i;
}

static int delete_internal_node(NODE_T *n,
                                short   pos,
                                short   indent) {
  assert(n && !_is_leaf(n) && (pos > 0) && (pos <= n->keycnt));
  debug(indent, "removing key at position %hd from internal node %hd",
        pos, n->id);
  if (n->node.internal.k[pos].key) {
    free(n->node.internal.k[pos].key);
  }
  if (pos < n->keycnt) {
    // (dest, src, size)
    (void)memmove(&(n->node.internal.k[pos]), &(n->node.internal.k[pos+1]),
                  sizeof(REDIRECT_T) * (n->keycnt - pos));
  }
  n->node.internal.k[n->keycnt].key = NULL;
  n->node.internal.k[n->keycnt].bigger = NULL;
  (n->keycnt)--;
  if (debugging()) {
    debug_no_nl(indent, "node now contains: ");
    bpltree_show_node(n, 0);
  }
  if (n->keycnt >= MIN_KEYS) {
    debug(indent,
          "still %hd key%s in it - success",
          n->keycnt, (n->keycnt > 1? "s" : ""));
    return 0; // Fine
  }
  if (n->parent == NULL) { // Root
    if (n->keycnt >= 1) {
      debug(indent,
            "removed one key from root (node %hd) - still %hd key%s in it",
            n->id,
            n->keycnt, (n->keycnt > 1? "s" : ""));
    } else {
      debug(indent,
            "node %hd deleted - new root node %hd",
            n->id, (n->node.internal.k[0].bigger)->id);
      bpltree_setroot(n->node.internal.k[0].bigger);
      free(n->node.internal.k);
      free(n);
      debug(indent, "former root freed");
    }
    return 0;  // Fine
  }
  // Underflow
  debug(indent, "underflow detected");
  // Try to borrow a key from the left
  if (borrow_from_left(n, indent)) {
    // If it fails borrow from right
    if (borrow_from_right(n, indent)) {
      // If it fails merge with left node, but
      // then we must recurse
      short   parent_pos = 0;
      NODE_T *par = n->parent;
      NODE_T *l = left_sibling(n, &parent_pos);

      if (l) {
         debug(indent, "merging with left node");
         parent_pos = merge_internal_nodes(l, n, par, indent);
      } else {
         NODE_T *r = right_sibling(n, &parent_pos);
         debug(indent, "merging with right node");
         parent_pos = merge_internal_nodes(n, r, par, indent);
      }
      debug(indent, "remove key at position %hd from parent %hd",
                    parent_pos, par->id);
      // After merging one node must be removed from the 
      // parent - recurse
      return delete_internal_node(par, parent_pos, indent+2);
    } else {
      return 0;
    }
  } else {
    return 0;
  }
  return -1;
}

static int delete_leaf_node(NODE_T *n,
                            short   pos,
                            short   indent) {
  assert(n && _is_leaf(n) && (pos >= 0) && (pos < n->keycnt));
  debug(indent, "removing key at position %hd from leaf node %hd",
        pos, n->id);
  if (n->node.leaf.k[pos].key) {
    free(n->node.leaf.k[pos].key);
  }
  if (pos < n->keycnt - 1) {
    // (dest, src, size)
    (void)memmove(&(n->node.leaf.k[pos]), &(n->node.leaf.k[pos+1]),
                  sizeof(REDIRECT_T) * (n->keycnt - pos));
  }
  n->node.leaf.k[n->keycnt-1].key = NULL;
  n->node.leaf.k[n->keycnt-1].pos = 0;
  (n->keycnt)--;
  if (debugging()) {
    debug_no_nl(indent, "node now contains: ");
    bpltree_show_node(n, 0);
  }
  if (n->keycnt >= MIN_KEYS) {
    debug(indent,
          "still %hd key%s in it - success",
          n->keycnt, (n->keycnt > 1? "s" : ""));
    return 0; // Fine
  }
  if (n->parent == NULL) { // Root
    if (n->keycnt >= 1) {
      debug(indent,
            "removed one key from root (node %hd) - still %hd key%s in it",
            n->id,
            n->keycnt, (n->keycnt > 1? "s" : ""));
    } else {
      debug(indent, "*** Tree emptied ***");
      bpltree_setroot(NULL);
      free(n->node.leaf.k);
      free(n);
    }
    return 0;  // Fine
  }
  // Underflow
  debug(indent, "underflow detected");
  // Try to borrow a key from the left
  if (borrow_from_left(n, indent)) {
    // If it fails borrow from right
    if (borrow_from_right(n, indent)) {
      // If it fails merge with left node, but
      // then we must recurse
      short   parent_pos = 0;
      NODE_T *par = n->parent;
      NODE_T *l = left_sibling(n, &parent_pos);

      if (l) {
         debug(indent, "merging with left node");
         parent_pos = merge_leaf_nodes(l, n, par, indent);
      } else {
         NODE_T *r = right_sibling(n, &parent_pos);
         debug(indent, "merging with right node");
         parent_pos = merge_leaf_nodes(n, r, par, indent);
      }
      debug(indent, "remove key at position %hd from parent %hd",
                    parent_pos, par->id);
      // After merging one node must be removed from the 
      // parent - recurse
      return delete_internal_node(par, parent_pos, indent+2);
    } else {
      return 0;
    }
  } else {
    return 0;
  }
  return -1;
}

static short delete_internal_key(NODE_T    *n,
                                 char      *key,
                                 short indent) {
    // -1 if there is something wrong, 0 if OK
    short pos = 1;
    short ret = -1;
    int   cmp = -1;
    char  is_root = 0;

    assert(key && n && !_is_leaf(n));
    if (debugging()) {
      debug_no_nl(indent, "searching node %hd: ", n->id);
      bpltree_show_node(n, 0);
    }
    if (n->parent == NULL) {
      is_root = 1;  // Special case
    }
    while ((pos <= n->keycnt)
           && ((cmp = bpltree_keycmp(key,
                                     n->node.internal.k[pos].key,
                                     KEYSEP)) > 0)) {
      pos++;
    }
    if (cmp <= 0) {
      ret = delete_key(n->node.internal.k[pos-1].bigger, key, indent+2);
      if ((ret == 0) && (cmp == 0)) {
        debug(indent, "** originally found at position %hd in node %hd",
                      pos, n->id);
        // Check whether the key is still where we believe it is
        // in the node - some reorgs may have been triggered by
        // deletions from the subtree
        pos = 1;
        while ((pos <= n->keycnt)
               && ((cmp = bpltree_keycmp(key,
                                         n->node.internal.k[pos].key,
                                         KEYSEP)) > 0)) {
          pos++;
        }
        if ((cmp == 0)
            && (!is_root
                || (is_root && (n == bpltree_root())))) {
          // Don't do anything if we were at the root and 
          // it is gone!
          debug(indent, "** found at position %hd in internal node %hd",
                        pos, n->id);
          // Note that the previous key and the succeeding key of
          // a key inside an internal node are always in a leaf.
          // There is always a previous key for an internal node
          debug(indent, "finding previous key");
          NODE_T *prev
              = leaf_with_greatest_key(n->node.internal.k[pos-1].bigger);

          assert(prev);
          if (bpltree_numeric()) {
            debug(indent, "previous key %d in leaf node %hd",
                  *((int*)(prev->node.leaf.k[prev->keycnt-1].key)),
                  prev->id);
          } else {
            debug(indent, "previous key %s in leaf node %hd",
                  prev->node.leaf.k[prev->keycnt-1].key,
                  prev->id);
          }
          if (n->node.internal.k[pos].key) {
            free(n->node.internal.k[pos].key);
          }
          n->node.internal.k[pos].key
                 = key_duplicate(prev->node.leaf.k[prev->keycnt-1].key);
          // Done ...
        } 
      }
      return ret;
    } else {
      // The search key is bigger than all keys in the node
      return delete_key(n->node.internal.k[n->keycnt].bigger, key, indent+2);
    }
    debug(indent, "removal failed");
    return -1;
}

static short delete_leaf_key(NODE_T    *n,
                             char      *key,
                             short indent) {
    // -1 if there is something wrong, 0 if OK
    // The only real deletion
    short pos = 0;
    int   cmp = -1;

    assert(key && n && _is_leaf(n));
    if (debugging()) {
      debug_no_nl(indent, "searching node %hd: ", n->id);
      bpltree_show_node(n, 0);
    }
    while ((pos < n->keycnt)
           && ((cmp = bpltree_keycmp(key, n->node.leaf.k[pos].key,
                                     KEYSEP)) > 0)) {
      pos++;
    }
    if (cmp == 0) {
      // We've found it
      debug(indent, "** found at position %hd", pos);
      if (debugging()) {
        debug_no_nl(indent, "before calling delete_node:");
        bpltree_show_node(n, 0);
      }
      return delete_leaf_node(n, pos, indent);
    }
    debug(indent, "key not found");
    return -1;
}

static short delete_key(NODE_T    *n,
                        char      *key,
                        short indent) {
    if (_is_leaf(n)) {
      return delete_leaf_key(n, key, indent);
    } else {
      return delete_internal_key(n, key, indent);
    }
}

extern int bpltree_delete(char *key) {
    int      val;
    int      ret;

    if (bpltree_numeric()) {
      if (sscanf(key, "%d", &val) == 0) {
        fprintf(stdout, "Tree only contains numerical values\n");
        return -1;
      }
    }
    if (bpltree_numeric()) {
      ret = delete_key(bpltree_root(), (char *)&val, 0);
    } else {
      ret = delete_key(bpltree_root(), key, 0);
    }
    /*
    if (debugging()) {
      if (bpltree_check(bpltree_root(), (char *)NULL)) {
        bpltree_display(bpltree_root(), 0);
        assert(0);  // Force exit
      }
    }
    */
    return ret;
}
