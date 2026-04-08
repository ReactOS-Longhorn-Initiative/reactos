/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     _ctype table for character classification
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"

const unsigned short _ctype[257] =
{
    0X0000,				// Dummy
    //						DEC HEX Sym	Description
    _CONTROL,			//	0	00	NUL	Null character
    _CONTROL,			//	1	01	SOH	Start of Heading
    _CONTROL,			//	2	02	STX	Start of Text
    _CONTROL,			//	3	03	ETX	End of Text
    _CONTROL,			//	4	04	EOT	End of Transmission
    _CONTROL,			//	5	05	ENQ	Enquiry
    _CONTROL,			//	6	06	ACK	Acknowledge
    _CONTROL,			//	7	07	BEL	Bell, Alert
    _CONTROL,			//	8	08	BS	Backspace
    _CONTROL | _SPACE,	//	9	09	HT	Horizontal Tab
    _CONTROL | _SPACE,	//	10	0A	LF	Line Feed
    _CONTROL | _SPACE,	//	11	0B	VT	Vertical Tabulation
    _CONTROL | _SPACE,	//	12	0C	FF	Form Feed
    _CONTROL | _SPACE,	//	13	0D	CR	Carriage Return
    _CONTROL,			//	14	0E	SO	Shift Out
    _CONTROL,			//	15	0F	SI	Shift In
    _CONTROL,			//	16	10	DLE	Data Link Escape
    _CONTROL,			//	17	11	DC1	Device Control One (XON)
    _CONTROL,			//	18	12	DC2	Device Control Two
    _CONTROL,			//	19	13	DC3	Device Control Three (XOFF)
    _CONTROL,			//	20	14	DC4	Device Control Four
    _CONTROL,			//	21	15	NAK	Negative Acknowledge
    _CONTROL,			//	22	16	SYN	Synchronous Idle
    _CONTROL,			//	23	17	ETB	End of Transmission Block
    _CONTROL,			//	24	18	CAN	Cancel
    _CONTROL,			//	25	19	EM	End of medium
    _CONTROL,			//	26	1A	SUB	Substitute
    _CONTROL,			//	27	1B	ESC	Escape
    _CONTROL,			//	28	1C	FS	File Separator
    _CONTROL,			//	29	1D	GS	Group Separator
    _CONTROL,			//	30	1E	RS	Record Separator
    _CONTROL,			//	31	1F	US	Unit Separator
    _SPACE | _BLANK,	//	32	20	SP	Space
    _PUNCT,				//	33	21	!	Exclamation mark
    _PUNCT,				//	34	22	"	Double quotes
    _PUNCT,				//	35	23	#	Number sign
    _PUNCT,				//	36	24	$	Dollar
    _PUNCT,				//	37	25	%	Per cent sign
    _PUNCT,				//	38	26	&	Ampersand
    _PUNCT,				//	39	27	'	Single quote
    _PUNCT,				//	40	28	(	Open parenthesis
    _PUNCT,				//	41	29	)	Close parenthesis
    _PUNCT,				//	42	2A	*	Asterisk
    _PUNCT,				//	43	2B	+	Plus
    _PUNCT,				//	44	2C	,	Comma
    _PUNCT,				//	45	2D	-	Hyphen-minus
    _PUNCT,				//	46	2E	.	Period, dot or full stop
    _PUNCT,				//	47	2F	/	Slash or divide
    _DIGIT | _HEX,		//	48	30	0	Zero
    _DIGIT | _HEX,		//	49	31	1	One
    _DIGIT | _HEX,		//	50	32	2	Two
    _DIGIT | _HEX,		//	51	33	3	Three
    _DIGIT | _HEX,		//	52	34	4	Four
    _DIGIT | _HEX,		//	53	35	5	Five
    _DIGIT | _HEX,		//	54	36	6	Six
    _DIGIT | _HEX,		//	55	37	7	Seven
    _DIGIT | _HEX,		//	56	38	8	Eight
    _DIGIT | _HEX,		//	57	39	9	Nine
    _PUNCT,				//	58	3A	:	Colon
    _PUNCT,				//	59	3B	;	Semicolon
    _PUNCT,				//	60	3C	<	Less than
    _PUNCT,				//	61	3D	=	Equals
    _PUNCT,				//	62	3E	>	Greater than
    _PUNCT,				//	63	3F	?	Question mark
    _PUNCT,				//	64	40	@	At sign
    _UPPER | _HEX,		//	65	41	A	Uppercase A
    _UPPER | _HEX,		//	66	42	B	Uppercase B
    _UPPER | _HEX,		//	67	43	C	Uppercase C
    _UPPER | _HEX,		//	68	44	D	Uppercase D
    _UPPER | _HEX,		//	69	45	E	Uppercase E
    _UPPER | _HEX,		//	70	46	F	Uppercase F
    _UPPER,				//	71	47	G	Uppercase G
    _UPPER,				//	72	48	H	Uppercase H
    _UPPER,				//	73	49	I	Uppercase I
    _UPPER,				//	74	4A	J	Uppercase J
    _UPPER,				//	75	4B	K	Uppercase K
    _UPPER,				//	76	4C	L	Uppercase L
    _UPPER,				//	77	4D	M	Uppercase M
    _UPPER,				//	78	4E	N	Uppercase N
    _UPPER,				//	79	4F	O	Uppercase O
    _UPPER,				//	80	50	P	Uppercase P
    _UPPER,				//	81	51	Q	Uppercase Q
    _UPPER,				//	82	52	R	Uppercase R
    _UPPER,				//	83	53	S	Uppercase S
    _UPPER,				//	84	54	T	Uppercase T
    _UPPER,				//	85	55	U	Uppercase U
    _UPPER,				//	86	56	V	Uppercase V
    _UPPER,				//	87	57	W	Uppercase W
    _UPPER,				//	88	58	X	Uppercase X
    _UPPER,				//	89	59	Y	Uppercase Y
    _UPPER,				//	90	5A	Z	Uppercase Z
    _PUNCT,				//	91	5B	[	Opening bracket
    _PUNCT,				//	92	5C	\	Backslash
    _PUNCT,				//	93	5D	]	Closing bracket
    _PUNCT,				//	94	5E	^	Caret - circumflex
    _PUNCT,				//	95	5F	_	Underscore
    _PUNCT,				//	96	60	`	Grave accent
    _LOWER | _HEX,		//	97	61	a	Lowercase a
    _LOWER | _HEX,		//	98	62	b	Lowercase b
    _LOWER | _HEX,		//	99	63	c	Lowercase c
    _LOWER | _HEX,		//	100	64	d	Lowercase d
    _LOWER | _HEX,		//	101	65	e	Lowercase e
    _LOWER | _HEX,		//	102	66	f	Lowercase f
    _LOWER,				//	103	67	g	Lowercase g
    _LOWER,				//	104	68	h	Lowercase h
    _LOWER,				//	105	69	i	Lowercase i
    _LOWER,				//	106	6A	j	Lowercase j
    _LOWER,				//	107	6B	k	Lowercase k
    _LOWER,				//	108	6C	l	Lowercase l
    _LOWER,				//	109	6D	m	Lowercase m
    _LOWER,				//	110	6E	n	Lowercase n
    _LOWER,				//	111	6F	o	Lowercase o
    _LOWER,				//	112	70	p	Lowercase p
    _LOWER,				//	113	71	q	Lowercase q
    _LOWER,				//	114	72	r	Lowercase r
    _LOWER,				//	115	73	s	Lowercase s
    _LOWER,				//	116	74	t	Lowercase t
    _LOWER,				//	117	75	u	Lowercase u
    _LOWER,				//	118	76	v	Lowercase v
    _LOWER,				//	119	77	w	Lowercase w
    _LOWER,				//	120	78	x	Lowercase x
    _LOWER,				//	121	79	y	Lowercase y
    _LOWER,				//	122	7A	z	Lowercase z
    _PUNCT,				//	123	7B	{	Opening brace
    _PUNCT,				//	124	7C	|	Vertical bar
    _PUNCT,				//	125	7D	}	Closing brace
    _PUNCT,				//	126	7E	~	Equivalency sign - tilde
    _CONTROL,			//	127	7F	DEL	Delete
    0X0000,				//	128	80	€	Euro sign
    0X0000,				//	129	81		Unused
    0X0000,				//	130	82	‚	Single low-9 quotation mark
    0X0000,				//	131	83	ƒ	Latin small letter f with hook
    0X0000,				//	132	84	„	Double low-9 quotation mark
    0X0000,				//	133	85	…	Horizontal ellipsis
    0X0000,				//	134	86	†	Dagger
    0X0000,				//	135	87	‡	Double dagger
    0X0000,				//	136	88	ˆ	Modifier letter circumflex accent
    0X0000,				//	137	89	‰	Per mille sign
    0X0000,				//	138	8A	Š	Latin capital letter S with caron
    0X0000,				//	139	8B	‹	Single left-pointing angle quotation
    0X0000,				//	140	8C	Œ	Latin capital ligature OE
    0X0000,				//	141	8D		Unused
    0X0000,				//	142	8E	Ž	Latin capital letter Z with caron
    0X0000,				//	143	8F		Unused
    0X0000,				//	144	90		Unused
    0X0000,				//	145	91	‘	Left single quotation mark
    0X0000,				//	146	92	’	Right single quotation mark
    0X0000,				//	147	93	“	Left double quotation mark
    0X0000,				//	148	94	”	Right double quotation mark
    0X0000,				//	149	95	•	Bullet
    0X0000,				//	150	96	–	En dash
    0X0000,				//	151	97	—	Em dash
    0X0000,				//	152	98	˜	Small tilde
    0X0000,				//	153	99	™	Trade mark sign
    0X0000,				//	154	9A	š	Latin small letter S with caron
    0X0000,				//	155	9B	›	Single right-pointing angle quotation mark
    0X0000,				//	156	9C	œ	Latin small ligature oe
    0X0000,				//	157	9D		Unused
    0X0000,				//	158	9E	ž	Latin small letter z with caron
    0X0000,				//	159	9F	Ÿ	Latin capital letter Y with diaeresis
    0X0000,				//	160	A0	NBSP	Non-breaking space
    0X0000,				//	161	A1	¡	Inverted exclamation mark
    0X0000,				//	162	A2	¢	Cent sign
    0X0000,				//	163	A3	£	Pound sign
    0X0000,				//	164	A4	¤	Currency sign
    0X0000,				//	165	A5	¥	Yen sign
    0X0000,				//	166	A6	¦	Pipe, broken vertical bar
    0X0000,				//	167	A7	§	Section sign
    0X0000,				//	168	A8	¨	Spacing diaeresis - umlaut
    0X0000,				//	169	A9	©	Copyright sign
    0X0000,				//	170	AA	ª	Feminine ordinal indicator
    0X0000,				//	171	AB	«	Left double angle quotes
    0X0000,				//	172	AC	¬	Negation
    0X0000,				//	173	AD	­SHY	Soft hyphen
    0X0000,				//	174	AE	®	Registered trade mark sign
    0X0000,				//	175	AF	¯	Spacing macron - overline
    0X0000,				//	176	B0	°	Degree sign
    0X0000,				//	177	B1	±	Plus-or-minus sign
    0X0000,				//	178	B2	²	Superscript two - squared
    0X0000,				//	179	B3	³	Superscript three - cubed
    0X0000,				//	180	B4	´	Acute accent - spacing acute
    0X0000,				//	181	B5	µ	Micro sign
    0X0000,				//	182	B6	¶	Pilcrow sign - paragraph sign
    0X0000,				//	183	B7	·	Middle dot - Georgian comma
    0X0000,				//	184	B8	¸	Spacing cedilla
    0X0000,				//	185	B9	¹	Superscript one
    0X0000,				//	186	BA	º	Masculine ordinal indicator
    0X0000,				//	187	BB	»	Right double angle quotes
    0X0000,				//	188	BC	¼	Fraction one quarter
    0X0000,				//	189	BD	½	Fraction one half
    0X0000,				//	190	BE	¾	Fraction three quarters
    0X0000,				//	191	BF	¿	Inverted question mark
    0X0000,				//	192	C0	À	Latin capital letter A with grave
    0X0000,				//	193	C1	Á	Latin capital letter A with acute
    0X0000,				//	194	C2	Â	Latin capital letter A with circumflex
    0X0000,				//	195	C3	Ã	Latin capital letter A with tilde
    0X0000,				//	196	C4	Ä	Latin capital letter A with diaeresis
    0X0000,				//	197	C5	Å	Latin capital letter A with ring above
    0X0000,				//	198	C6	Æ	Latin capital letter AE
    0X0000,				//	199	C7	Ç	Latin capital letter C with cedilla
    0X0000,				//	200	C8	È	Latin capital letter E with grave
    0X0000,				//	201	C9	É	Latin capital letter E with acute
    0X0000,				//	202	CA	Ê	Latin capital letter E with circumflex
    0X0000,				//	203	CB	Ë	Latin capital letter E with diaeresis
    0X0000,				//	204	CC	Ì	Latin capital letter I with grave
    0X0000,				//	205	CD	Í	Latin capital letter I with acute
    0X0000,				//	206	CE	Î	Latin capital letter I with circumflex
    0X0000,				//	207	CF	Ï	Latin capital letter I with diaeresis
    0X0000,				//	208	D0	Ð	Latin capital letter ETH
    0X0000,				//	209	D1	Ñ	Latin capital letter N with tilde
    0X0000,				//	210	D2	Ò	Latin capital letter O with grave
    0X0000,				//	211	D3	Ó	Latin capital letter O with acute
    0X0000,				//	212	D4	Ô	Latin capital letter O with circumflex
    0X0000,				//	213	D5	Õ	Latin capital letter O with tilde
    0X0000,				//	214	D6	Ö	Latin capital letter O with diaeresis
    0X0000,				//	215	D7	×	Multiplication sign
    0X0000,				//	216	D8	Ø	Latin capital letter O with slash
    0X0000,				//	217	D9	Ù	Latin capital letter U with grave
    0X0000,				//	218	DA	Ú	Latin capital letter U with acute
    0X0000,				//	219	DB	Û	Latin capital letter U with circumflex
    0X0000,				//	220	DC	Ü	Latin capital letter U with diaeresis
    0X0000,				//	221	DD	Ý	Latin capital letter Y with acute
    0X0000,				//	222	DE	Þ	Latin capital letter THORN
    0X0000,				//	223	DF	ß	Latin small letter sharp s - ess-zed
    0X0000,				//	224	E0	à	Latin small letter a with grave
    0X0000,				//	225	E1	á	Latin small letter a with acute
    0X0000,				//	226	E2	â	Latin small letter a with circumflex
    0X0000,				//	227	E3	ã	Latin small letter a with tilde
    0X0000,				//	228	E4	ä	Latin small letter a with diaeresis
    0X0000,				//	229	E5	å	Latin small letter a with ring above
    0X0000,				//	230	E6	æ	Latin small letter ae
    0X0000,				//	231	E7	ç	Latin small letter c with cedilla
    0X0000,				//	232	E8	è	Latin small letter e with grave
    0X0000,				//	233	E9	é	Latin small letter e with acute
    0X0000,				//	234	EA	ê	Latin small letter e with circumflex
    0X0000,				//	235	EB	ë	Latin small letter e with diaeresis
    0X0000,				//	236	EC	ì	Latin small letter i with grave
    0X0000,				//	237	ED	í	Latin small letter i with acute
    0X0000,				//	238	EE	î	Latin small letter i with circumflex
    0X0000,				//	239	EF	ï	Latin small letter i with diaeresis
    0X0000,				//	240	F0	ð	Latin small letter eth
    0X0000,				//	241	F1	ñ	Latin small letter n with tilde
    0X0000,				//	242	F2	ò	Latin small letter o with grave
    0X0000,				//	243	F3	ó	Latin small letter o with acute
    0X0000,				//	244	F4	ô	Latin small letter o with circumflex
    0X0000,				//	245	F5	õ	Latin small letter o with tilde
    0X0000,				//	246	F6	ö	Latin small letter o with diaeresis
    0X0000,				//	247	F7	÷	Division sign
    0X0000,				//	248	F8	ø	Latin small letter o with slash
    0X0000,				//	249	F9	ù	Latin small letter u with grave
    0X0000,				//	250	FA	ú	Latin small letter u with acute
    0X0000,				//	251	FB	û	Latin small letter u with circumflex
    0X0000,				//	252	FC	ü	Latin small letter u with diaeresis
    0X0000,				//	253	FD	ý	Latin small letter y with acute
    0X0000,				//	254	FE	þ	Latin small letter thorn
    0X0000,				//	255	FF	ÿ	Latin small letter y with diaeresis
};

#undef _pctype
const unsigned short* _pctype = &_ctype[1];

const unsigned short * __cdecl __pctype_func(void)
{
    return &_ctype[1];
}
