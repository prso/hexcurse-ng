/******************************************************************************\
 *  Copyright (C) 2001 writen by Jewfish and Armoth                           *
 *  Copyright (C) 2020-2022 prso at github, fixes and improvements            *
 *									      *
 *  Description: this codes allows a user to view and edit the hexadecimal and*
 *		 and ascii values of a file.  The curses library is used to   *
 *		 display and manipulate the output.  See the README file      *
 *		 included for more information.				      *
 *									      *
 *  This program is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation; either version 2 of the License, or	      *
 *  (at your option) any later version.					      *
 *									      *
 *  This program is distributed in the hope that it will be useful,	      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of	      *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	      *
 *  GNU General Public License for more details.			      *
 *									      *
 *  You should have received a copy of the GNU General Public License	      *
 *  along with this program; if not, write to the Free Software		      *
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 *									      *
\******************************************************************************/

#include "hex.h"					/* custom header      */

/*#define DEBUG_LLIST*/
/*#define DEBUG_GOTO*/

FILE *fpIN;

int     BASE, MAXY, resize = 0;
int     MIN_ADDR_LENGTH;
hexList *head;						/* linked list struct */
WINS    *windows;					/* window structure   */
char    EBCDIC[256],
	*fpINfilename = NULL,
        *fpOUTfilename = NULL;
bool 	printHex;					/* address format     */
bool    USE_EBCDIC;
bool    TERM_COLORS;
bool    FNUMBERS;
bool    IN_HELP;					/* if help displayed  */
bool    saved = TRUE;
int     hex_win_width,
        ascii_win_width,
        hex_outline_width,
        ascii_outline_width,
        color_level = 2;


    /* partial EBCDIC table contributed by Ted (ted@php.net) */
    char EBCDIC[] = {
    /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D    E   F */
      '.','.','.','.','.','.','.','.','.','.','.','.','.','.' ,'.','.', /* 0 */
      '.','.','.','.','.','.','.','.','.','.','.','.','.','.' ,'.','.', /* 1 */
      '.','.','.','.','.','.','.','.','.','.','.','.','.','.' ,'.','.', /* 2 */
      '.','.','.','.','.','.','.','.','.','.','.','.','.','.' ,'.','.', /* 3 */
      ' ','.','.','.','.','.','.','.','.','.','.','.','<','(' ,'+','|', /* 4 */
      '&','.','.','.','.','.','.','.','.','.','!','$','*',')' ,';','.', /* 5 */
      '-','/','.','.','.','.','.','.','.','.','.',',','%','_' ,'>','?', /* 6 */
      '.','.','.','.','.','.','.','.','.','.',':','#','@','\'','=','"', /* 7 */
      '.','a','b','c','d','e','f','g','h','i','.','.','.','.' ,'.','.', /* 8 */
      '.','.','j','k','l','m','n','o','p','q','.','.','.','.' ,'.','.', /* 9 */
      '.','r','s','t','u','v','w','x','y','z','.','.','.','.' ,'.','.', /* A */
      '.','.','.','.','.','.','.','.','.','`','.','.','.','.' ,'.','.', /* B */
      '.','A','B','C','D','E','F','G','H','I','.','.','.','.' ,'.','.', /* C */
      '.','.','J','K','L','M','N','O','P','Q','.','.','.','.' ,'.','.', /* D */
      '.','R','S','T','U','V','W','X','Y','Z','.','.','.','.' ,'.','.', /* E */
      '0','1','2','3','4','5','6','7','8','9','.','.','.','.' ,'.','.'};/* F */

