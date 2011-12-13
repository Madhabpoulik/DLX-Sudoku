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
 *
 * For how (r, c, n) maps back and forth to constraint column id's and row id's
 * see the get_ids / fill_values / hint2cells and row_id / hint2rcn functions
 */

#include <stdlib.h>
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
     * The 81 cell constraint id's come first, then 81 row constraint id's,
     * then column constraint id's, and finally 81 region constraint id's.  The
     * code shows the formulas to obtain each from r, c, n.  All the subtracted
     * constants are to correct for all three of r, c, n being 1-indexed, while
     * the ids are 0-indexed.
     */
    col_ids[CELL_ID]    = CELL_ID   * 81 + (9 * r) - 9 + c - 1;
    col_ids[ROW_ID]     = ROW_ID    * 81 + (9 * r) - 9 + n - 1;
    col_ids[COL_ID]     = COL_ID    * 81 + (9 * c) - 9 + n - 1;
    col_ids[REGION_ID]  = REGION_ID * 81 + (9 * R) - 9 + n - 1;
}

/** @return row index given digit n in row r col c */
static size_t row_id(int r, int c, int n)
{
    return ((9 * r) - 9 + c - 1) * 9 + n - 1;
}

/**
 * @brief helper function to colid2rowid: computes as many of the parameters r,
 * c, n, R as it can given the input col id.  Reverse of function get_ids.
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
static size_t row2row_id(node * rn)
{
    int r, c, n, R;
    r = c = n = R = 0;

    fill_values(*(int *) (rn->chead->id), &r, &c, &n, &R);
    rn = rn->right;
    fill_values(*(int *) (rn->chead->id), &r, &c, &n, &R);
    rn = rn->right;
    fill_values(*(int *) (rn->chead->id), &r, &c, &n, &R);

    /* with 3 values we are guaranteed to have r, c, and n */
    return row_id(r, c, n);
}


/**
 * @brief initializes the links in the preallocated nodes to a full sudoku dlx
 * array with 324 columns and 729 rows, corresponding to the entire search
 * space with nothing eliminated.  
 *
 * All storage must be pre-allocated; this function only does initialization.
 * The rows are grouped by cell, in the standard cell order described in the
 * file header comments.  Thus, the first row corresponds to 1 in (1,1), the
 * 2nd row is a 2 in (1,1), all the way up to the last row being a 9 in (9,9).
 */
static void init(sudoku_dlx *puzzle_dlx)
{
    int i, j, k;
    size_t r;
    int col_ids[NTYPES];

    /* set up circularly linked list */
    dlx_make_headers(&puzzle_dlx->root, puzzle_dlx->headers, NCOLS);

    /* initialize id member in all header nodes */
    /* initialize ids array */
    for (i = 0; i < NCOLS; i++) {
        puzzle_dlx->headers[i].id = puzzle_dlx->ids + i;
        puzzle_dlx->ids[i] = i;
    }
    puzzle_dlx->root.id = NULL;

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
                dlx_make_row(puzzle_dlx->nodes[r], puzzle_dlx->headers,
                             col_ids, NTYPES);
                r++;
            }
}

/**
 * @brief Remove givens from the full dlx matrix while making sure the puzzle
 * is still valid.
 *
 * @param solution  givens are added here in the order they are processed
 *
 * @return number of givens found, or some number > 81 if any givens conflict
 *         (which means puzzle is invalid and has no solution)
 */
static int
process_givens(const char *givens, sudoku_dlx *puzzle_dlx, node *solution[])
{
    size_t n, i;
    int c;
    node *ni;
    /* eliminate givens one at a time: iterate through all 81 cells */
    n = 0;      /* num givens found so far */
    for (i = 0; i < 81; i++) {
        /* could also just use isdigit, but w/e */
        c = givens[i] - '0';
        if (c > 0 && c <= 9) { /* c is a digit */
            /* given how row order from init matches input cell order, row index
             * is simple to calculate; pick any node in the row (i.e. first
             * one), and force select it */
            ni = puzzle_dlx->nodes[i * 9 + c - 1];
            if (dlx_force_row(ni) != 0) {
                /* non-zero return means ni has already been removed, meaning
                 * it conflicts with a previously encountered given, so puzzle
                 * is invalid; anything > 81 will do since you can only have
                 * 81 givens */
                n = 255;
                break;
            }
            solution[n++] = ni;     /* add given row to solution, increment count */
        }
    }
    return n;
}

/** @brief convert solution rows to 81 char string form */
static void to_simple_string(char *buf, node *solution[], size_t len)
{
    size_t n, i;
    for (i = 0; i < len; i++) {
        n = row2row_id(solution[i]); /* see init() comments for row id order */
        buf[n / 9] = n % 9 + '1';
    }
    buf[len] = '\0';
}

