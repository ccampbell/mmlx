#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"

unsigned char auto_inc;
unsigned char auto_tag;
unsigned int  auto_tag_value;


/* ----
 * class1()
 * ----
 * 1 byte, no operand field
 */

void
class1(int *ip)
{
	check_eol(ip);

	/* update location counter */
	loccnt++;

	/* generate code */
	if (pass == LAST_PASS) {
		/* opcode */
		putbyte(data_loccnt, opval);

		/* output line */
		println();
	}
}


/* ----
 * class2()
 * ----
 * 2 bytes, relative addressing
 */

void
class2(int *ip)
{
	unsigned int addr;

	/* update location counter */
	loccnt += 2;

	/* get destination address */
	if (!evaluate(ip, ';'))
		return;

	/* generate code */
	if (pass == LAST_PASS) {
		/* opcode */
		putbyte(data_loccnt, opval);

		/* calculate branch offset */
		addr = value - (loccnt + (page << 13));

		/* check range */
		if (addr > 0x7F && addr < 0xFFFFFF80) {
			error("Branch address out of range!");
			return;
		}

		/* offset */
		putbyte(data_loccnt+1, addr);

		/* output line */
		println();
	}
}


/* ----
 * class3()
 * ----
 * 2 bytes, inherent addressing
 */

void
class3(int *ip)
{
	check_eol(ip);

	/* update location counter */
	loccnt += 2;

	/* generate code */
	if (pass == LAST_PASS) {
		/* opcodes */
		putbyte(data_loccnt, opval);
		putbyte(data_loccnt+1, optype);

		/* output line */
		println();
	}
}


/* ----
 * class4()
 * ----
 * various addressing modes
 */

