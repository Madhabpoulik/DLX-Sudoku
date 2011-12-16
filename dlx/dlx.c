/**
 * @file
 * @brief Implementation of Donald Knuth's
 * <a href="http://www-cs-faculty.stanford.edu/~uno/papers/dancing-color.ps.gz">
 * Dancing Links Algorithm</a>.
 *
 * All algorithms taken straight out of Knuth's DLX paper, translated fairly
 * literally into C.
 */
#include "dlx.h"

/* Summary of fundamental idea behind Knuth's DLX algorithm:
 * (1) Remove x from list:
 *      x->left->right = x->right;
 *      x->right->left = x->left;
 * (2) Restore x to its original position:
 *      x->left->right = x;
 *      x->right->left = x;
 */

/**
 * @name GROUP_STATIC_NODE_UTILS
 * Private utility functions for manipulating node links
 * @{
 */

/** @brief Remove node n from its left-right list */
static void remove_lr(node *n)
{
    n->left->right = n->right;
    n->right->left = n->left;
}

/** @brief Remove node n from its up-down list */
static void remove_ud(node *n)
{
    n->up->down = n->down;
    n->down->up = n->up;
}

/** @brief Restore node n to its left-right list */
static void insert_lr(node *n)
{
    n->left->right = n->right->left = n;
}

/** @brief Restore node n to its up-down list */
static void insert_ud(node *n)
{
    n->up->down = n->down->up = n;
}

/* A node has been removed from its list if and only if both neighbors
 * do not point to itself.  However, it is not possible for a node to be half
 * in the list, so only one side is checked.
 */
/** @return 1 if node has been removed from its up-down list, 0 otherwise */
static int is_removed_ud(node *n)
{
    return n->up->down != n;
}

/**
 * @brief Insert new node n into bottom of column c. 
 *
 * @param n     new node to insert.  n  must not be in the column, or else this
 *              function will break the matrix horribly.
 */
static void column_append_node(hnode *c, node *n)
{
    node *cn = (node *) c;

    n->chead = c;
    n->up    = cn->up;
    n->down  = cn;
    insert_ud(n);
}

/**
 * @brief Remove c from the header list and all its rows from each of their
 * columns except for column c itself.
 */
static void cover(hnode *chead)
{
    node *i, *j;
    node *c = (node *) chead;

    /* An illustration: x represents nodes to remove from up-down list,
     *                  r is row nodes that don't get touched,
     *                  c is a column header,
     *                  n is a normal node.
     *
     * c c c x c c
     * n   n   n
     *   x   r x
     *   n     n n
     *     x r   x
     */

    remove_lr(c);

    i = c;
    while ((i = i->down) != c) {    /* for each row, except c */
        j = i;
        while ((j = j->right) != i) {   /* for each node except i */
            remove_ud(j);
            (j->chead->s)--;            /* update column node count */
        }
    }
}

/**
 * @brief Restore all rows in c to their respective columns, then
 * insert c back into the header list.  
 *
 * Must be called in exact reverse order as cover() to ensure matrix is
 * correctly restored to original state.
 */
static void uncover(hnode *chead)
{
    node *i, *j;
    node *c = (node *) chead;

    /* all loops must traverse in opposite order from cover() */
    i = c;
    while ((i = i->up) != c) {      /* for each row except c */
        j = i;
        while ((j = j->left) != i) {    /* for each node except i */
            (j->chead->s)++;            /* update column node count */
            insert_ud(j);
        }
    }

    insert_lr(c);
}

/**
 * @return a column header with the smallest s field, or NULL if column list is
 *          empty
 */
static hnode *min_hnode_s(hnode *root)
{
    size_t n;
    size_t min = -1u;
    node *h = (node *) root;
    node *i = h;
    node *c = NULL; /* return value */

    while ((i = i->right) != h) {
        n = ((hnode *) i)->s;
        if (n < min) {
            min = n;
            c   = i;
        }
    }
    return (hnode *) c;
}

/** @} */

/**
 * @name GROUP_DLX_ALGORITHMS
 * Variations on the core DLX algorithm by Knuth.
 * @{
 */

