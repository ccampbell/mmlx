extern unsigned char rom[128][8192];
extern unsigned char map[128][8192];
extern char bank_name[128][64];
extern int  bank_loccnt[4][256];
extern int  bank_page[4][256];
extern int max_zp;		/* higher used address in zero page */
extern int max_bss;		/* higher used address in ram */
extern int max_bank;	/* last bank used */
extern int data_loccnt;	/* data location counter */
extern int data_size;	/* size of binary output (in bytes) */
extern int data_level;	/* data output level, must be <= listlevel to be outputed */
extern int loccnt;	/* location counter */
extern int bank;	/* current bank */
extern int bank_base;	/* bank base index */
extern int bank_limit;	/* bank limit */
extern int rom_limit;	/* rom max. size in bytes */
extern int page;	/* page */
extern int rsbase;	/* .rs counter */
extern int section;	/* current section: S_ZP, S_BSS, S_CODE or S_DATA */
extern int section_bank[4];	/* current bank for each section */
extern int in_if;	/* true if in a '.if' statement */
extern int if_expr;	/* set when parsing an .if expression */
extern int if_level;	/* level of nested .if's */
extern int skip_lines;	/* when true skip lines */
extern int continued_line;	/* set when a line is the continuation of another line */
extern int pcx_w, pcx_h;	/* PCX dimensions */
extern int pcx_nb_colors;	/* number of colors (16/256) in the PCX */
extern int pcx_nb_args;		/* number of argument */
extern unsigned int   pcx_arg[8];	/* PCX args array */
extern unsigned char *pcx_buf;	/* pointer to the PCX buffer */
extern unsigned char  pcx_pal[256][3];	/* palette */
extern unsigned char *expr;	/* expression string pointer */
extern int  mopt;
extern int  in_macro;
extern int  expand_macro;
extern char marg[8][10][80];
extern int  midx;
extern int  mcounter, mcntmax;
extern int  mcntstack[8];
extern struct t_line  *mstack[8];
extern struct t_line  *mlptr;
extern struct t_macro *macro_tbl[256];
extern struct t_macro *mptr;
extern struct t_func  *func_tbl[256];
extern struct t_func  *func_ptr;
extern struct t_proc  *proc_ptr;
extern int   proc_nb;
extern char  func_arg[8][10][80];
extern int   func_idx;
extern int   infile_error;
extern int   infile_num;
extern FILE	*out_fp;	/* file pointers, output */
extern FILE	*in_fp;		/* input */
extern FILE	*lst_fp;	/* listing */
extern struct t_input_info input_file[8];
extern struct t_machine *machine;
extern struct t_machine  nes;
extern struct t_machine  pce;
extern struct t_opcode *inst_tbl[256];	/* instructions hash table */
extern struct t_symbol *hash_tbl[256];	/* label hash table */
extern struct t_symbol *lablptr;	/* label pointer into symbol table */
extern struct t_symbol *glablptr;	/* pointer to the latest defined global symbol */
extern struct t_symbol *lastlabl;	/* last label we have seen */
extern struct t_symbol *bank_glabl[4][256];	/* latest global label in each bank */
extern char hex[];		/* hexadecimal character buffer */
extern int  stop_pass;	/* stop the program; set by fatal_error() */
extern int  errcnt;		/* error counter */
extern void (*opproc)(int *);	/* instruction gen proc */
extern int  opflg;		/* instruction flags */
extern int  opval;		/* instruction value */
extern int  optype;		/* instruction type */
extern char opext;		/* instruction extension (.l or .h) */
extern int  pass;		/* pass counter */
extern char prlnbuf[];	/* input line buffer */
extern char tmplnbuf[];	/* temporary line buffer */
extern int  slnum;		/* source line number counter */
extern char symbol[];	/* temporary symbol storage */
extern int  undef;		/* undefined symbol in expression flag */
extern unsigned int	value;	/* operand field value */
extern int  mlist_opt;	/* macro listing main flag */
extern int  xlist;		/* listing file main flag */
extern int  list_level;	/* output level */
extern int  asm_opt[8];	/* assembler option state */
extern int  opvaltab[6][16];

