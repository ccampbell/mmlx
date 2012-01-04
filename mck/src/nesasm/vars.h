unsigned char rom[128][8192];
unsigned char map[128][8192];
char bank_name[128][64];
int  bank_loccnt[4][256];
int  bank_page[4][256];
int max_zp;		/* higher used address in zero page */
int max_bss;	/* higher used address in ram */
int max_bank;	/* last bank used */
int data_loccnt;	/* data location counter */
int data_size;		/* size of binary output (in bytes) */
int data_level;		/* data output level, must be <= listlevel to be outputed */
int loccnt;	/* location counter */
int bank;	/* current bank */
int bank_base;	/* bank base index */
int rom_limit;	/* bank limit */
int bank_limit;	/* rom max. size in bytes */
int page;	/* page */
int rsbase;	/* .rs counter */
int section;	/* current section: S_ZP, S_BSS, S_CODE or S_DATA */
int section_bank[4];	/* current bank for each section */
int stop_pass;		/* stop the program; set by fatal_error() */
int errcnt;			/* error counter */
struct t_machine *machine;
struct t_opcode  *inst_tbl[256];	/* instructions hash table */
struct t_symbol  *hash_tbl[256];	/* label hash table */
struct t_symbol  *lablptr;	/* label pointer into symbol table */
struct t_symbol  *glablptr;	/* pointer to the latest defined global label */
struct t_symbol  *lastlabl;	/* last label we have seen */
struct t_symbol  *bank_glabl[4][256];	/* latest global symbol for each bank */
char hex[5];			/* hexadecimal character buffer */
void (*opproc)(int *);	/* instruction gen proc */
int  opflg;		/* instruction flags */
int  opval;		/* instruction value */
int  optype;	/* instruction type */
char opext;		/* instruction extension (.l or .h) */
int  pass;		/* pass counter */
char prlnbuf[LAST_CH_POS+4];	/* input line buffer */
char tmplnbuf[LAST_CH_POS+4];	/* temporary line buffer */
int  slnum;				/* source line number counter */
char symbol[SBOLSZ+1];	/* temporary symbol storage */
int undef;				/* undefined symbol in expression flg  */
unsigned int value;		/* operand field value */
int  opvaltab[6][16] = {
   {0x08, 0x08, 0x04, 0x14, 0x14, 0x11, 0x00, 0x10,  // CPX CPY LDX LDY
	0x0C, 0x1C, 0x18, 0x2C, 0x3C, 0x00, 0x00, 0x00},
   {0x00, 0x00, 0x04, 0x14, 0x14, 0x00, 0x00, 0x00,  // ST0 ST1 ST2 TAM TMA
	0x0C, 0x1C, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00},
   {0x00, 0x89, 0x24, 0x34, 0x00, 0x00, 0x00, 0x00,  // BIT
	0x2C, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {0x3A, 0x00, 0xC6, 0xD6, 0x00, 0x00, 0x00, 0x00,  // DEC
	0xCE, 0xDE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {0x1A, 0x00, 0xE6, 0xF6, 0x00, 0x00, 0x00, 0x00,  // INC
	0xEE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {0x00, 0x00, 0x64, 0x74, 0x00, 0x00, 0x00, 0x00,  // STZ
	0x9C, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

