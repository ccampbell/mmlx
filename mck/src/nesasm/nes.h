
/* NES.C */
void nes_write_header(FILE *f, int banks);
int  nes_pack_8x8_tile(unsigned char *buffer, void *data, int line_offset, int format);
void nes_defchr(int *ip);
void nes_inesprg(int *ip);
void nes_ineschr(int *ip);
void nes_inesmap(int *ip);
void nes_inesmir(int *ip);

/* NES specific pseudos */
struct t_opcode nes_pseudo[11] = {
	{NULL,  "DEFCHR",  nes_defchr,  PSEUDO, P_DEFCHR,  0},
	{NULL,  "INESPRG", nes_inesprg, PSEUDO, P_INESPRG, 0},
	{NULL,  "INESCHR", nes_ineschr, PSEUDO, P_INESCHR, 0},
	{NULL,  "INESMAP", nes_inesmap, PSEUDO, P_INESMAP, 0},
	{NULL,  "INESMIR", nes_inesmir, PSEUDO, P_INESMIR, 0},

	{NULL, ".DEFCHR",  nes_defchr,  PSEUDO, P_DEFCHR,  0},
	{NULL, ".INESPRG", nes_inesprg, PSEUDO, P_INESPRG, 0},
	{NULL, ".INESCHR", nes_ineschr, PSEUDO, P_INESCHR, 0},
	{NULL, ".INESMAP", nes_inesmap, PSEUDO, P_INESMAP, 0},
	{NULL, ".INESMIR", nes_inesmir, PSEUDO, P_INESMIR, 0},
	{NULL, NULL, NULL, 0, 0, 0}
};

/* NES machine description */
struct t_machine nes = {
	MACHINE_NES,   /* type */
	"NESASM", /* asm_name */
	"NES Assembler (v2.51)", /* asm_title */
	".nes",  /* rom_ext */
	"NES_INCLUDE", /* include_env */
	0x100,  /* zp_limit */
	0x800,  /* ram_limit */
	0,      /* ram_base */
	0,      /* ram_page */
	RESERVED_BANK, /* ram_bank */
	NULL,       /* inst */
	nes_pseudo, /* pseudo_inst */
	nes_pack_8x8_tile, /* pack_8x8_tile */
	NULL,              /* pack_16x16_tile */
	NULL,              /* pack_16x16_sprite */
	nes_write_header   /* write_header */
};

