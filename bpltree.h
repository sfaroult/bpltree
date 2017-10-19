#ifndef BTREE_H

#define BTREE_H

#define DEF_MAX_KEYS  4
#define DEF_FILL_RATE 0.5 

#define KEYSEP       ':'

#define _is_leaf(n)  (n->is_leaf)
#define MIN_KEYS     (int)(bpltree_maxkeys() * bpltree_fillrate())

struct node_t;

// Convenience structures
typedef struct redirect_t {
            char          *key;
            struct node_t *bigger;
                     // Pointer to the subtree that contains
                     // keys bigger than the current one
                     // and smaller than the key to the right
        } REDIRECT_T;

typedef struct key_pos_t {
            char    *key;
            off_t    pos;  // We assume that the value we look for
                           // is an offset in a file
        } KEY_POS_T; 

// Node structures
typedef struct internal_node_t {
          REDIRECT_T         *k;      // 1 + maximum number of keys
         } INTERNAL_NODE_T;

typedef struct leaf_node_t {
          KEY_POS_T     *k;      // 1 + maximum number of keys
          struct node_t *next;   // Link leaf nodes for range searches
         } LEAF_NODE_T;

typedef struct node_t {
          char                is_leaf;  // Flag
          short               id;       // For educational purposes
          short               keycnt;
          struct node_t      *parent;   // Helps when deleting
          union {
             INTERNAL_NODE_T  internal;
             LEAF_NODE_T      leaf;
          } node;
        } NODE_T;

// Convenience structure (key location)
typedef struct keyloc_t {
          NODE_T  *n;
          short    pos;
         } KEYLOC_T;

extern void     bpltree_setfilesep(char sep);
extern char     bpltree_filesep(void);
extern void     bpltree_setnumeric(void);
extern char     bpltree_numeric(void);
extern void     bpltree_setmaxkeys(short n);
extern short    bpltree_maxkeys(void);
extern float    bpltree_fillrate(void);
extern NODE_T  *bpltree_root(void);
extern void     bpltree_setroot(NODE_T *n);
extern int      bpltree_insert(char *key, unsigned long val);
extern int      bpltree_delete(char *key);
extern void     bpltree_search(char *key);
extern void     bpltree_free(void);
extern void     bpltree_show_node(NODE_T *n, short indent);
extern void     bpltree_display(NODE_T *n, int blanks);
extern int      bpltree_keycmp(char *k1, char *k2, char sep);
extern KEYLOC_T bpltree_find_key(char *key);
extern int      bpltree_get(char *key, FILE *fp, char show_data);
extern int      bpltree_scan(char *key, FILE *fp, char show_data);
// For debugging
extern char     bpltree_check(NODE_T *n, char *prev_key);


extern char    *key_duplicate(char *key);
extern NODE_T  *new_node(NODE_T *parent, char leaf);
extern NODE_T  *left_sibling(NODE_T *n, short *sep_pos);
extern NODE_T  *right_sibling(NODE_T *n, short *sep_pos);
extern NODE_T  *find_node(NODE_T *tree, char *key);
extern short    find_pos(NODE_T *n, char *key, char present, short lvl);

#endif