int ASCII_to_EBCDIC[] =
{
  NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF,
  NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF,
  NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF, NODEF,

  /* 32 */  0x40,  0x5a,  0x7f,  0x7b,  0x5b,  0x6c,  0x50,  0x7d,  0x4d,  0x5d,
  /* 42 */  0x5c,  0x4e,  0x6b,  0x60, NODEF,  0x61,  0xf0,  0xf1,  0xf2,  0xf3,
  /* 52 */  0xf4,  0xf5,  0xf6,  0xf7,  0xf8,  0xf9,  0x7a,  0x5e,  0x4c,  0x7e,
  /* 62 */  0x6e,  0x6f,  0x7c,  0xc1,  0xc2,  0xc3,  0xc4,  0xc5,  0xc6,  0xc7,
  /* 72 */  0xc8,  0xc9,  0xd2,  0xd3,  0xd4,  0xd5,  0xd6,  0xd7,  0xd8,  0xd9,
  /* 82 */  0xe1,  0xe2,  0xe3,  0xe4,  0xe5,  0xe6,  0xe7,  0xe8,  0xe9, NODEF,
  /* 92 */ NODEF, NODEF, NODEF,  0x6d,  0xb9,  0x81,  0x82,  0x83,  0x84,  0x85,
  /* 102 */ 0x86,  0x87,  0x88,  0x89,  0x92,  0x93,  0x94,  0x95,  0x96,  0x97,
  /* 112 */ 0x98,  0x99,  0xa1,  0xa2,  0xa3,  0xa4,  0xa5,  0xa6,  0xa7,  0xa8,
  /* 122 */ 0xa9, NODEF,  0x4f, NODEF, NODEF /* 126 */
};


int main(int argc, char *argv[])			/* main program       */
{

    int   x, retval = 1;			/* counters, etc.     */
    off_t val, len;					/* len need to be off_t*/

    windows = (WINS *) calloc(1, sizeof(WINS));	/* malloc windows     */
    head = llalloc();							/* malloc list space  */
    fpINfilename = NULL;			/* allocate in and    */
    fpOUTfilename = NULL;			/* out file name ptrs */
    printHex = TRUE;							/* address format     */
    USE_EBCDIC = FALSE;							/*use ascii by default*/
    TERM_COLORS = FALSE;                     /*don't use term defined colors by default */
    FNUMBERS = FALSE;     /* keep old behaviour without function key numbers by default */

							/* get cmd line args  */
    len = parseArgs(argc, argv);
    MIN_ADDR_LENGTH = getMinimumAddressLength(len);

    use_env(TRUE);					/* use env values     */
    slk_init(0);					/* init menu bar      */
    init_screen();					/* init visuals       */
    init_colors();

    if ((COLS < MIN_COLS) || (LINES < MIN_LINES))	/* screen's too small */
    {
	endwin();
	fprintf(stderr,"\n\nThe screen size too small.\nThe minimum allowable");
	fprintf(stderr," screen size is %dx%d\n\n", MIN_COLS, MIN_LINES + 1);
	exit(-1);
    }
    
    init_fkeys();					/* define menu bar    */
    

    while (retval)
    {
	free_windows(windows);
        
	/* calculate screen   */
	BASE                = (resize > 0 && resize < COLS) ? resize:((COLS-6-MIN_ADDR_LENGTH)/4);
	MAXY                = (LINES) - 3;
	hex_win_width       = BASE * 3;
	ascii_win_width     = BASE;
	hex_outline_width   = (BASE * 3) + 3 + MIN_ADDR_LENGTH;
	ascii_outline_width = BASE + 2;
            
	init_menu(windows);				/* init windows       */
	head = freeList(head);				/* free & init head   */
							/* print origin loc   */
	mvwprintw(windows->hex_outline, 0, 1, "%0*d", MIN_ADDR_LENGTH, 0);
    
	if (fpIN != NULL)				/* if no infile...    */
	{
            len = maxLoc(fpIN);				/* get last file loc  */
	    val = maxLines(len); 			/* max file lines     */
            for (x = 0; x <= MAXY && x<=val; x++)       /* output lines       */
		outline(fpIN, x);
	}

	wmove(windows->hex, 0, 0);			/* cursor to origin   */
    
	refreshall(windows);				/* refresh all wins   */
	doupdate();					/* update screen      */
        
	mvwaddch(windows->scrollbar, 1, 0, ACS_CKBOARD);/* clear scroller     */
							/* get user input     */
	retval = wacceptch(windows, len); 
    }
    
    free(fpINfilename);
    free(fpOUTfilename);
    freeList(head);
    
    screen_exit(0);					/* end visualizations */
    return retval;					/* return             */
}

