#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"

int  mopt;
int  in_macro;
int  expand_macro;
char marg[8][10][80];
int  midx;
int  mcounter, mcntmax;
int  mcntstack[8];
struct t_line  *mstack[8];
struct t_line  *mlptr;
struct t_macro *macro_tbl[256];
struct t_macro *mptr;

/* .macro pseudo */

void
do_macro(int *ip)
{
	if (pass == LAST_PASS)
		println();
	else {
		/* error checking */
		if (expand_macro) {
			error("Can not nest macro definitions!");
			return;
		}
		if (lablptr == NULL) {
			/* skip spaces */
			while (isspace(prlnbuf[*ip]))
				(*ip)++;

			/* search a label after the .macro */
			if (colsym(ip) == 0) {
				error("No name for this macro!");
				return;
			}

			/* put the macro name in the symbol table */
			if ((lablptr = stlook(1)) == NULL)
				return;
		}
		if (lablptr->refcnt) {
			switch (lablptr->type) {
			case MACRO:
				fatal_error("Macro already defined!");
				return;

			case FUNC:
				fatal_error("Symbol already used by a function!");
				return;

			default:
				fatal_error("Symbol already used by a label!");
				return;
			}
		}
		if (!check_eol(ip))
			return;

		/* install this new macro in the hash table */
		if (!macro_install())
			return;
	} 
	in_macro = 1;
}

/* .endm pseudo */

void
do_endm(int *ip)
{
	error("Unexpected ENDM!");
	return;
}

/* search a macro in the hash table */

struct t_macro *macro_look(int *ip)
{
	struct t_macro *ptr;
	char name[32];
	char c;
	int  hash;
	int  l;

	/* calculate the symbol hash value and check syntax */
	l = 0;
	hash = 0;
	for (;;) {
		c = prlnbuf[*ip];
		if (c == '\0' || c == ' ' || c == '\t' || c == ';')
			break;
		if (!isalnum(c) && c != '_')
			return (NULL);
		if (l == 0) {
			if (isdigit(c))
				return (NULL);
		}
		if (l == 31)
			return (NULL);
		name[l++] = c;
		hash += c;
		hash  = (hash << 3) + (hash >> 5) + c;
		(*ip)++;
	}
	name[l] = '\0';
	hash &= 0xFF;

	/* browse the hash table */
	ptr = macro_tbl[hash];
	while (ptr) {
		if (!strcmp(name, ptr->name))
			break;			
		ptr = ptr->next;
	}

	/* return result */
	return (ptr);
}

/* extract macro arguments */

