/**
 * @file
 * @author Jimmy Wu
 * @brief The Sudoku module handles the conversion between sudoku puzzles and
 * the 0/1 exact cover constraint matrix form that DLX uses.
 *
 * Sudoku puzzle input format is a single char * string that is 81 characters
 * long, one for each cell of the puzzle.  The numbers 1 - 9 are represented by
 * the corresponding ASCII characters, and blanks are represented by hypens
 * (0x2D).  The cell order goes left to right, top to bottom, and are numbered
 * from 1 to 81.  Rows are numbered from 1 to 9, top to bottom.  Columns are
 * numbered 1 to 9, left to right.  3x3 block regions are numberd 1 - 9 in the
 * same order as the cells (left to right, top to bottom).
 *
 * In ASCII art, this looks like:
 *
 * <pre>
 *      1   2   3    4    ...
 *   ++===+===+===++===+= ...
 * 1 || 1 | 2 | 3 || 4 |  ...
 *   ++---+---+---++---+- ...
 * 2 ||10 |11 |12 ||13 |  ...
 *   ++---+---+---++---+- ...
 * 3 ||19 |20 |21 ||22 |  ...
 *   ++===+===+===++===+= ...
 * 4 ||28 |29 |30 ||31 |  ...
 *   ++---+---+---++---+- ...
 * . ..   .   .   ..   .  .
 * . ..   .   .   ..   .   .
 * . ..   .   .   ..   .    .
 * </pre>
 *
 * Sudoku has NTYPES = 4 types of constraints: 
 *
 *   -# "Cell": each cell must be filled by exactly one number.
 *      There are 81 cells for 81 constraint columns total.
 *   -# "Row" : each row must have exactly 1 of each number.
 *      There are 9 rows and 9 numbers for 81 constraint columns total.
 *   -# "Column": like row but with columns instead.
 *      9 columns with 9 numbers each make 81 constraint columns.
 *   -# "Region": like row and column but with the 3x3 regions.
 *      9 regions with 9 numbers each make 81 constraint columns.
 *
 * Thus the total number of constraint columns is 81*4 = 324 = NCOLS.  There are
 * 9 possible ways to fill cell for a total of 9*81 = 729 = NROWS rows.  Since
 * each filled cell satisfies exactly NTYPES = 4 constraints, each row has only
 * NTYPES = 4 elements.  Thus the data structures used include:
 *
 *   - a root header node which is located in the column header list
 *   - 324 column headers 
 *   - 324 column id's
 *   - 729x4 internal nodes
 *   - 81 rows in solution
 */

#include <assert.h>
#include <stdlib.h>
#include "dlx.h"
#include "sudoku.h"

/**
 * @brief Fills col_ids with the NTYPES column ids satisfied by placing number
 * "n" at row r column c, in ascending order.
 */
static void get_ids(int col_ids[], int r, int c, int n) 
{
     /* shortcut to find region number, using integer division truncation */
    int R = (r - 1) / 3 * 3 + (c - 1) / 3 + 1;

    /* The constraint to id mapping is as follows:
     *
     * The 81 cell id's come first, then 81 row id's, then column id's, and
     * finally 81 region id's.  The code shows the formulas to obtain each from
     * r, c, n.  All the subtracted constants are to correct for all three of r,
     * c, n being 1-indexed, while the ids are 0-indexed.
     */
    col_ids[CELL_ID]    = CELL_ID   * 81 + (9 * r) - 9 + c - 1;
    col_ids[ROW_ID]     = ROW_ID    * 81 + (9 * r) - 9 + n - 1;
    col_ids[COL_ID]     = COL_ID    * 81 + (9 * c) - 9 + n - 1;
    col_ids[REGION_ID]  = REGION_ID * 81 + (9 * R) - 9 + n - 1;
}

/**
 * @brief helper function to colid2row: computes as many of the parameters r, c,
 * n, R as it can given the input col id.
 */
static void fill_values(int col, int *r, int *c, int *n, int *R)
{
    if (col < (CELL_ID + 1) * 81) {
        *r = (col - CELL_ID   * 81) / 9 + 1;
        *c = (col - CELL_ID   * 81) % 9 + 1;
    } else if (col < (ROW_ID + 1) * 81) {
        *r = (col - ROW_ID    * 81) / 9 + 1;
        *n = (col - ROW_ID    * 81) % 9 + 1;
    } else if (col < (COL_ID + 1) * 81) {
        *c = (col - COL_ID    * 81) / 9 + 1;
        *n = (col - COL_ID    * 81) % 9 + 1;
    } else { /* (col < (REGION_ID + 1) * 81) */
        *R = (col - REGION_ID * 81) / 9 + 1;
        *n = (col - REGION_ID * 81) % 9 + 1;
    }
}

