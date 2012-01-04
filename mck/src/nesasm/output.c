#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"


/* ----
 * println()
 * ----
 * prints the contents of prlnbuf
 */

void
println(void)
{
	int nb, cnt;
	int i;

	/* check if output possible */
	if (list_level == 0)
		return;
	if (!xlist || !asm_opt[OPT_LIST] || (expand_macro && !asm_opt[OPT_MACRO]))
		return;

	/* update line buffer if necessary */
	if (continued_line)
		strcpy(prlnbuf, tmplnbuf);

	/* output */
	if (data_loccnt == -1)
		/* line buffer */
		fprintf(lst_fp, "%s\n", prlnbuf);
	else {
		/* line buffer + data bytes */
		loadlc(data_loccnt, 0);

		/* number of bytes */
		nb = loccnt - data_loccnt;

		/* check level */
		if ((data_level > list_level) && (nb > 3))
			/* doesn't match */
			fprintf(lst_fp, "%s\n", prlnbuf);
		else {
			/* ok */
			cnt = 0;
			for (i = 0; i < nb; i++) {
				if (bank >= RESERVED_BANK) {
					prlnbuf[16 + (3*cnt)] = '-';
					prlnbuf[17 + (3*cnt)] = '-';
				}
				else {
					hexcon(2, rom[bank][data_loccnt]);
					prlnbuf[16 + (3*cnt)] = hex[1];
					prlnbuf[17 + (3*cnt)] = hex[2];
				}
				data_loccnt++;
				cnt++;
				if (cnt == data_size) {
					cnt = 0;
					fprintf(lst_fp, "%s\n", prlnbuf);
					clearln();
					loadlc(data_loccnt, 0);
				}
			}
			if (cnt)
				fprintf(lst_fp, "%s\n", prlnbuf);
		}
	}
}


/* ----
 * clearln()
 * ----
 * clear prlnbuf
 */

void
clearln(void)
{
	int i;

	for (i = 0; i < SFIELD; i++)
		prlnbuf[i] = ' ';
	prlnbuf[i] = 0;
}


/* ----
 * loadlc()
 * ----
 * load 16 bit value in printable form into prlnbuf
 */

void
loadlc(int offset, int pos)
{
	int	i;

	if (pos)
		i = 16;
	else
		i = 7;

	if (pos == 0) {
		if (bank >= RESERVED_BANK) {
			prlnbuf[i++] = '-';
			prlnbuf[i++] = '-';
		}
		else {
			hexcon(2, bank);
			prlnbuf[i++] = hex[1];
			prlnbuf[i++] = hex[2];
		}
		prlnbuf[i++] = ':';
		offset += page << 13;
	}
	hexcon(4, offset);
	prlnbuf[i++] = hex[1];
	prlnbuf[i++] = hex[2];
	prlnbuf[i++] = hex[3];
	prlnbuf[i]   = hex[4];
}


/* ----
 * hexcon()
 * ----
 * convert number supplied as argument to hexadecimal in hex[digit]
 */

void
hexcon(int digit, int num)
{
	for (; digit > 0; digit--) {
		hex[digit] = (num & 0x0f) + '0';
		if (hex[digit] > '9')
			hex[digit] += 'A' - '9' - 1;
		num >>= 4;
	}
}


/* ----
 * putbyte()
 * ----
 * store a byte in the rom
 */

void
putbyte(int offset, int data)
{
	if (bank >= RESERVED_BANK)
		return;
	if (offset < 0x2000) {
		rom[bank][offset] = (data) & 0xFF;
		map[bank][offset] = section + (page << 5);

		/* update rom size */
		if (bank > max_bank)
			max_bank = bank;
	}
}


/* ----
 * putword()
 * ----
 * store a word in the rom
 */

void
putword(int offset, int data)
{
	if (bank >= RESERVED_BANK)
		return;
	if (offset < 0x1FFF) {
		/* low byte */
		rom[bank][offset] = (data) & 0xFF;
		map[bank][offset] = section + (page << 5);

		/* high byte */
		rom[bank][offset+1] = (data >> 8) & 0xFF;
		map[bank][offset+1] = section + (page << 5);

		/* update rom size */
		if (bank > max_bank)
			max_bank = bank;
	}
}


/* ----
 * putbuffer()
 * ----
 * copy a buffer at the current location
 */

