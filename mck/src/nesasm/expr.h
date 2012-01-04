/* value types */
#define T_DECIMAL	0
#define T_HEXA		1
#define T_BINARY	2
#define T_CHAR		3
#define T_SYMBOL	4
#define T_PC		5

/* operators */
#define OP_START 0
#define OP_OPEN	 1
#define OP_ADD	 2
#define OP_SUB	 3
#define OP_MUL	 4
#define OP_DIV	 5
#define OP_MOD	 6
#define OP_NEG	 7
#define OP_SHL	 8
#define OP_SHR	 9
#define OP_OR	10
#define OP_XOR	11
#define OP_AND	12
#define OP_COM	13
#define OP_NOT	14
#define OP_EQUAL		15
#define OP_NOT_EQUAL	16
#define OP_LOWER		17
#define OP_LOWER_EQUAL	18
#define OP_HIGHER		19
#define OP_HIGHER_EQUAL	20
#define OP_DEFINED	21
#define OP_HIGH		22
#define OP_LOW		23
#define OP_PAGE		24
#define OP_BANK		25
#define OP_VRAM		26
#define OP_PAL		27
#define OP_SIZEOF	28

/* operator priority */
int op_pri[] = {
	 0 /* START */,  0 /* OPEN  */,
	 7 /* ADD   */,  7 /* SUB   */,  8 /* MUL   */,  8 /* DIV   */,
	 8 /* MOD   */, 10 /* NEG   */,  6 /* SHL   */,  6 /* SHR   */,
	 1 /* OR    */,  2 /* XOR   */,  3 /* AND   */, 10 /* COM   */,
	 9 /* NOT   */,  4 /* =     */,  4 /* <>    */,  5 /* <     */,
	 5 /* <=    */,  5 /* >     */,  5 /* >=    */,
	10 /* DEFIN.*/, 10 /* HIGH  */, 10 /* LOW   */, 10 /* PAGE  */,
	10 /* BANK  */, 10 /* VRAM  */, 10 /* PAL   */, 10 /* SIZEOF*/
};

unsigned int  op_stack[64] = { OP_START };	/* operator stack */
unsigned int val_stack[64];	/* value stack */
int op_idx, val_idx;	/* index in the operator and value stacks */
int need_operator;		/* when set await an operator, else await a value */
unsigned char *expr;	/* pointer to the expression string */
unsigned char *expr_stack[16];	/* expression stack */
struct t_symbol *expr_lablptr;	/* pointer to the lastest label */
int expr_lablcnt;		/* number of label seen in an expression */
char *keyword[8] = {	/* predefined functions */
	"\7DEFINED",
	"\4HIGH", "\3LOW",
	"\4PAGE", "\4BANK",
	"\4VRAM", "\3PAL",
	"\6SIZEOF"
};