/**
 * @brief Exact cover DLX algorithm by Knuth, adapted to C.
 * @param k     used internally; must set to 0 for the algorithm to work
 *              properly
 * @return 0 if no solution, size of solution otherwise
 */
size_t dlx_exact_cover(node *solution[], hnode *root, size_t k)
{
    size_t n;
    node *i, *j, *cn;
    hnode *c;
    node *h = (node *) root;

    /* if array has no columns left, we are done */
    if (h->right == h) {
        /* Knuth's version: print solutions here, and halt.
         * In order for this to be general enough to pass the solution back to 
         * the client code, we have to unwind the stack all the way back to the
         * client while keeping the solutions intact.  */
        return k;
    }

    c = min_hnode_s(root);

    cover(c);

    cn = (node *) c;
    n = 0;      /* return value if column c is empty */

    /* guess each row in column c one at a time and recurse */

    i = cn;
    while ((i = i->down) != cn) {
        solution[k] = i;

        /* cover all of the other columns in the new row */
        j = i;
        while ((j = j->right) != i)
            cover(j->chead);

        n = dlx_exact_cover(solution, root, k + 1);     /* recurse */

        /* restore the node links: uncover in reverse order */
        j = i;
        while ((j = j->left) != i)
            uncover(j->chead);

        /* If the recursive calls succeeded, a solution has been found with the
         * current row, so don't bother with the rest. */
        if (n > 0)
            break;
    }

    /* restore node links and backtrack */
    uncover(c);
    return n;
}

/**
 * @brief Run exact cover DLX algorithm by Knuth, adapted to C, and also
 * include extra hint information.
 * @param k     used internally; must set to 0 for the algorithm to work
 *              properly
 * @return 0 if no solution, size of solution otherwise
 */
/* note: code is very similar to dlx_exact_cover; violation of DRY, I know, but
 * I haven't found a way around this yet.  The only differences are the parts
 * that deal with solution[].  */
size_t dlx_exact_cover_hints(dlx_hint solution[], hnode *root, size_t k)
{
    size_t n;
    node *i, *j, *cn;
    hnode *c;
    node *h = (node *) root;

    /* if array has no columns left, we are done */
    if (h->right == h)
        return k;

    /* find a column "*c" with min s field, "min" */
    c = min_hnode_s(root);

    cover(c);

    /* record column info for hint */
    solution[k].id = c->id;
    solution[k].s = c->s;

    cn = (node *) c;
    n = 0;      /* return value if c is empty */

    /* guess each row in column c one at a time and recurse */
    i = cn;
    while ((i = i->down) != cn) {
        solution[k].row = i;   /* record solution row */

        /* cover all of the columns in the new row */
        j = i;
        while ((j = j->right) != i)
            cover(j->chead);

        n = dlx_exact_cover_hints(solution, root, k + 1);     /* recurse */

        /* restore the node links: uncover in reverse order */
        j = i;
        while ((j = j->left) != i)
            uncover(j->chead);

        /* If the recursive calls succeeded, a solution has been found with the
         * current row, don't bother with the rest.  */
        if (n > 0)
            break;
    }

    /* restore node links and backtrack */
    uncover((hnode *) c);
    return n;
}

/**
 * @brief Exact cover DLX algorithm by Knuth, adapted to C.
 * @param k     max number of solutions to find
 * @return (k - n) where n is the number of solutions found, up to a max of k.
 *          In other words, the smallest return value is 0, and the largest is k.
 */
/* More violation of DRY since most of this is very similar to dlx_exact_cover.
 * However there are a few more key differences this time.  The first is the
 * return value has to do with the number of solutions as opposed to the number
 * of rows in the solution, if any, so any lines involving k are different.
 * The second is that solutions are not stored, so all references to solution[]
 * are removed. */
