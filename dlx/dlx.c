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

/**
 * @brief removes c from the header list and all rows it intersects from each
 * of their columns.
 */
static void cover(hnode *c)
{
    node *cb;       /* c base: cast c to (node *) */
    node *i, *j;

    cb = (node *) c;
    /* drop c out of the column headers list */
    cb->right->left = cb->left;
    cb->left->right = cb->right;

    /* for each row i in the column ... */
    i = cb;
    while ((i = i->down) != cb) {
        /* drop the entire row containing i from the array, but leave the
         * column itself alone */
        j = i;
        while ((j = j->right) != i) {
            /* drop node from its column */
            j->down->up = j->up;
            j->up->down = j->down;
            (j->chead->s)--;        /* update column node count */
        }
    }
}

/**
 * @brief restores all rows c intersects to their respective columns, then
 * inserts c back into the header list
 */
static void uncover(hnode *c)
{
    node *cb;       /* c base: cast c to (node *) */
    node *i, *j;

    cb = (node *) c;
    /* for each row i in column ... 
     * traversed in opposite order from cover() */
    i = cb;
    while ((i = i->up) != cb) {
        /* restore all other nodes in the row to their columns */
        j = i;
        while ((j = j->left) != i) {
            (j->chead->s)++;        /* update column node count */
            /* restore node to its column */
            j->down->up = j;
            j->up->down = j;
        }
    }

    /* restore c back to the column headers list */
    cb->right->left = cb;
    cb->left->right = cb;
}

/**
 * @brief Exact cover DLX algorithm by Knuth, adapted to C.
 * @return 0 if no solution, size of solution otherwise
 */
size_t dlx_exact_cover(node *solution[], hnode *h, size_t k)
{
    node *hb;       /* h base: cast h to (node *) */
    size_t min, n;  /* for finding column with min s */
    node *i, *j, *cb;

    hb = (node *) h;

    /* if array has no columns left, we are done */
    if (hb->right == hb) {
        /* Knuth's version: print solutions here, and break out of the recursive
         * call stack somehow.  In order for this to be general enough to allow
         * the client code to print the solutions however it wants, we have to
         * unwind the stack all the way back to the client while keeping the
         * solutions intact.  A nonzero return value signifies a solution has
         * been found (of size k), so don't touch it.
         */
        return k;
    }

    /* find a column "*cb" with min size "min" */
    min = -1u;
    i   = hb;
    cb  = NULL;
    while ((i = i->right) != hb) {
        n = ((hnode *) i)->s;
        if (n < min) {
            min = n;
            cb  = i;
        }
    }

    cover((hnode *) cb);

    n = 0;      /* return value if cb is empty */
    /* for each row i in column ... */
    i = cb;
    while ((i = i->down) != cb) {
        /* guess the row as solution */
        solution[k] = i;
        /* cover all of the columns in the row */
        j = i;
        while ((j = j->right) != i) 
            cover(j->chead);

        n = dlx_exact_cover(solution, h, k + 1);     /* recurse */

        /* restore the matrix links; makes freeing it later easier for the
         * client */
        j = i;
        while ((j = j->left) != i)
            uncover(j->chead);

        /* if the recursive calls succeeded we need to return immediately or the
         * outer while loop will indiscriminately try all the other rows,
         * wasting time and overwriting the solution
         */
        if (n > 0) 
            return n;
    }

    uncover((hnode *) cb);  /* restore matrix links as above */

    return n;
}

/**
 * @brief Extra utility function to modify the matrix by selecting row r and
 * covering all columns it covers.  
 *
 * This can be useful if you want to force a certain row to be included in the
 * solution (hence the name).
 */
void dlx_force_row(node *r)
{
    node *i;

    i = r;
    do {
        cover(i->chead);
    } while ((i = i->right) != r);
}

/**
 * @brief utility function to properly initialize n pre-allocated column headers
 * and the root node h into a circularly linked list.  The id member is left
 * untouched.
 *
 * @param h         pointer to root node
 * @param headers   pre-allocated chunk of n hnodes
 * @param n         number of column headers, not including root node h
 * @return h
 */
hnode *dlx_make_headers(hnode *h, hnode *headers, size_t n)
{
    node *hb, *ni;
    size_t i;

    /* set up the root node */
    hb = (node *) h;
    hb->left    = (node *) (headers + n - 1);
    hb->right   = (node *) headers;
    hb->up      = NULL;
    hb->down    = NULL;
    hb->chead   = NULL;
    h->s    = 0;

    /* set up the rest of the column headers:
     * left and right links point left and right
     * up and down links point to self
     * chead not used
     * initial size s is 0
     * id is the corresponding id entry
     */

    /* first column header */
    ni = (node *) headers;
    ni->left    = hb;
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
        headers[i].s    = 0;
    }

    /* last column header */
    ni = (node *) (headers + n - 1);
    ni->left    = (node *) (headers + n - 2);
    ni->right   = hb;
    ni->up      = ni;
    ni->down    = ni;
    ni->chead   = NULL;
    headers[n - 1].s    = 0;

    return h;
}

/**
 * @brief make a circularly linked node row out of row r of the pre-allocated
 * r by n nodes array, then insert the row into the correct columns determined
 * by the n col_ids.
 *
 * @param nodes pre-allocated node block of size at least r rows by n columns
 * @param r     allocated row of nodes array to initialize
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
    ni->up->down = ni;
    ni->down->up = ni;
    (ni->chead->s)++;

    ni++;

    /* 2nd to 2nd from last nodes */
    for (i = 1; i < n - 1; i++, ni++) {
        ni->left    = ni - 1;
        ni->right   = ni + 1;
        ni->chead   = headers + cols[i];

        ni->up      = ((node *) ni->chead)->up;
        ni->down    = (node *) ni->chead;
        ni->up->down    = ni;
        ni->down->up    = ni;
        (ni->chead->s)++;
    }

    /* last node */
    ni->left  = ni - 1;
    ni->right = nodes;
    ni->chead = headers + cols[n - 1];
    ni->up    = ((node *) ni->chead)->up;
    ni->down  = (node *) ni->chead;
    ni->up->down    = ni;
    ni->down->up    = ni;
    (ni->chead->s)++;
}