void
class4(int *ip)
{
	char buffer[32];
	char c;
	int len, mode;
	int	i;

	/* skip spaces */
	while (isspace(prlnbuf[*ip]))
		(*ip)++;

	/* low/high byte prefix string */
	if (isalpha(prlnbuf[*ip])) {
		len = 0;
		i = *ip;

		/* extract string */
		for (;;) {
			c = prlnbuf[i];
			if (c == '\0' || c == ' ' || c == '\t' || c == ';')
				break;
			if ((!isalpha(c) && c != '_') || (len == 31)) {
				len = 0;
				break;
			}
			buffer[len++] = c;
			i++;
		}

		/* check */
		if (len) {
			buffer[len] = '\0';

			if (strcasecmp(buffer, "low_byte") == 0) {
				opext = 'L';
				*ip = i;
			}
			if (strcasecmp(buffer, "high_byte") == 0) {
				opext = 'H';
				*ip = i;
			}
		}
	}

	/* get operand */
	mode = getoperand(ip, opflg, ';');
	if (!mode)
		return;

	/* make opcode */
	if (pass == LAST_PASS) {
		for (i = 0; i < 32; i++) {
			if (mode & (1 << i))
				break;
		}
		opval += opvaltab[optype][i];
	}

	/* auto-tag */
	if (auto_tag) {
		if (pass == LAST_PASS) {
			putbyte(loccnt, 0xA0);
			putbyte(loccnt+1, auto_tag_value);
		}
		loccnt += 2;
	}

	/* generate code */
	switch(mode) {
	case ACC:
		/* one byte */
		if (pass == LAST_PASS)
			putbyte(loccnt, opval);

		loccnt++;
		break;

	case IMM:
	case ZP:
	case ZP_X:
	case ZP_Y:
	case ZP_IND:
	case ZP_IND_X:
	case ZP_IND_Y:
		/* two bytes */
		if (pass == LAST_PASS) {
			putbyte(loccnt, opval);
			putbyte(loccnt+1, value);
		}
		loccnt += 2;
		break;

	case ABS:
	case ABS_X:
	case ABS_Y:
	case ABS_IND:
	case ABS_IND_X:
		/* three bytes */
		if (pass == LAST_PASS) {
			putbyte(loccnt, opval);
			putword(loccnt+1, value);
		}
		loccnt += 3;
		break;
	}

	/* auto-increment */
	if (auto_inc) {
		if (pass == LAST_PASS)
			putbyte(loccnt, auto_inc);

		loccnt += 1;
	}

	/* output line */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * class5()
 * ----
 * 3 bytes, zp/relative addressing
 */

void
class5(int *ip)
{
	int	zp;
	unsigned int addr;
	int mode;

	/* update location counter */
	loccnt += 3;

	/* get first operand */
	mode = getoperand(ip, ZP, ',');
	zp   = value;
	if (!mode)
		return;

	/* get second operand */
	mode = getoperand(ip, ABS, ';');
	if (!mode)
		return;

	/* generate code */
	if (pass == LAST_PASS) {
		/* opcodes */
		putbyte(data_loccnt, opval);
		putbyte(data_loccnt+1, zp);

		/* calculate branch offset */
		addr = value - (loccnt + (page << 13));

		/* check range */
		if (addr > 0x7F && addr < 0xFFFFFF80) {
			error("Branch address out of range!");
			return;
		}

		/* offset */
		putbyte(data_loccnt+2, addr);

		/* output line */
		println();
	}
}


/* ----
 * class6()
 * ----
 * 7 bytes, src/dest/length
 */

void
class6(int *ip)
{
	int	i;
	int addr[3];

	/* update location counter */
	loccnt +=7;

	/* get operands */
    for (i = 0; i < 3; i++) {
		if (!evaluate(ip, (i < 2) ? ',' : ';'))
			return;
		if (pass == LAST_PASS) {
			if (value & 0xFFFF0000) {
				error("Operand size error!");
				return;
			}
		}
	    addr[i] = value;
	}

	/* generate code */
	if (pass == LAST_PASS) {
		/* opcodes */
		putbyte(data_loccnt, opval);
		putword(data_loccnt+1, addr[0]);
		putword(data_loccnt+3, addr[1]);
		putword(data_loccnt+5, addr[2]);

		/* output line */
		println();
	}
}


/* ----
 * class7()
 * ----
 * TST instruction
 */

void
class7(int *ip)
{
	int mode;
	int addr, imm;

	/* get first operand */
	mode = getoperand(ip, IMM, ',');
	imm  = value;
	if (!mode)
		return;

	/* get second operand */
	mode = getoperand(ip, (ZP | ZP_X | ABS | ABS_X), ';');
	addr = value;
	if (!mode)
		return;

	/* make opcode */
	if (mode & (ZP | ZP_X))
		opval = 0x83;
	if (mode & (ABS | ABS_X))
		opval = 0x93;
	if (mode & (ZP_X | ABS_X))
		opval+= 0x20;

	/* generate code */
	if (pass == LAST_PASS) {
		/* opcodes */
		putbyte(loccnt, opval);
		putbyte(loccnt+1, imm);

		if (mode & (ZP | ZP_X))
			/* zero page */
			putbyte(loccnt+2, addr);
		else
			/* absolute */
			putword(loccnt+2, addr);
	}

	/* update location counter */ 
	if (mode & (ZP | ZP_X))
		loccnt += 3;
	else
		loccnt += 4;

	/* auto-increment */
	if (auto_inc) {
		if (pass == LAST_PASS)
			putbyte(loccnt, auto_inc);

		loccnt += 1;
	}

	/* output line */
	if (pass == LAST_PASS)
		println();
}


/* ----
 * class8()
 * ----
 * TAM/TMA instruction
 */

void
class8(int *ip)
{
	int mode;

	/* update location counter */
	loccnt += 2;

	/* get operand */
	mode = getoperand(ip, IMM, ';');
	if (!mode)
		return;

	/* generate code */
	if (pass == LAST_PASS) {
		/* check page index */
		if (value & 0xF8) {
			error("Incorrect page index!");
			return;
		}

		/* opcodes */
		putbyte(data_loccnt, opval);
		putbyte(data_loccnt+1, (1 << value));

		/* output line */
		println();
	}
}


/* ----
 * class9()
 * ----
 * RMB/SMB instructions
 */

void
class9(int *ip)
{
	int bit;
	int mode;

	/* update location counter */
	loccnt += 2;

	/* get the bit index */
	mode = getoperand(ip, IMM, ',');
	bit  = value;
	if (!mode)
		return;

	/* get the zero page address */
	mode = getoperand(ip, ZP, ';');
	if (!mode)
		return;

	/* generate code */
	if (pass == LAST_PASS) {
		/* check bit number */
		if (bit > 7) {
			error("Incorrect bit number!");
			return;
		}

		/* opcodes */
		putbyte(data_loccnt, opval + (bit << 4));
		putbyte(data_loccnt+1, value);

		/* output line */
		println();
	}
}


/* ----
 * class10()
 * ----
 * BBR/BBS instructions
 */

void
class10(int *ip)
{
	int bit;
	int zp;
	int mode;
	unsigned int addr;

	/* update location counter */
	loccnt += 3;

	/* get the bit index */
	mode = getoperand(ip, IMM, ',');
	bit  = value;
	if (!mode)
		return;

	/* get the zero page address */
	mode = getoperand(ip, ZP, ',');
	zp   = value;
	if (!mode)
		return;

	/* get the jump address */
	mode = getoperand(ip, ABS, ';');
	if (!mode)
		return;

	/* generate code */
	if (pass == LAST_PASS) {
		/* check bit number */
		if (bit > 7) {
			error("Incorrect bit number!");
			return;
		}

		/* opcodes */
		putbyte(data_loccnt, opval + (bit << 4));
		putbyte(data_loccnt+1, zp);

		/* calculate branch offset */
		addr = value - (loccnt + (page << 13));

		/* check range */
		if (addr > 0x7F && addr < 0xFFFFFF80) {
			error("Branch address out of range!");
			return;
		}

		/* offset */
		putbyte(data_loccnt+2, addr);

		/* output line */
		println();
	}
}


/* ----
 * getoperand()
 * ----
 */

int
getoperand(int *ip, int flag, int last_char)
{
	unsigned int tmp;
	char c;
	int code;
	int mode;
	int pos;
	int end;

	/* init */
	auto_inc = 0;
	auto_tag = 0;

	/* skip spaces */
	while (isspace(prlnbuf[*ip]))
		(*ip)++;

	/* check addressing mode */
	switch (prlnbuf[*ip]) {
	case '\0':
	case ';':
		/* no operand */
		error("Operand missing!");
		return (0);

	case 'A':
	case 'a':
		/* accumulator */
		c = prlnbuf[(*ip)+1];
		if (isspace(c) || c == '\0' || c == ';' || c == ',') {
			mode = ACC;
			(*ip)++;
			break;
		}

	default:
		/* other */
		switch(prlnbuf[*ip]) {
		case '#':
			/* immediate */
			mode = IMM;
			(*ip)++;
			break;

		case '<':
			/* zero page */
			mode = ZP | ZP_X | ZP_Y;
			(*ip)++;
			break;

		case '[':
			/* indirect */
			mode = ABS_IND | ABS_IND_X | ZP_IND | ZP_IND_X | ZP_IND_Y;
			(*ip)++;
			break;

		default:
			/* absolute */
			mode = ABS | ABS_X | ABS_Y;
			break;
		}

		/* get value */
		if (!evaluate(ip, 0))
			return (0);

		/* check addressing mode */
		code = 0;
		end = 0;
		pos = 0;

		while (!end) {
			c = prlnbuf[*ip];
			if (c == ';' || c == '\0')
				break;
			switch (toupper(c)) {
			case ',':		/* , = 5 */
				if (!pos)
					 pos = *ip;
				else {
					end = 1;
					break;
				}
				code++;
			case '+':		/* + = 4 */
				code++;
			case ']':		/* ] = 3 */
				code++;
				if (prlnbuf[*ip + 1] == '.') {
					end = 1;
					break;
				}
			case 'X':		/* X = 2 */
				code++;
			case 'Y':		/* Y = 1 */
				code++;
				code <<= 4;
			case ' ':
			case '\t':
				(*ip)++;
				break;
			default:
				code = 0xFFFFFF;
				end  = 1;
				break;
			}
		}

		/* absolute, zp, or immediate */
		if (code == 0x000000)
			mode &= (ABS | ZP | IMM);

		/* indirect */
		else if (code == 0x000030)
			mode &= (ZP_IND | ABS_IND);		// ]

		/* indexed modes */
		else if (code == 0x000510)
			mode &= (ABS_Y | ZP_Y);			// ,Y
		else if (code == 0x000520)
			mode &= (ABS_X | ZP_X);			// ,X
		else if (code == 0x005230)
			mode &= (ZP_IND_X | ABS_IND_X);	// ,X]
		else if (code == 0x003510)
			mode &= (ZP_IND_Y);				// ],Y
		else if (code == 0x000001) {
			mode &= (ZP_IND_Y);				// ].tag
		  (*ip) += 2;

			/* get tag */
			tmp = value;

			if (!evaluate(ip, 0))
				return (0);

			/* ok */
			auto_tag = 1;
			auto_tag_value = value;
			value = tmp;
		}
		/* indexed modes with post-increment */
		else if (code == 0x051440) {
			mode &= (ABS_Y | ZP_Y);			// ,Y++
			auto_inc = 0xC8;
		}
		else if (code == 0x052440) {
			mode &= (ABS_X | ZP_X);			// ,X++
			auto_inc = 0xE8;
		}
		else if (code == 0x351440) {
			mode &= (ZP_IND_Y);				// ],Y++
			auto_inc = 0xC8;
		}

		/* absolute, zp, or immediate (or error) */
		else {
			mode &= (ABS | ZP | IMM);
			if (pos)
				*ip = pos;
		}

		/* check value on last pass */
		if (pass == LAST_PASS) {
			/* zp modes */
			if (mode & (ZP | ZP_X | ZP_Y | ZP_IND | ZP_IND_X | ZP_IND_Y) & flag) {
				/* extension stuff */
				if (opext && !auto_inc) {
					if (mode & (ZP_IND | ZP_IND_X | ZP_IND_Y))
						error("Instruction extension not supported in indirect modes!");
					if (opext == 'H')
						value++;
				}
				/* check address validity */
				if ((value & 0xFFFFFF00) && ((value & 0xFFFFFF00) != machine->ram_base))
					error("Incorrect zero page address!");
			}

			/* immediate mode */
			else if (mode & (IMM) & flag) {
				/* extension stuff */
				if (opext == 'L')
					value = (value & 0xFF);
				else if (opext == 'H')
					value = (value & 0xFF00) >> 8;
				else {
					/* check value validity */
					if ((value > 0xFF) && (value < 0xFFFFFF00))
						error("Incorrect immediate value!");
				}
			}

			/* absolute modes */
			else if (mode & (ABS | ABS_X | ABS_Y | ABS_IND | ABS_IND_X) & flag) {
				/* extension stuff */
				if (opext && !auto_inc) {
					if (mode & (ABS_IND | ABS_IND_X))
						error("Instruction extension not supported in indirect modes!");
					if (opext == 'H')
						value++;
				}
				/* check address validity */
				if (value & 0xFFFF0000)
					error("Incorrect absolute address!");
			}
		}
		break;
	}

	/* compare addressing mode */
	mode &= flag;
	if (!mode) {
		error("Incorrect addressing mode!");
		return (0);
	}

	/* skip spaces */
	while (isspace(prlnbuf[*ip]))
		(*ip)++;

	/* get last char */
	c = prlnbuf[*ip];

	/* check if it's what the user asked for */
	switch (last_char) {
	case ';':
		/* last operand */
		if (c != ';' && c != '\0') {
			error("Syntax error!");
			return (0);
		}
		(*ip)++;
		break;		

	case ',':
		/* need more operands */
		if (c != ',') {
			error("Operand missing!");
			return (0);
		}
		(*ip)++;
		break;		
	}

	/* ok */
	return (mode);
}


/* ----
 * getstring()
 * ----
 * get a string from prlnbuf
 */

int
getstring(int *ip, char *buffer, int size)
{
	char c;
	int i;

	/* skip spaces */
	while (isspace(prlnbuf[*ip]))
		(*ip)++;

	/* string must be enclosed */
	if (prlnbuf[(*ip)++] != '\"') {
		error("Incorrect string syntax!");
		return (0);
	}

	/* get string */
	i = 0;
	for (;;) {
		c = prlnbuf[(*ip)++];
		if (c == '\"')
			break;
		if (i >= size) {
			error("String too long!");
			return (0);
		}
		buffer[i++] = c;
	}

	/* end the string */
	buffer[i] = '\0';

	/* skip spaces */
	while (isspace(prlnbuf[*ip]))
		(*ip)++;

	/* ok */
	return (1);
}