/********************************************************\
 * Description: prints out debug info to a file         *
 * Returns:     nothing                                 *
\********************************************************/
/*
void printDebug(hexList *head, long int loc)
{
    FILE *tmpofp;
    hexList *tmpHead = head;

    tmpofp = fopen("debug_llist", "a+");
    tmpHead = head;

    fprintf(tmpofp, "location undone: %08X\n", loc);
    while (tmpHead != NULL)
    {
	fprintf(tmpofp, "head->loc: %08X   head->val: %02X (%c)\n", tmpHead->loc, tmpHead->val, tmpHead->val);

	tmpHead = tmpHead->next;
    }
    fprintf(tmpofp, "\n");

    fclose(tmpofp);
}
*/

/********************************************************\
 * Description: parses command line arguments and 	*
 *		processes them.				*
 * Returns:	length of file				*
\********************************************************/
off_t parseArgs(int argc, char *argv[])
{
    extern char *optarg;				/* extern vars for    */
    extern int optind, /*opterr,*/ optopt;		/* getopt()	      */

    int val;						/* counters, etc.     */

							/* get args           */
    while ((val = hgetopt(argc, argv, "c:ai:o:r:etf?h")) != -1)
    {
	switch (val)					/* test args          */
        {
            case 'c':   color_level = atoi(optarg);
                        if (color_level<0 || color_level>3)
                        {
                            print_usage();
                            exit(-1);
                        }
                        break;

            case 'a':	printHex = FALSE;		/* decimal addresses  */
                        break;
							/* infile             */
            case 'i':	free(fpINfilename);
                        fpINfilename = strdup(optarg);
                        break;
							/* outfile            */
            case 'o':   free(fpOUTfilename);
                        fpOUTfilename = strdup(optarg);
                        break;

            case 'r':   resize = atoi(optarg);		/* don't resize screen*/
                        break;

            case 'e':   USE_EBCDIC=TRUE;		/*use instead of ascii*/
                        break;

            case 't':   TERM_COLORS=TRUE;       /* keep term defined colors */
                        break;                  /* if defined and not set may look bad */

            case 'f':   FNUMBERS=TRUE;          /* show numbers in labels for function keys */
                        break;
							/* help/invalid args  */
            case '?':
            case 'h':	print_usage();			/* output help        */
                        if ((optopt == 'h') || (optopt == '?'))
                           exit(0);			/* exit               */
                        else				/* illegal option     */
                           exit(-1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argv[0])
    {
	free(fpINfilename);
        fpINfilename = strdup(argv[0]);
    }

    if (fpINfilename == NULL) {
        print_usage();
        exit(-1);
    } else if (fpINfilename && strcmp(fpINfilename, "")) {
        if ((fpIN = fopen(fpINfilename, "r")) == NULL)
            exit_err("Could not open file");
    }

    return ((fpIN != NULL) ? maxLoc(fpIN):0);		/* return file length */
}

/********************************************************\
 * Description: Get the minimum address length for the  *
 *              address column                          *
 * Returns:	minimum length for addresses		*
\********************************************************/
int getMinimumAddressLength(off_t len)
{
        int min_address_length;
        
        min_address_length = snprintf(NULL, 0, "%jd", (intmax_t)len);
        
        /* At least 8 characters wide */
        return min_address_length > 8 ? min_address_length : 8;
}

/********************************************************\
 * Description: in the event of a segmentation fault    *
 * 		this catches the signal and prints out  *
 *		instructions on where to send a bug     *
 * 		report.					*
 * Returns:	length of file				*
\********************************************************/
RETSIGTYPE catchSegfault(int sig)
{
    /* Avoid unused variable warning */
    UNUSED(sig);
    
    endwin();
    printf("\n\nHexcurse has encountered a segmentation fault!\n");
    printf("\tPlease submit a full bug report to devel@jewfish.net.\n");
    printf("\tInclude what you did to cause the segfault, and if possible\n");
    printf("\tinclude the core dump.  And for your troubles, we'll add you \n");
    printf("\tto the Changelog. Then you can brag to your friends about it!\n");

    exit(-1);
}
