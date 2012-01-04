#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"
#include "nes.h"

/* locals */
static int ines_prg;		/* number of prg banks */
static int ines_chr;		/* number of character banks */
static int ines_mapper[2];	/* rom mapper type */
static struct INES {		/* INES rom header */
	unsigned char id[4];
	unsigned char prg;
	unsigned char chr;
	unsigned char mapper[2];
	unsigned char unused[8];
} header;


/* ----
 * write_header()
 * ----
 * generate and write rom header
 */

void
nes_write_header(FILE *f, int banks)
{
	/* setup INES header */
	memset(&header, 0, sizeof(header));
	header.id[0] = 'N';
	header.id[1] = 'E';
	header.id[2] = 'S';
	header.id[3] = 26;
	header.prg = ines_prg;
	header.chr = ines_chr;
	header.mapper[0] = ines_mapper[0];
	header.mapper[1] = ines_mapper[1];

	/* write */
	fwrite(&header, sizeof(header), 1, f);
}


/* ----
 * pack_8x8_tile()
 * ----
 * encode a 8x8 tile for the NES
 */

int
nes_pack_8x8_tile(unsigned char *buffer, void *data, int line_offset, int format)
{
	int i, j;
	int cnt, err;
	unsigned int   pixel;
	unsigned char *ptr;
	unsigned int  *packed;

	/* pack the tile only in the last pass */
	if (pass != LAST_PASS)
		return (16);

	/* clear buffer */
	memset(buffer, 0, 16);

	/* encode the tile */
	switch (format) {
	case CHUNKY_TILE:
		/* 8-bit chunky format */
		cnt = 0;
		ptr = data;

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				pixel = ptr[j ^ 0x07];
				buffer[cnt]   |= (pixel & 0x01) ? (1 << j) : 0;
				buffer[cnt+8] |= (pixel & 0x02) ? (1 << j) : 0;
			}				
			ptr += line_offset;
			cnt += 1;
		}
		break;

	case PACKED_TILE:
		/* 4-bit packed format */
		cnt = 0;
		err = 0;
		packed = data;
	
		for (i = 0; i < 8; i++) {
			pixel = packed[i];
	
			for (j = 0; j < 8; j++) {
				/* check for errors */
				if (pixel & 0x0C)
					err++;

				/* convert the tile */
				buffer[cnt]   |= (pixel & 0x01) ? (1 << j) : 0;
				buffer[cnt+8] |= (pixel & 0x02) ? (1 << j) : 0;
				pixel >>= 4;
			}
			cnt += 1;
		}

		/* error message */
		if (err)
			error("Incorrect pixel color index!");
		break;

	default:
		/* other formats not supported */
		error("Internal error: unsupported format passed to 'pack_8x8_tile'!");
		break;
	}

	/* ok */
	return (16);
}


/* ----
 * do_defchr()
 * ----
 * .defchr pseudo
 */

void
nes_defchr(int *ip)
{
	unsigned char buffer[16];
	unsigned int data[8];
	int size;
	int i;

	/* define label */
	labldef(loccnt, 1);

	/* output infos */
	data_loccnt = loccnt;
	data_size   = 3;
	data_level  = 3;

	/* get tile data */
	for (i = 0; i < 8; i++) {
		/* get value */
		if (!evaluate(ip, (i < 7) ? ',' : ';'))
			return;

		/* store value */
		data[i] = value;
	}

	/* encode tile */
	size = nes_pack_8x8_tile(buffer, data, 0, PACKED_TILE);

	/* store tile */
	putbuffer(buffer, size);

	/* output line */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * do_inesprg()
 * ----
 * .inesprg pseudo
 */

void
nes_inesprg(int *ip)
{
	if (!evaluate(ip, ';'))
		return;

	if ((value < 0) || (value > 64)) 
	{
		error("Prg bank value out of range!");
	
		return;
	}
	
	ines_prg = value;

	if (pass == LAST_PASS) 
	{
		println();
	}
}


/* ----
 * do_ineschr()
 * ----
 * .ineschr pseudo
 */

void
nes_ineschr(int *ip)
{
	if (!evaluate(ip, ';'))
		return;

	if ((value < 0) || (value > 64)) 
	{
		error("Prg bank value out of range!");
	
		return;
	}
	
	ines_chr = value;

	if (pass == LAST_PASS) 
	{
		println();
	}
}


/* ----
 * do_inesmap()
 * ----
 * .inesmap pseudo
 */

void
nes_inesmap(int *ip)
{
	if (!evaluate(ip, ';'))
		return;

	if ((value < 0) || (value > 255)) 
	{
		error("Mapper value out of range!");
	
		return;
	}
	
	ines_mapper[0] &= 0x0F;
	ines_mapper[0] |= (value & 0x0F) << 4;
	ines_mapper[1]  = (value & 0xF0);

	if (pass == LAST_PASS) 
	{
		println();
	}
}


/* ----
 * do_inesmir()
 * ----
 * .ines.mirror pseudo
 */

void
nes_inesmir(int *ip)
{
	if (!evaluate(ip, ';'))
		return;

	if ((value < 0) || (value > 15)) 
	{
		error("Mirror value out of range!");
	
		return;
	}
	
	ines_mapper[0] &= 0xF0;
	ines_mapper[0] |= (value  & 0x0F);

	if (pass == LAST_PASS) 
	{
		println();
	}
}

