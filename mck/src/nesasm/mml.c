#include <stdio.h>
#include <ctype.h>
#include "defs.h"
#include "externs.h"
#include "protos.h"

/* defines */
#define SND_STOP		0	/* stop processing (track finish) */
#define SND_OFF			1	/* mute sound */
#define SND_ON			2	/* turn on sound */
#define SND_VOLUME		3	/* set voice volume (left & right) */
#define SND_FREQ		4	/* set voice frequency */
#define SND_DURATION	5	/* set voice duration */
#define SND_NOISE_FREQ	6	/* set noise frequency */
#define SND_NOISE_OFF	7	/* set noise off */
#define SND_LFO_FREQ	8	/* set LFO frequency */
#define SND_LFO_CTRL	9	/* set LFO control */
#define SND_WAVE_SINE	20	/* init to sine wave (predefined) */
#define SND_WAVE_SAW	21	/* init to sawtooth wave (predefined) */
#define SND_WAVE_SQR	22	/* init to square wave (predefined) */
#define SND_WAVE_DATA	30	/* init to inline wave data */

/* locals */
static unsigned int snd_wave;
static unsigned int snd_octave;
static unsigned int snd_volume;
static unsigned int snd_length;
static unsigned int snd_tempo;
static unsigned int snd_timebase;
static unsigned int snd_ticks_h, snd_ticks_l;
static int snd_wave_flag;
static int snd_off;
static int tone_table[7]  = { 10, 12, 1, 3, 5, 6, 8 };
static int freq_table[14] = {
	0x001EDD,
	0x0020B3, 0x0022A6,
	0x0024B5, 0x0026E3,
	0x002933,
	0x002BA6, 0x002E40,
	0x003100, 0x0033E8,
	0x003700, 0x003A45,
	0x003DBA,
	0x004166
};

/* protos */
int mml_get_value(char **ptr);
int mml_get_length(char **ptr);
int mml_calc_duration(unsigned int len);


/* ----
 * mml_start()
 * ----
 */

int
mml_start(unsigned char *buffer)
{
	*buffer = SND_OFF;

	snd_off = 1;
	snd_wave_flag = 0;
	snd_octave = 4;
	snd_volume = 15;
	snd_length = 4;
	snd_tempo = 128;
	snd_timebase = 60;
	snd_ticks_h = 0;
	snd_ticks_l = 0;

	/* ok */
	return (1);
}


/* ----
 * mml_stop()
 * ----
 */

int
mml_stop(unsigned char *buffer)
{
	*buffer = SND_STOP;

	return (1);
}


/* ----
 * mml_parse()
 * ----
 */