/**
 * @brief solves puzzle and puts solution in buf
 * @param puzzle    81 char string representing puzzle.  Cells go in order left
 *                  to right, top to bottom; char '1' - '9' represent
 *                  corresponding digits; any other char represents a blank
 * @param buf   char array, must be 82 characters long to hold
 *              solution and null terminator byte.
 * @return 0 if unsolveable, 1 if solution found.
 */
int sudoku_solve(const char *puzzle, char *buf)
{
    sudoku_dlx  puzzle_dlx;
    node        *solution[81];
    size_t      n;

    init(&puzzle_dlx);  /* make full sudoku dlx array */

    if ((n = process_givens(puzzle, &puzzle_dlx, solution)) > 81)
        return 0;      /* invalid givens, no solution possible */

    n += dlx_exact_cover(solution + n, &puzzle_dlx.root, 0);

    if (n < 81)     /* no solution found */
        return 0;

    to_simple_string(buf, solution, n);

    return 1;
}

/**
 * @brief Tries to find up to n solutions and returns one of them
 *
 * @return 0 if unsolvable, else, number of solutions found
 */
size_t sudoku_nsolve(const char *puzzle, char *buf, size_t n)
{
    sudoku_dlx  puzzle_dlx;
    node        *solution[81];
    size_t      s, a;

    init(&puzzle_dlx);
    if ((s = process_givens(puzzle, &puzzle_dlx, solution)) > 81)
        return 0;   /* invalid givens, no solution */

    a = dlx_has_covers(&puzzle_dlx.root, n);

    s += dlx_exact_cover(solution + s, &puzzle_dlx.root, 0);

    if (s < 81)     /* no solution */
        return 0;

    to_simple_string(buf, solution, s);

    return n - a;
}

/**
 * @brief solves puzzle with solution hints
 * @param puzzle    81 char string representing puzzle, plus null terminator.  
 *                  Cells go in order left to right, top to bottom; char '1' -
 *                  '9' represent corresponding digits; any other char
 *                  represents a blank.
 * @param hints     81 hints
 * @return 0 if unsolveable, 1 if solution found.
 */
int sudoku_solve_hints(const char *puzzle, sudoku_hint hints[])
{
    sudoku_dlx  puzzle_dlx;
    node        *solution[81];
    dlx_hint    dlx_hints[81];
    size_t      n, i;

    init(&puzzle_dlx);  /* make full sudoku dlx array */

    if ((n = process_givens(puzzle, &puzzle_dlx, solution)) > 81)
        return 0;      /* invalid givens, no solution possible */

    /* fill hints for the givens */
    for (i = 0; i < n; i++) {
        hints[i].constraint_id = *((int *) solution[i]->chead->id);
        hints[i].solution_id = row2row_id(solution[i]);
        hints[i].nchoices = 1;  /* it's a given; only 1 choice available */
    }

    n += dlx_exact_cover_hints(dlx_hints + n, &puzzle_dlx.root, 0);

    if (n < 81)     /* no solution found */
        return 0;

    /* fill hints */
    for (n = 0; n < 81; n++) {
        hints[n].constraint_id = *((int *) dlx_hints[n].row->chead->id);
        hints[n].solution_id = row2row_id(dlx_hints[n].row);
        hints[n].nchoices = dlx_hints[n].s;
    }

    return 1;
}

/** @brief convert hint to row, col, number */
void hint2rcn(sudoku_hint *hint, int *r, int *c, int *n)
{
    size_t row = hint->solution_id;
    *r = row / 81 + 1;
    *c = row / 9 % 9 + 1;
    *n = row % 81 + 1;
}

/**
 * @brief given hint, fill cell_ids with cell id's of cells the hint covers
 *
 * @return number of cells the hint covers; for standard sudoku, this is 1 or 9
 */
size_t hint2cells(sudoku_hint *hint, int cell_ids[])
{
    size_t i;
    int r, c, n, R; 
    int constraint_id = hint->constraint_id;
    r = c = n = R = 0;

    fill_values(constraint_id, &r, &c, &n, &R);
    
    i = 0;
    if (constraint_id < (CELL_ID + 1) * 81) {
        cell_ids[0] = 9 * (r - 1) + c - 1;
    } else if (constraint_id < (ROW_ID + 1) * 81) {
        for (i = 0; i < 9; i++) /* i = 0-indexed c */
            cell_ids[i] = 9 * (r - 1) + i;
    } else if (constraint_id < (COL_ID + 1) * 81) {
        for (i = 0; i < 9; i++) /* i = 0-indexed r */
            cell_ids[i] = 9 * (i) + c - 1;
    } else { /* (constraint_id < (REGION_ID + 1) * 81) */
        /* reverse formala for R from get_ids, but use 0-indexing */
        r = (R - 1) / 3 * 3;    /* r = 0-indexed version of r */
        c = (R - 1) % 3 * 3;    /* c = 0-indexed version of c */
        for (i = 0; i < 9; i++)
            cell_ids[i] = (r + i / 3) * 9 + c + i % 3;
    }
    return i;
}