size_t dlx_has_covers(hnode *root, size_t k)
{
    node *i, *j, *cn;
    hnode *c;
    node *h = (node *) root;

    /* if array has no columns left, we have found another solution */
    if (h->right == h) {
        /* internally, k = remaining number of solutions to try to find */
        return k - 1;
    }

    c = min_hnode_s(root);

    cover(c);

    cn = (node *) c;

    /* guess each row in column c one at a time and recurse */
    i = cn;
    while ((i = i->down) != cn) {
        /* cover all of the columns in the new row */
        j = i;
        while ((j = j->right) != i)
            cover(j->chead);

        k = dlx_has_covers(root, k);

        /* restore the node links: uncover in reverse order */
        j = i;
        while ((j = j->left) != i)
            uncover(j->chead);

        /* recursive calls reached max number of solutions, so don't bother
         * looking for any more */
        if (k == 0)
            break;
    }

    /* restore node links and backtrack */
    uncover(c);

    return k;
}

/** @} */

/**
 * @name GROUP_DLX_FORCE_ROWS
 * Utility functions to force a certain row to be part of the solution (e.g.
 * useful for selecting predetermined givens in some problems) and undoing
 * those selections.  Note that unselections must be done in exact reverse
 * order of selections.
 * @{
 */

/**
 * @brief Modify the matrix by covering all columns that row r covers.
 *
 * This can be useful if you want to force a certain row to be included in the
 * solution (hence the name).
 *
 * @return 0 on success, -1 if r has already been removed from the matrix and
 * cannot be selected.
 */
int dlx_force_row(node *r)
{
    node *i = r;
    if (is_removed_ud(r))
        return -1;

    /* cover all of r's columns, starting with r->chead itself */
    do {
        cover(i->chead);
    } while ((i = i->right) != r);
    return 0;
}

/**
 * @brief Undo dlx_force_row.  Must be called in exact reverse order as
 * dlx_force_row for links to be restored properly.
 *
 * @return 0 on success, -1 if r is still in the matrix.
 */
int dlx_unselect_row(node *r)
{
    node *i = r;
    if (!is_removed_ud(r))
        return -1;

    /* reverse order of dlx_force_row; uncover all of r's columns, finishing
     * with r->chead last */
    do {
        uncover((i = i->left)->chead);
    } while (i != r);
    return 0;
}

/** @} */

/**
 * @name GROUP_MATRIX_INIT_UTILS
 * Utility functions to aid in setting up node links when constructing a DLX
 * matrix.
 * @{
 */

/**
 * @brief Make n pre-allocated column headers and the root node h into a
 * circularly linked left-right list.  The id member is not touched.
 *
 * @param h         pointer to root node
 * @param headers   pre-allocated contiguous chunk of n hnodes
 * @param n         number of column headers, not including root node h
 * @return h
 */
hnode *dlx_make_headers(hnode *root, hnode *headers, size_t n)
{
    node *h = (node *) root;
    node *ni;
    size_t i;

    /* set up the root node */
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
    ni->chead   = (hnode *) ni;
    headers->s  = 0;

    /* from 2nd to 2nd to last column header */
    for (i = 1; i < n - 1; i++) {
        ni = (node *) (headers + i);
        ni->left    = (node *) (headers + i - 1);
        ni->right   = (node *) (headers + i + 1);
        ni->up      = ni;
        ni->down    = ni;
        ni->chead   = (hnode *) ni;
        ((hnode *) ni)->s   = 0;
    }

    /* last column header */
    ni = (node *) (headers + n - 1);
    ni->left    = (node *) (headers + n - 2);
    ni->right   = h;
    ni->up      = ni;
    ni->down    = ni;
    ni->chead   = (hnode *) ni;
    ((hnode *) ni)->s   = 0;

    return root;
}

/**
 * @brief Make the pre-allocated nodes array of size n into a circularly linked
 * left-right list, then insert the row into the correct columns as determined
 * by the n column ids in cols[].
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
    column_append_node(headers + cols[0], ni);
    (ni->chead->s)++;

    ni++;

    /* from 2nd node to 2nd from last node */
    for (i = 1; i < n - 1; i++, ni++) {
        ni->left    = ni - 1;
        ni->right   = ni + 1;
        column_append_node(headers + cols[i], ni);
        (ni->chead->s)++;
    }

    /* last node */
    ni->left  = ni - 1;
    ni->right = nodes;
    column_append_node(headers + cols[n - 1], ni);
    (ni->chead->s)++;
}

/** @} */
