/******************************************************************************\
 *  Copyright (C) 2001, hexcurse is written by Jewfish and Armoth             *
 *  Copyright (C) 2020,2021 prso at github, fixes and improvements            *
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
#include "hex.h"

/********************************************************\
 * Description: deletes an item from the linked list and*
 *	 	returns the head of the linked list.    *
\********************************************************/	
hexList *deleteNode(hexList *head, off_t loc)
{
    if (head == NULL)					/* if NULL            */
        return head;					/* just return        */
    else if (head->loc > loc)
	return head;
    else if (head->loc < loc) 				/* if loc > current   */
        head->next = deleteNode(head->next, loc);	/* go to next list    */
    else if (head->loc == loc)				/* if correct loc     */
    {
        hexList *tmpHead = head;
        head = head->next;				/* "delete" it        */
        free(tmpHead);					/* free the memory    */
    }

        /*printDebug(head, loc);*/

    return head;
}

/********************************************************\
 * Description: inserts an item into the linked list and*
 *	 	returns the head of the linked list.    *
\********************************************************/	
hexList *insertItem(hexList *head, off_t loc, int val)
{
    hexList *curr = head,
            *prev = NULL,
            *newHead = head,
            *newItem;

    while (curr != NULL && loc > curr->loc)  /* iterate until correct */
    {                                        /* position for loc      */
        prev = curr;                         /* or end of list        */
        curr = curr->next;
    }
    newItem  = llalloc();                  /* allocate space        */
    if (prev == NULL) newHead = newItem;   /* new head of list      */
    else prev->next = newItem;             /* or point to previous  */
    newItem->loc = loc;                    /* store the location    */
    newItem->val = val;                    /* store the value       */
    newItem->next = curr;                  /* point next to current */
    return newHead;
}

/********************************************************\
 * Description: Search through the linked list for to   *
 *		check if loc exist in the list. The     *
 *		function returns -1 if not found or a   *
 *		positve int of the modified value       * 
\********************************************************/
int searchList(hexList *head, off_t loc)
{   
    hexList *tmpHead;					/* allocate temp space*/
    tmpHead = head;					/* temp points to head*/
    while (tmpHead != NULL)				/* while not null     */
    {
        if (tmpHead->loc == loc)			/*if loc == return val*/
			return tmpHead->val;
		else
			tmpHead = tmpHead->next;	/* move to next item  */
    }
    return -1;
}

/********************************************************\
 * Description: Count the number of items for a given   *
 *      loc in the list and return it                   *
\********************************************************/
off_t countList(hexList *head, off_t loc)
{
    hexList *curr = head;
    off_t count = 0;

    while (curr != NULL && curr->loc < loc) curr = curr->next;
    while (curr != NULL && curr->loc == loc)
    {
        count++;
        curr = curr->next;
    }
    return count;
}

/********************************************************\
 * Description: Search through the linked list to     *
 *		find if there are values for locations        *
 *		between pos1 and pos2. If there are, changes  *
 *		the corresponding values inside the buffer    *
\********************************************************/
void updateBuf(hexList *head, char *buf, off_t pos1, off_t pos2)
{
    hexList *tmpHead;					/* allocate temp space*/
    off_t   prev_loc = -1;

    tmpHead = head;					/* temp points to head*/
    while (tmpHead != NULL)				/* while not null     */
    {
        if (prev_loc != tmpHead->loc && tmpHead->loc >= pos1 && tmpHead->loc < pos2)
        {
            buf[tmpHead->loc - pos1] = (unsigned char) tmpHead->val;
        }
        prev_loc = tmpHead->loc;
        tmpHead = tmpHead->next;	/* move to next item  */
    }
}

/********************************************************\
 * Description: write the changes to either the current *
 *		file or to a specified output file	*
\*******************************************************/
int writeChanges()
{   
    FILE *fpOUT = NULL;
    FILE *fptmp = NULL;
    
    off_t prev_loc;					/* declare llist vars */
    hexList *tmpHead = head;
    int nread, fs, fp, errfile = 0;
    char *buff;
    bool errfpOUT = FALSE;

    if (fpOUTfilename && fpIN)
    {							/* open the write file*/
	if ( (fpOUT = fopen(fpOUTfilename, "w+")) )
	{
		fptmp = fpOUT;
		rewind(fpIN);				/* set file loc to 0  */
		buff = malloc(FILEBUFF);		/* allocate buffer */
		while (!feof(fpIN))
		{			  /* copy original file to new one */
			nread = fread(buff, 1, FILEBUFF, fpIN);
			fwrite(buff, 1, nread, fpOUT);
			if (ferror(fpOUT) || ferror(fpIN))
			{
				fclose(fpOUT);
				remove(fpOUTfilename);
				errfpOUT = TRUE;
				break;
			}
		}
		free(buff);
		rewind(fptmp);
		fclose(fpIN);
		free(fpINfilename);
		fpINfilename = strdup(fpOUTfilename);
		free(fpOUTfilename);
		fpOUTfilename = NULL;
	}
	else errfpOUT = TRUE;
	if (errfpOUT)
	{
		popupWin("Error writing to file", -1);
		return 1;
	}
    }
    else if (fpIN)				/* if no output file  */
    {
	fptmp = fopen(fpINfilename, "r+");
	if (!fptmp)
	{
		popupWin("Cannot write to file: bad permissions", -1);
		return 1;
	}
	fclose(fpIN);
    }
    else
    {
	popupWin("No data written.", -1);
	return 1;
    }
    
    prev_loc = -1;
    while (tmpHead != NULL)		/* write to file      */
    {
	/* only print the latest change  from the linked list*/
	if (prev_loc != tmpHead->loc) { 
		fs = fseeko(fptmp, tmpHead->loc, SEEK_SET);
		fp = fputc(tmpHead->val, fptmp);
		if (fs != 0 || fp == EOF)
		{
			popupWin("Error writing to file", -1);
			errfile = 1;
			break;
		}
	}
	prev_loc = tmpHead->loc;
	tmpHead = tmpHead->next;
    }
    if ( (fpIN = fopen(fpINfilename, "r")) )   /* reopen file readonly */
	fclose(fptmp);
    else
    {
	fpIN = fptmp;       /* if not possible, keep it readwrite */
	fflush(fpIN);
	rewind(fpIN);
    }
    return errfile;
} 

/********************************************************\
 * Description: recursivly frees all the memory that was*
 *		allocated via malloc(), this avoids     *
 *		memory leaks that exist far too many    *
 *		programs that you have to pay to use    *
\********************************************************/
hexList *freeList(hexList *head)
{
    if (head != NULL) 						/* while head != NULL */
    {
        freeList(head->next);				/* check next item    */
        free(head);							/* free the memory    */
    }
    return NULL;							/* return NULL to head*/
} 