int
macro_getargs(int ip)
{
	char *ptr;
	char  c, t;
	int   i, j, f, arg;
	int   level;

	/* can not nest too much macros */
	if (midx == 7) {
		error("Too many nested macro calls!");
		return (0);
	}

	/* initialize args */
	mcntstack[midx] = mcounter;
	mstack[midx++] = mlptr;
	ptr = marg[midx][0];
	arg = 0;

	for (i = 0; i < 9; i++)
		marg[midx][i][0] = '\0';

	/* extract args */
	for (;;) {
		/* skip spaces */
		while (isspace(prlnbuf[ip]))
			ip++;

		c = prlnbuf[ip++];
		switch (c) {
		/* no arg */
		case ',':
			arg++;
			ptr = marg[midx][arg];
			if (arg == 9) {
				error("Too many arguments for a macro!");
				return (0);
			}
			break;

		/* string */			
		case '{':
			c = '}';
		case '\"':
			i = 0;
			if (c == '\"')
				ptr[i++] = c;
			for (;;) {
				t = prlnbuf[ip++];
				if (t == '\0') {
					error("Unterminated string!");
					return (0);
				}
				if (i == 80) {
					error("String too long, max. 80 characters!");
					return (0);
				}
				if (t == c)
					break;
				ptr[i++] = t;
			}
			if (c == '\"')
				ptr[i++] = t;

			/* skip spaces */
			while (isspace(prlnbuf[ip]))
				ip++;

			/* check end of arg */
			switch (prlnbuf[ip]) {
			case '\0':
			case ',':
			case ';':
				break;

			default:
				error("Syntax error!");
				return (0);
			}

			/* end arg string */
			ptr[i] = '\0';
			break;

		/* end of line */	
		case ';':
		case '\0':
			return (1);

		/* continuation char */
		case '\\':
			/* skip spaces */
			i = ip;
			while (isspace(prlnbuf[i]))
				i++;

			/* check */
			if (prlnbuf[i] == ';' || prlnbuf[i] == '\0') {
				/* output line */
				if (pass == LAST_PASS) {
					println();
					clearln();
				}

				/* read a new line */
				if (readline() == -1)
					return (0);
				
				/* rewind line pointer and continue */
				ip = SFIELD;
				break;
			}

		/* other */
		default:
			i = 0;
			j = 0;
			f = 0;
			level = 0;
			while (c) {
				if (c == ',') {
					if (level == 0)
						break;
				}
				else if ((c == '(') || (c == '[')) {
					level++;
				}
				else if ((c == ')') || (c == ']')) {
					if (level)
						level--;
				}
				else if (c == ';') {
					break;
				}
				if (f) {
					if (c != ' ') {
						while (i < j)
							ptr[i++] = ' ';
						ptr[i++] = c;
						f = 0;
					}
				}
				else if (c == ' ') {
					f = 1;
				}
				else {
					ptr[i++] = c;
				}
				if (i == 80) {
					error("Macro argument string too long, max. 80 characters!");
					return (0);
				}
				j++;
				c = prlnbuf[ip++];
			}
			ptr[i] = '\0';
			ip--;

			/* check if arg is X or Y */
			if (strlen(ptr) && arg) {
				c = tolower(ptr[0]);

				if ((c == 'x') || (c == 'y')) {
					if ((strcasecmp(ptr, "x++") == 0) ||
						(strcasecmp(ptr, "y++") == 0) ||
						(strlen(ptr) == 1))
					{
						arg--;
						ptr = marg[midx][arg];
	
						/* check string length */
						if (strlen(ptr) > 75) {
							error("Macro argument string too long, max. 80 characters!");
							return (0);
						}
	
						/* attach current arg to the previous one */
						strcat(ptr, ",");
						strcat(ptr, marg[midx][arg + 1]);
						ptr = marg[midx][arg + 1];
						ptr[0] = '\0';
					}
			 	}
			}
			break;
		}
	}
}

/* install a macro in the hash table */

int
macro_install(void)
{
	char c;
	int hash = 0;
	int i;

	/* mark the macro name as reserved */
	lablptr->type = MACRO;

	/* check macro name syntax */
	if (strchr(&symbol[1], '.')) {
		error("Invalid macro name!");
		return (0);
	}

	/* calculate symbol hash value */
	for (i = 1; i <= symbol[0]; i++) {
		c = symbol[i];
		hash += c;
		hash  = (hash << 3) + (hash >> 5) + c;
	}
	hash &= 0xFF;

	/* allocate a macro struct */
	mptr = (void *)malloc(sizeof(struct t_macro));
	if (mptr == NULL) {
		error("Out of memory!");
		return (0);
	}

	/* initialize it */
	strcpy(mptr->name, &symbol[1]);
	mptr->line = NULL;
	mptr->next = macro_tbl[hash];
	macro_tbl[hash] = mptr;
	mlptr = NULL;

	/* ok */
	return (1);
}

/* send back the addressing mode of a macro arg */

int
macro_getargtype(char *arg)
{
	struct t_symbol *sym;
	char c;
	int  i;

	/* skip spaces */
	while (isspace(*arg))
		arg++;

	/* get type */
	switch (toupper(*arg++)) {
	case '\0':
		return (NO_ARG);

	case '"':
		return (ARG_STRING);

	case '#':
		return (ARG_IMM);

	case '[':
		return (ARG_INDIRECT);

	case 'A':
	case 'X':
	case 'Y':
		if (*arg == '\0')
			return (ARG_REG);

	default:
		/* symbol */
		for(i = 0; i < SBOLSZ; i++) {
			c = arg[i];
			if (isdigit(c) && (i == 0))
				break;
			if ((!isalnum(c)) && (c != '_') && (c != '.'))
				break;
		}

		if (i == 0)
			return (ARG_ABS);
		else {
			if (c != '\0')
				return (ARG_ABS);
			else {
				strncpy(&symbol[1], arg, i);
				symbol[0] = i;
				symbol[i+1] = '\0';

				if ((sym = stlook(0)) == NULL)
					return (ARG_LABEL);
				else {
					if((sym->type == UNDEF) || (sym->type == IFUNDEF))
						return (ARG_LABEL);
					if (sym->bank == RESERVED_BANK)
						return (ARG_ABS);
					else
						return (ARG_LABEL);
				}
			}
		}
	}
}

