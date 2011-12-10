/** @file */
#include "dlx.h"

/* Summary of Knuth's DLX idea:
 * (1) Remove x from list:
 *      x->left->right = x->right;
 *      x->right->left = x->left;
 * (2) Restore x to its original position:
 *      x->left->right = x;
 *      x->right->left = x;
 */

/* All algorithms taken straight out of Knuth's DLX paper */

/** @brief Utility function to remove a node from its horizontal left-right list */
static void remove_lr(node *n)
{
    n->left->right = n->right;
    n->right->left = n->left;
}

/** @brief Utility function to remove a node from its vertical up-down list */
static void remove_ud(node *n)
{
    n->up->down = n->down;
    n->down->up = n->up;
}

/** @brief Utility function to restore a node to its horizontal left-right list */
static void insert_lr(node *n)
{
    n->left->right = n->right->left = n;
}

/** * @brief Utility function to restore a node to its vertical up-down list */
static void insert_ud(node *n)
{
    n->up->down = n->down->up = n;
}

/* A node has been removed from its list if and only if both neighbors
 * do not point to itself.
 *
 * It is not possible for a node to be half in the list, so only one side is
 * checked.
 */

/** @return 1 if node has been removed from its up-down list, 0 otherwise */
static int is_removed_ud(node *n)
{
    return n->up->down != n;
}


/**
 * @brief removes c from the header list and all rows it intersects from each
 * of their columns.
 */
static void cover(hnode *chead)
{
    node *i, *j;
    node *c = (node *) chead;

    /* An illustration: x represents nodes to remove.
     *                  c is a column header.
     *                  n is a normal node
     *
     * c c c x c c
     * n   n   n
     *   x   x x
     *   n     n n
     *     x x   x
     */

    remove_lr(c);

    i = c;
    while ((i = i->down) != c) {
        j = i;
        while ((j = j->right) != i) {
            remove_ud(j);
            (j->chead->s)--;        /* update column node count */
        }
    }
}

/**
 * @brief restores all rows c intersects to their respective columns, then
 * inserts c back into the header list
 */
static void uncover(hnode *chead)
{
    node *i, *j;
    node *c = (node *) chead;

    /* for each row i in column ...
     * traversed in opposite order from cover() */
    i = c;
    while ((i = i->up) != c) {
        /* restore all other nodes in the row to their columns */
        j = i;
        while ((j = j->left) != i) {
            (j->chead->s)++;        /* update column node count */
            insert_ud(j);
        }
    }

    insert_lr(c);
}

/**
 * @brief Exact cover DLX algorithm by Knuth, adapted to C.
 * @return 0 if no solution, size of solution otherwise
 */
size_t dlx_exact_cover(node *solution[], hnode *root, size_t k)
{
    size_t min, n;  /* for finding column with min s */
    node *i, *j, *c;
    node *h = (node *) root;

    /* if array has no columns left, we are done */
    if (h->right == h) {
        /* Knuth's version: print solutions here, and break out of the recursive
         * call stack somehow.  In order for this to be general enough to allow
         * the client code to print the solutions however it wants, we have to
         * unwind the stack all the way back to the client while keeping the
         * solutions intact.
         */
        return k;
    }

    /* find a column "*c" with min size "min" */
    min = -1u;
    c  = NULL;
    i   = h;
    while ((i = i->right) != h) {
        n = ((hnode *) i)->s;
        if (n < min) {
            min = n;
            c  = i;
        }
    }

    cover((hnode *) c);

    n = 0;      /* return value if c is empty */
    /* guess each row in column c one at a time and recurse */
    i = c;
    while ((i = i->down) != c) {
        solution[k] = i;

        /* cover all of the columns in the new row */
        j = i;
        while ((j = j->right) != i)
            cover(j->chead);

        n = dlx_exact_cover(solution, root, k + 1);     /* recurse */

        /* restore the node links: uncover in reverse order */
        j = i;
        while ((j = j->left) != i)
            uncover(j->chead);

        /* if the recursive calls succeeded, a solution has been found with the
         * current row, don't bother with the rest
         */
        if (n > 0)
            break;
    }

    /* end of loop with no solution found,
     * so restore node links and backtrack */

    uncover((hnode *) c);

    return n;   /* n should be 0 */
}

/**
 * @brief Exact cover DLX algorithm by Knuth, adapted to C.
 * @param k     max number of solutions to find
 * @return (k - n) where n is the number of solutions found up to a max of k
 */