/**
 * @brief Takes any 3 distinct column id's and computes the row index according
 * to the ordering described in init().
 *
 * @return row index according to ordering described in init().
 */
static size_t colid2row(int col1, int col2, int col3)
{
    int r, c, n, R;
    r = c = n = R = 0;

    fill_values(col1, &r, &c, &n, &R);
    fill_values(col2, &r, &c, &n, &R);
    fill_values(col3, &r, &c, &n, &R);

    /* with 3 values we are guaranteed to have r, c, and n */
    return ((9 * r) - 9 + c - 1) * 9 + n - 1;
}


/**
 * @brief creates a full dlx array with 324 columns and 729 rows, corresponding
 * to the entire search space with nothing eliminated.  All storage must be
 * pre-allocated.  Every argument is initialized to appropriate values.  The
 * rows are grouped by cell, in the standard cell order described in the file
 * header comments.  Thus, the first row corresponds to 1 in (1,1), the 2nd row
 * is a 2 in (1,1), all the way up to the last row being a 9 in (9,9).
 */
static void init(hnode *h, hnode columns[], int ids[], node nodes[][NTYPES])
{
    int i, j, k;
    size_t r;
    int col_ids[NTYPES];

    /* set up circularly linked list */
    dlx_make_headers(h, columns, NCOLS);

    /* initialize id member in all header nodes */
    /* initialize ids array */
    for (i = 0; i < NCOLS; i++) {
        columns[i].id = ids + i;
        ids[i] = i;
    }
    h->id = NULL;

    /* add the 729 rows to the matrix.  This is done by looping through all 729
     * cell-number combinations, calculating the correct column ids, and writing
     * the node links to the pre-allocated nodes array accordingly.
     */
    r = 0;          /* row index for nodes[r][] array */
    for (i = 1; i < 9 + 1; i++)             /* row    */
        for (j = 1; j < 9 + 1; j++)         /* column */
            for (k = 1; k < 9 + 1; k++) {   /* number */
                /* (i, j, k) is the number k in row i, column j */
                get_ids(col_ids, i, j, k);
                dlx_make_row(nodes[0] + NTYPES * r, columns, col_ids, NTYPES);
                r++;
            }
}

/**
 * @brief solvez puzzle and puts solution in buf
 * @param buf   char array, must be at least 82 characters long to hold
 * solution.
 * @return -1 if unsolveable, 0 if solution found.
 */
int sudoku_solve(const char *puzzle, char *buf)
{
    /* data structures as described in file comment above */
    hnode h;
    hnode headers[NCOLS];
    int   ids[NCOLS];
    node  nodes[NROWS][NTYPES];
    node  *solution[81];

    /* other local variables */
    int c;
    size_t s, n, i;
    node *ni;

    /* char * representation to dlx conversion algorithm:
     * fill the dlx sparse matrix with all possible rows, then apply the givens
     * one at a time using the forced row selection of the dlx module
     */

    init(&h, headers, ids, nodes);

    /* process givens: iterate through all cells */
    n = 0;      /* num givens found so far */
    for (i = 0; i < 81; i++) {
        /* could also just use isdigit, but w/e */
        c = puzzle[i] - '0';
        if (c > 0 && c <= 9) { /* c is a digit */
            /* given how row order from init matches input cell order, row index
             * is simple to calculate
             * pick any node in the row (i.e. first one), and force select it */
            ni = nodes[i * 9 + c - 1];
            dlx_force_row(ni);
            solution[n++] = ni;     /* add given row to solution, increment count */
        }
    }

    s = dlx_exact_cover(solution + n, &h, 0);
    s += n;

    if (s < 81)     /* no solution found */
        return -1;

    assert(s == 81);

    /* else, process solution into string form */
    for (i = 0; i < s; i++) {
        n = colid2row(*(int *) (solution[i]->chead->id),
                      *(int *) (solution[i]->right->chead->id),
                      *(int *) (solution[i]->right->right->chead->id));
        buf[n / 9] = n % 9 + '1';
    }
    buf[81] = '\0';

    return 0;
}
