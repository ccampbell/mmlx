
/* path separator */
#if defined(DJGPP) || defined(MSDOS) || defined(WIN32)
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STRING "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STRING "/"
#endif

/* machine */
#define MACHINE_PCE	0
#define MACHINE_NES	1

/* reserved bank index */
#define RESERVED_BANK	0xF0
#define PROC_BANK		0xF1
#define GROUP_BANK		0xF2

/* tile format for encoder */
#define CHUNKY_TILE		1
#define PACKED_TILE		2

/* line buffer length */
#define LAST_CH_POS	158
#define SFIELD	26
#define SBOLSZ	32

/* macro argument types */
#define NO_ARG			0
#define ARG_REG			1
#define ARG_IMM			2
#define ARG_ABS			3
#define ARG_INDIRECT	4
#define ARG_STRING		5
#define ARG_LABEL		6

/* section types */
#define S_ZP	0
#define S_BSS	1
#define S_CODE	2
#define S_DATA	3

/* assembler options */
#define OPT_LIST	 0
#define OPT_MACRO	 1
#define OPT_WARNING	 2
#define OPT_OPTIMIZE 3

/* assembler directives */
#define P_DB		 0	// .db
#define P_DW		 1	// .dw
#define P_DS		 2	// .ds
#define P_EQU		 3	// .equ
#define P_ORG		 4	// .org
#define P_PAGE		 5	// .page
#define P_BANK		 6	// .bank
#define P_INCBIN	 7	// .incbin
#define P_INCLUDE	 8	// .include
#define P_INCCHR	 9	// .incchr
#define P_INCSPR	10	// .incspr
#define P_INCPAL	11	// .incpal
#define P_INCBAT	12	// .incbat
#define P_MACRO		13	// .macro
#define P_ENDM		14	// .endm
#define P_LIST		15	// .list
#define P_MLIST		16	// .mlist
#define P_NOLIST	17	// .nolist
#define P_NOMLIST	18	// .nomlist
#define P_RSSET		19	// .rsset
#define P_RS		20	// .rs
#define P_IF		21	// .if
#define P_ELSE		22	// .else
#define P_ENDIF		23	// .endif
#define P_FAIL		24	// .fail
#define P_ZP		25	// .zp
#define P_BSS		26	// .bss
#define P_CODE		27	// .code
#define P_DATA		28	// .data
#define P_DEFCHR	29	// .defchr
#define P_FUNC		30	// .func
#define P_IFDEF		31	// .ifdef
#define P_IFNDEF	32	// .ifndef
#define P_VRAM		33	// .vram
#define P_PAL		34	// .pal
#define P_DEFPAL	35	// .defpal
#define P_DEFSPR	36	// .defspr
#define P_INESPRG	37	// .inesprg
#define P_INESCHR	38	// .ineschr
#define P_INESMAP	39	// .inesmap
#define P_INESMIR	40	// .inesmir
#define P_OPT		41	// .opt
#define P_INCTILE	42	// .inctile
#define P_INCMAP	43	// .incmap
#define P_MML		44	// .mml
#define P_PROC		45	// .proc
#define P_ENDP		46	// .endp
#define P_PGROUP	47	// .procgroup
#define P_ENDPG		48	// .endprocgroup
#define P_CALL		49	// .call

/* symbol flags */
#define MDEF	3	/* multiply defined */
#define UNDEF	1	/* undefined - may be zero page */
#define IFUNDEF	2	/* declared in a .if expression */
#define DEFABS	4	/* defined - two byte address */
#define MACRO	5	/* used for a macro name */
#define FUNC	6	/* used for a function */

/* operation code flags */
#define PSEUDO		0x0008000
#define CLASS1		0x0010000
#define CLASS2		0x0020000
#define CLASS3		0x0040000
#define CLASS5		0x0080000
#define CLASS6		0x0100000
#define CLASS7		0x0200000
#define CLASS8		0x0400000
#define CLASS9		0x0800000
#define CLASS10		0x1000000
#define ACC			0x0000001
#define IMM			0x0000002
#define ZP			0x0000004
#define ZP_X		0x0000008
#define ZP_Y		0x0000010
#define ZP_IND		0x0000020
#define ZP_IND_X	0x0000040
#define ZP_IND_Y	0x0000080
#define ABS			0x0000100
#define ABS_X		0x0000200
#define ABS_Y		0x0000400
#define ABS_IND		0x0000800
#define ABS_IND_X	0x0001000

/* pass flags */
#define FIRST_PASS	0
#define LAST_PASS	1

/* structs */
typedef struct t_opcode {
	struct t_opcode *next;
	char  *name;
	void (*proc)(int *);
	int    flag;
	int    value;
	int    type_idx;
} t_opcode;

typedef struct t_input_info {
	FILE *fp;
	int   lnum;
	int   if_level;
	char  name[116];
} t_input_info;

typedef struct t_proc {
	struct t_proc *next;
	struct t_proc *link;
	struct t_proc *group;
	int  old_bank;
	int  bank;
	int  org;
	int  base;
	int  size;
	int  call;
	int  type;
	int  refcnt;
	char name[SBOLSZ+1];
} t_proc;

typedef struct t_symbol {
	struct t_symbol *next;
	struct t_symbol *local;
	struct t_proc   *proc;
	int  type;
	int  value;
	int  bank;
	int  page;
	int  nb;
	int  size;
	int  vram;
	int  pal;
	int  refcnt;
	int  reserved;
	int  data_type;
	int  data_size;
	char name[SBOLSZ+1];
} t_symbol;

typedef struct t_line {
	struct t_line *next;
	char *data;
} t_line;

typedef struct t_macro {
	struct t_macro *next;
	struct t_line *line;
	char name[SBOLSZ+1];
} t_macro;

typedef struct t_func {
	struct t_func *next;
	char line[128];
	char name[SBOLSZ+1];
} t_func;

typedef struct t_tile {
	struct t_tile *next;
	unsigned char *data;
	unsigned int   crc;
	int index;
} t_tile;

typedef struct t_machine {
	int type;
	char *asm_name;
	char *asm_title;
	char *rom_ext;
	char *include_env;
	unsigned int zp_limit;
	unsigned int ram_limit;
	unsigned int ram_base;
	unsigned int ram_page;
	unsigned int ram_bank;
	struct t_opcode *inst;
	struct t_opcode *pseudo_inst;
    int  (*pack_8x8_tile)(unsigned char *, void *, int, int);
    int  (*pack_16x16_tile)(unsigned char *, void *, int,  int);
    int  (*pack_16x16_sprite)(unsigned char *, void *, int,  int);
    void (*write_header)(FILE *, int);
} MACHINE;