size_t dlx_has_covers(hnode *root, size_t k)
{
    size_t min, n;  /* for finding column with min s */
    node *i, *j, *c;
    node *h = (node *) root;

    /* if array has no columns left, we have found another solution */
    if (h->right == h) {
        /* internally, k = remaining number of solutions to try to find */
        return k - 1;
    }

    /* find a column "*c" with min size "min" */
    min = -1u;
    c  = NULL;
    i   = h;
    while ((i = i->right) != h) {
        n = ((hnode *) i)->s;
        if (n < min) {
            min = n;
            c  = i;
        }
    }

    cover((hnode *) c);

    /* guess each row in column c one at a time and recurse */
    i = c;
    while ((i = i->down) != c) {
        /* cover all of the columns in the new row */
        j = i;
        while ((j = j->right) != i)
            cover(j->chead);

        k = dlx_has_covers(root, k);

        /* restore the node links: uncover in reverse order */
        j = i;
        while ((j = j->left) != i)
            uncover(j->chead);

        if (k == 0)     /* reached max number of solutions */
            break;
    }

    /* end of loop with no solution found,
     * so restore node links and backtrack */

    uncover((hnode *) c);

    return k;
}

/**
 * @brief Extra utility function to modify the matrix by selecting row r and
 * covering all columns it covers.
 *
 * This can be useful if you want to force a certain row to be included in the
 * solution (hence the name).
 *
 * @return 0 on success, -1 if r had already been removed
 */
int dlx_force_row(node *r)
{
    node *i = r;
    if (is_removed_ud(r))
        return -1;

    do {
        cover(i->chead);
    } while ((i = i->right) != r);
    return 0;
}

/**
 * @brief Undoes a dlx_force_row.  Must be called in reverse order or else
 * links will not be restored properly.
 *
 * @return 0 on success, -1 if r has not been removed
 */
int dlx_unselect_row(node *r)
{
    node *i = r;
    if (!is_removed_ud(r))
        return -1;

    /* reverse order of dlx_force_row */
    while ((i = i->left) != r) {
        uncover(i->chead);
    }
    uncover(r->chead);
    return 0;
}

/**
 * @brief utility function to properly initialize n pre-allocated column headers
 * and the root node h into a circularly linked list.  The id member is left
 * untouched.
 *
 * @param h         pointer to root node
 * @param headers   pre-allocated contiguous chunk of n hnodes
 * @param n         number of column headers, not including root node h
 * @return h
 */
hnode *dlx_make_headers(hnode *root, hnode *headers, size_t n)
{
    node *h, *ni;
    size_t i;

    /* set up the root node */
    h = (node *) root;
    h->left     = (node *) (headers + n - 1);
    h->right    = (node *) headers;
    h->up       = NULL;
    h->down     = NULL;
    h->chead    = NULL;
    root->s     = 0;

    /* set up the rest of the column headers:
     * left and right links point left and right
     * up and down links point to self
     * chead not used
     * initial size s is 0
     * id is not touched
     */

    /* first column header */
    ni = (node *) headers;
    ni->left    = h;
    ni->right   = (node *) (headers + 1);
    ni->up      = ni;
    ni->down    = ni;
    ni->chead   = NULL;
    headers->s  = 0;

    /* from 2nd to 2nd to last column header */
    for (i = 1; i < n - 1; i++) {
        ni = (node *) (headers + i);
        ni->left    = (node *) (headers + i - 1);
        ni->right   = (node *) (headers + i + 1);
        ni->up      = ni;
        ni->down    = ni;
        ni->chead   = NULL;
        ((hnode *) ni)->s   = 0;
    }

    /* last column header */
    ni = (node *) (headers + n - 1);
    ni->left    = (node *) (headers + n - 2);
    ni->right   = h;
    ni->up      = ni;
    ni->down    = ni;
    ni->chead   = NULL;
    ((hnode *) ni)->s   = 0;

    return root;
}

/**
 * @brief make a circularly linked node row of n nodes out of the pre-allocated
 * nodes array of size n, then insert the row into the correct columns
 * determined by the n col_ids.
 *
 * @param nodes     contiguous, pre-allocated node block of size n
 * @param headers   contiguous, pre-initialized column headers array
 * @param cols      int[n] containg column indices in increasing order
 * @param n     number of nodes per row in nodes
 */
void dlx_make_row(node *nodes, hnode *headers, int cols[], size_t n)
{
    size_t i;
    node *ni;

    ni = nodes;

    /* first node */
    ni->left  = ni + n - 1;
    ni->right = ni + 1;
    ni->chead = headers + cols[0];
    ni->up    = ((node *) ni->chead)->up;
    ni->down  = (node *) ni->chead;
    insert_ud(ni);
    (ni->chead->s)++;

    ni++;

    /* 2nd to 2nd from last nodes */
    for (i = 1; i < n - 1; i++, ni++) {
        ni->left    = ni - 1;
        ni->right   = ni + 1;
        ni->chead   = headers + cols[i];

        ni->up      = ((node *) ni->chead)->up;
        ni->down    = (node *) ni->chead;
        insert_ud(ni);
        (ni->chead->s)++;
    }

    /* last node */
    ni->left  = ni - 1;
    ni->right = nodes;
    ni->chead = headers + cols[n - 1];
    ni->up    = ((node *) ni->chead)->up;
    ni->down  = (node *) ni->chead;
    insert_ud(ni);
    (ni->chead->s)++;
}

