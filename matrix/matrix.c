/** @file */

#include <stdlib.h>
#include "dlx.h"

/**
 * @brief Convert 2D matrix with given dimensions to a sparse 2D node array.
 * @return a pointer suitable for use in the dlx algorithm, or NULL on failure
 */
hnode *make_sparse(const int *matrix, size_t rows, size_t columns)
{
    size_t i, j;
    hnode *h;
    node *pa, *pb;
    size_t *ids;

    /* allocate column headers and root node */
    h = malloc(sizeof(*h) * (columns + 1));

    /* alloc size_t[] for column header IDs; root node has no ID */
    ids = malloc(sizeof(*ids) * columns);

    if (h == NULL || ids == NULL)
        return NULL;

    /* connect all header nodes in a linked list */
    for (i = 1; i < columns; i++) {
        pb = (node *) (h + i);
        pb->left    = (node *) (h + i - 1);
        pb->right   = (node *) (h + i + 1);
        pb->down    = pb;
        pb->up      = pb;
        /* h->chead unused for headers */

        h[i].s      = 0;
        h[i].id     = ids + i - 1;
        ids[i - 1]  = i - 1;
    }

    /* special code to connect first and last nodes */
    pb = (node *) h;
    pb->left    = (node *) (h + columns);
    pb->right   = (node *) (h + 1);
    pb->up      = pb;
    pb->down    = pb;
    h->id       = NULL;
    h->s        = 0;

    pb = (node *) (h + columns);
    pb->left    = (node *) (h + columns - 1);
    pb->right   = (node *) (h);
    pb->up      = pb;
    pb->down    = pb;
    h[columns].s        = 0;
    h[columns].id       = ids + columns - 1;
    ids[columns - 1]    = columns - 1;

    /* iterate through matrix, adding one row at a time */
    for (i = 0; i < rows; i++) {
        pa = NULL;  /* new empty row */
        for (j = 0; j < columns; j++) {
            if (matrix[i * columns + j] == 0) 
                continue;

            /* else, allocate new node; insert into column and row */
            pb = malloc(sizeof(node));
            if (pb == NULL)
                return NULL;

            /* insert pb into column */
            pb->chead   = h + j + 1;
            pb->up      = ((node *) pb->chead)->up;
            pb->down    = (node *) pb->chead;
            pb->up->down    = pb;
            pb->down->up    = pb;

            /* insert pb into row */
            if (pa != NULL) {
                pb->left    = pa;
                pb->right   = pa->right;
                pb->right->left = pb;
                pa->right       = pb;
            } else {
                pb->left    = pb;
                pb->right   = pb;
            }
            pa = pb;

            /* update column header's node count */
            (pb->chead->s)++;
        }
    }

    return h;
}

