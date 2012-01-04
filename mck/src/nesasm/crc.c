
/* locals */
static unsigned int crc_table[256];


/* ----
 * crc_init()
 * ----
 */

void
crc_init(void)
{
	int i;
	unsigned int t, *p, *q;
	unsigned int poly = 0x864CFB;

	p = q = crc_table;
	*q++ = 0;
	*q++ = poly;

	for (i = 1; i < 128; i++) {
		t = *(++p);
		if (t & 0x800000) {
			t <<= 1;
			*q++ = t ^ poly;
			*q++ = t;
		}
		else {
			t <<= 1;
			*q++ = t;
			*q++ = t ^ poly;
		}
	}
}


/* ----
 * crc_calc()
 * ----
 * 24-bit crc
 */

unsigned int
crc_calc(unsigned char *data, int len)
{
	unsigned int crc = 0;
	int i;

	for (i = 0; i < len; i++)
		crc = (crc << 8) ^ crc_table[(unsigned char)(crc >> 16) ^ *data++];

	/* ok */
	return (crc & 0xFFFFFF);
}