void
putbuffer(void *data, int size)
{
	int addr;

	/* check size */
	if (size == 0)
		return;

	/* check if the buffer will fit in the rom */
	if (bank >= RESERVED_BANK) {
		addr  = loccnt + size;

		if (addr > 0x1FFF) {
			fatal_error("PROC overflow!");
			return;
		}
	}
	else {
		addr  = loccnt + size + (bank << 13);

		if (addr > rom_limit) {
			fatal_error("ROM overflow!");
			return;
		}

		/* copy the buffer */
		if (pass == LAST_PASS) {
			if (data) {
				memcpy(&rom[bank][loccnt], data, size);
				memset(&map[bank][loccnt], section + (page << 5), size);
			}
			else {
				memset(&rom[bank][loccnt], 0, size);
				memset(&map[bank][loccnt], section + (page << 5), size);
			}
		}
	}

	/* update the location counter */
	bank  += (loccnt + size) >> 13;
	loccnt = (loccnt + size) & 0x1FFF;

	/* update rom size */
	if (bank < RESERVED_BANK) {
		if (bank > max_bank) {
			if (loccnt)
				max_bank = bank;
			else
				max_bank = bank - 1;
		}
	}
}


/* ----
 * write_srec()
 * ----
 */

void
write_srec(char *file, char *ext, int base)
{
	unsigned char data, chksum;
	char  fname[128];
	int   addr, dump, cnt, pos, i, j;
	FILE *fp;

	/* status message */
	if (!strcmp(ext, "mx"))
		printf("writing mx file... ");
	else
		printf("writing s-record file... ");

	/* flush output */
	fflush(stdout);

	/* add the file extension */
	strcpy(fname, file);
	strcat(fname, ".");
	strcat(fname, ext);

	/* open the file */
	if ((fp = fopen(fname, "w")) == NULL) {
		printf("can not open file '%s'!\n", fname);
		return;
	}

	/* dump the rom */
	dump = 0;
	cnt  = 0;
	pos  = 0;

	for (i = 0; i <= max_bank; i++) {
		for (j = 0; j < 8192; j++) {
			if (map[i][j] != 0xFF) {
				/* data byte */
				if (cnt == 0)
					pos = j;
				cnt++;
				if (cnt == 32)
					dump = 1;
			}
			else {
				/* free byte */
				if (cnt)
					dump = 1;
			}
			if (j == 8191)
				if (cnt)
					dump = 1;

			/* dump */
			if (dump) {
				dump = 0;
				addr = base + (i << 13) + pos;
				chksum = cnt + ((addr >> 16) & 0xFF) +
							   ((addr >> 8) & 0xFF) +
							   ((addr) & 0xFF) +
							   4;

				/* number, address */
				fprintf(fp, "S2%02X%06X", cnt + 4, addr);

				/* code */
				while (cnt) {
					data = rom[i][pos++];
					chksum += data;
					fprintf(fp, "%02X", data);
					cnt--;
				}

				/* chksum */
				fprintf(fp, "%02X\n", (~chksum) & 0xFF);
			}
		}
	}

	/* starting address */
	addr   = ((map[0][0] >> 5) << 13);
	chksum = ((addr >> 8) & 0xFF) + (addr & 0xFF) + 4;
	fprintf(fp, "S804%06X%02X", addr, (~chksum) & 0xFF);

	/* ok */
	fclose(fp);
	printf("OK\n");
}


/* ----
 * fatal_error()
 * ----
 * stop compilation
 */

void
fatal_error(char *stptr)
{
	error(stptr);
	stop_pass = 1;
}


/* ----
 * error()
 * ----
 * error printing routine
 */

void
error(char *stptr)
{
	warning(stptr);
	errcnt++;
}


/* ----
 * warning()
 * ----
 * warning printing routine
 */

void
warning(char *stptr)
{
	int i, temp;

	/* put the source line number into prlnbuf */
	i = 4;
	temp = slnum;
	while (temp != 0) {
		prlnbuf[i--] = temp % 10 + '0';
		temp /= 10;
	}

	/* update the current file name */
	if (infile_error != infile_num) {
		infile_error  = infile_num;
		printf("#[%i]   %s\n", infile_num, input_file[infile_num].name);
	}

	/* output the line and the error message */
	loadlc(loccnt, 0);
	printf("%s\n", prlnbuf);
	printf("       %s\n", stptr);
}