int
mml_parse(unsigned char *buffer, int bufsize, char *ptr)
{
	unsigned int tone, freq, value, len;
	int size = 0;
	char c;

	/* parse string */
	while ((c = *ptr++) != '\0') {
		/* check buffer size */
		if (bufsize < 7) {
			fatal_error("Internal error: MML buffer too small!");
			return (-1);
		}

		switch (c) {
		/* octave */
		case 'O':
			snd_octave = mml_get_value(&ptr);
	
			if ((snd_octave < 1) || (snd_octave > 7)) {
				error("Incorrect octave!");
				return (-1);
			}
			break;
	
		/* volume */
		case 'V':
			snd_volume = mml_get_value(&ptr);
	
			if (snd_volume > 15) {
				error("Incorrect volume!");
				return (-1);
			}
	
			/* gen code */
			*buffer++ = SND_VOLUME;
			*buffer++ = (snd_volume << 4) + snd_volume;
			size += 2;
			break;

		/* tempo */
		case 'T':
			snd_tempo = mml_get_value(&ptr);
	
			if ((snd_tempo < 32) || (snd_tempo > 256)) {
				error("Incorrect tempo!");
				return (-1);
			}
			break;
	
		/* length */
		case 'L':
			snd_length = mml_get_length(&ptr);
	
			if (!snd_length) {
				error("Incorrect note length!");
				return (-1);
			}
			break;
	
		/* notes */
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
			/* base */
			tone = tone_table[c - 'A'];

			/* extra commands */
			if ((*ptr == '#') || (*ptr == '+')) {
				tone++;
				ptr++;
			}
			else if (*ptr == '-') {
				tone--;
				ptr++;
			}
			if (isdigit(*ptr))
				len = (mml_get_length(&ptr) << 8);
			else {
				len = (snd_length << 8);
			}
			if (*ptr == '.') {
				len = (len * 0xC0) >> 8;
				ptr++;
			}

			/* check length */
			if (!len) {
				error("Incorrect note length!");
				return (-1);
			}

			/* calculate frequency */
			freq  = (freq_table[tone] << (snd_octave - 1));
			value = (((3580000 * 16) / freq) + 1) >> 1;

			/* gen code */
			*buffer++ = SND_FREQ;
			*buffer++ = (value)  & 0xFF;
			*buffer++ = (value >> 8) & 0xFF;

			if (snd_wave_flag) {
				/* new waveform */
				snd_wave_flag = 0;
				snd_off = 0;
			   *buffer++ = SND_WAVE_SINE + (snd_wave - 1);
				size += 1;
			}
			else {
				if (snd_off) {
					/* sound on */
					snd_off = 0;
				   *buffer++ = SND_ON;
					size += 1;
				}
			}

			*buffer++ = SND_DURATION;
			*buffer++ = mml_calc_duration(len);
			size += 5;
			break;
	
		/* rest */
		case 'R':
			/* local length */
			if (isdigit(*ptr))
				len = (mml_get_length(&ptr) << 8);
			else {
				len = (0x0400);
			}
			if (*ptr == '.') {
				len = (len * 0xC0) >> 8;
				ptr++;
			}

			/* check length */
			if (!len) {
				error("Incorrect note length!");
				return (-1);
			}

			/* gen code */
			*buffer++ = SND_OFF;
			*buffer++ = SND_DURATION;
			*buffer++ = mml_calc_duration(len);
			snd_off = 1;
			size += 3;
			break;

		/* waveform */
		case 'W':
			snd_wave = mml_get_value(&ptr);
			snd_wave_flag = 1;
	
			if ((snd_wave < 1) || (snd_wave > 3)) {
				error("Incorrect waveform!");
				return (-1);
			}
			break;
	
		/* other */
		default:
			error("Syntax error!");
			return (-1);
		}

		/* update size */
		bufsize -= size;
	}

	/* ok */
	return (size);
}


/* ----
 * mml_get_value()
 * ----
 */

int
mml_get_value(char **ptr)
{
	unsigned int value;
	char c;

	/* get value */
	value = 0;

	for (;;) {
		c = **ptr;

		if (!isdigit(c))
			break;
		if (value > 65535)
			return (-1);

		value = (value * 10) + (c - '0');
		(*ptr)++;
	}

	/* ok */
	return (value);
}


/* ----
 * mml_get_length()
 * ----
 */

int
mml_get_length(char **ptr)
{
	int len;

	len = mml_get_value(ptr);

	switch (len) {
	case  1:
	case  2:
	case  3:
	case  4:
	case  6:
	case  8:
	case 12:
	case 16:
	case 24:
	case 32:
	case 48:
	case 64:
	case 96:
		return (len);

	default:
		return (0);
	}
}


/* ----
 * mml_calc_duration()
 * ----
 */

int
mml_calc_duration(unsigned int len)
{
	unsigned int ticks, mask, tmp;

	/* ticks */
	ticks = 0xC00000 / len;

	/* base */
	if (snd_timebase == 0) {
		/* timer */
		mask = 0xFF;
		snd_ticks_h = (ticks >> 8);
		snd_ticks_l = (ticks & 0xFF) + snd_ticks_l;
	}
	else {
		/* fixed frequency (ie. vsync) */
		mask = 0xFFFFFF;
		tmp  = (256 - snd_tempo) * snd_timebase * ticks * 8;
		snd_ticks_h = (tmp >> 24);
		snd_ticks_l = (tmp & 0xFFFFFF) + snd_ticks_l;
	}

	/* adjust timings */
	if (snd_ticks_l  > mask) {
		snd_ticks_l &= mask;
		snd_ticks_h++;
	}
	if (snd_ticks_h == 0)
		snd_ticks_h = 1;

	/* result */
	return (snd_ticks_h - 1);
}

