#include <stdio.h>
#include "dlx.h"
#include "matrix.h"

int main(int argc, char *argv[])
{
    int matrix[][7] = { 
        {0, 0, 1, 0, 1, 1, 0},
        {1, 0, 0, 1, 0, 0, 1},
        {0, 1, 1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0, 0, 1},
        {0, 0, 0, 1, 1, 0, 1}
    };

    int i, j;
    node *solutions[] = {NULL, NULL, NULL, NULL, NULL, NULL};
    node *pi, *pj;

    j = dlx_exact_cover(solutions, make_sparse(matrix[0], 6, 7), 0);

    printf("solution size: %d\n", j);

    for (i = 0; i < j; i++) {
        pj = pi = solutions[i];
        do {
            printf("%d ", * (int *) (pj->chead->id));
        } while ((pj = pj->right) != pi);
        putchar('\n');
    }

    return 0;
}
