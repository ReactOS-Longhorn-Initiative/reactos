/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     _wctype table for wide character classification
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_wctype.h"

const unsigned short _wctype[257] =
{
    0X0000,				// Dummy
    //									DEC	HEX	Sym	Description
    _CONTROL,						//	0	00	NUL	Null character
    _CONTROL,						//	1	01	SOH	Start of Heading
    _CONTROL,						//	2	02	STX	Start of Text
    _CONTROL,						//	3	03	ETX	End of Text
    _CONTROL,						//	4	04	EOT	End of Transmission
    _CONTROL,						//	5	05	ENQ	Enquiry
    _CONTROL,						//	6	06	ACK	Acknowledge
    _CONTROL,						//	7	07	BEL	Bell, Alert
    _CONTROL,						//	8	08	BS	Backspace
    _CONTROL | _SPACE | _BLANK,		//	9	09	HT	Horizontal Tab
    _CONTROL | _SPACE,				//	10	0A	LF	Line Feed
    _CONTROL | _SPACE,				//	11	0B	VT	Vertical Tabulation
    _CONTROL | _SPACE,				//	12	0C	FF	Form Feed
    _CONTROL | _SPACE,				//	13	0D	CR	Carriage Return
    _CONTROL,						//	14	0E	SO	Shift Out
    _CONTROL,						//	15	0F	SI	Shift In
    _CONTROL,						//	16	10	DLE	Data Link Escape
    _CONTROL,						//	17	11	DC1	Device Control One (XON)
    _CONTROL,						//	18	12	DC2	Device Control Two
    _CONTROL,						//	19	13	DC3	Device Control Three (XOFF)
    _CONTROL,						//	20	14	DC4	Device Control Four
    _CONTROL,						//	21	15	NAK	Negative Acknowledge
    _CONTROL,						//	22	16	SYN	Synchronous Idle
    _CONTROL,						//	23	17	ETB	End of Transmission Block
    _CONTROL,						//	24	18	CAN	Cancel
    _CONTROL,						//	25	19	EM	End of medium
    _CONTROL,						//	26	1A	SUB	Substitute
    _CONTROL,						//	27	1B	ESC	Escape
    _CONTROL,						//	28	1C	FS	File Separator
    _CONTROL,						//	29	1D	GS	Group Separator
    _CONTROL,						//	30	1E	RS	Record Separator
    _CONTROL,						//	31	1F	US	Unit Separator
    _SPACE | _BLANK,				//	32	20	SP	Space
    _PUNCT,							//	33	21	!	Exclamation mark
    _PUNCT,							//	34	22	"	Double quotes
    _PUNCT,							//	35	23	#	Number sign
    _PUNCT,							//	36	24	$	Dollar
    _PUNCT,							//	37	25	%	Per cent sign
    _PUNCT,							//	38	26	&	Ampersand
    _PUNCT,							//	39	27	'	Single quote
    _PUNCT,							//	40	28	(	Open parenthesis
    _PUNCT,							//	41	29	)	Close parenthesis
    _PUNCT,							//	42	2A	*	Asterisk
    _PUNCT,							//	43	2B	+	Plus
    _PUNCT,							//	44	2C	,	Comma
    _PUNCT,							//	45	2D	-	Hyphen-minus
    _PUNCT,							//	46	2E	.	Period, dot or full stop
    _PUNCT,							//	47	2F	/	Slash or divide
    _DIGIT | _HEX,					//	48	30	0	Zero
    _DIGIT | _HEX,					//	49	31	1	One
    _DIGIT | _HEX,					//	50	32	2	Two
    _DIGIT | _HEX,					//	51	33	3	Three
    _DIGIT | _HEX,					//	52	34	4	Four
    _DIGIT | _HEX,					//	53	35	5	Five
    _DIGIT | _HEX,					//	54	36	6	Six
    _DIGIT | _HEX,					//	55	37	7	Seven
    _DIGIT | _HEX,					//	56	38	8	Eight
    _DIGIT | _HEX,					//	57	39	9	Nine
    _PUNCT,							//	58	3A	:	Colon
    _PUNCT,							//	59	3B	;	Semicolon
    _PUNCT,							//	60	3C	<	Less than
    _PUNCT,							//	61	3D	=	Equals
    _PUNCT,							//	62	3E	>	Greater than
    _PUNCT,							//	63	3F	?	Question mark
    _PUNCT,							//	64	40	@	At sign
    0x0100 | _UPPER | _HEX,			//	65	41	A	Uppercase A
    0x0100 | _UPPER | _HEX,			//	66	42	B	Uppercase B
    0x0100 | _UPPER | _HEX,			//	67	43	C	Uppercase C
    0x0100 | _UPPER | _HEX,			//	68	44	D	Uppercase D
    0x0100 | _UPPER | _HEX,			//	69	45	E	Uppercase E
    0x0100 | _UPPER | _HEX,			//	70	46	F	Uppercase F
    0x0100 | _UPPER,				//	71	47	G	Uppercase G
    0x0100 | _UPPER,				//	72	48	H	Uppercase H
    0x0100 | _UPPER,				//	73	49	I	Uppercase I
    0x0100 | _UPPER,				//	74	4A	J	Uppercase J
    0x0100 | _UPPER,				//	75	4B	K	Uppercase K
    0x0100 | _UPPER,				//	76	4C	L	Uppercase L
    0x0100 | _UPPER,				//	77	4D	M	Uppercase M
    0x0100 | _UPPER,				//	78	4E	N	Uppercase N
    0x0100 | _UPPER,				//	79	4F	O	Uppercase O
    0x0100 | _UPPER,				//	80	50	P	Uppercase P
    0x0100 | _UPPER,				//	81	51	Q	Uppercase Q
    0x0100 | _UPPER,				//	82	52	R	Uppercase R
    0x0100 | _UPPER,				//	83	53	S	Uppercase S
    0x0100 | _UPPER,				//	84	54	T	Uppercase T
    0x0100 | _UPPER,				//	85	55	U	Uppercase U
    0x0100 | _UPPER,				//	86	56	V	Uppercase V
    0x0100 | _UPPER,				//	87	57	W	Uppercase W
    0x0100 | _UPPER,				//	88	58	X	Uppercase X
    0x0100 | _UPPER,				//	89	59	Y	Uppercase Y
    0x0100 | _UPPER,				//	90	5A	Z	Uppercase Z
    _PUNCT,							//	91	5B	[	Opening bracket
    _PUNCT,							//	92	5C	\	Backslash
    _PUNCT,							//	93	5D	]	Closing bracket
    _PUNCT,							//	94	5E	^	Caret - circumflex
    _PUNCT,							//	95	5F	_	Underscore
    _PUNCT,							//	96	60	`	Grave accent
    0x0100 | _LOWER | _HEX,			//	97	61	a	Lowercase a
    0x0100 | _LOWER | _HEX,			//	98	62	b	Lowercase b
    0x0100 | _LOWER | _HEX,			//	99	63	c	Lowercase c
    0x0100 | _LOWER | _HEX,			//	100	64	d	Lowercase d
    0x0100 | _LOWER | _HEX,			//	101	65	e	Lowercase e
    0x0100 | _LOWER | _HEX,			//	102	66	f	Lowercase f
    0x0100 | _LOWER,				//	103	67	g	Lowercase g
    0x0100 | _LOWER,				//	104	68	h	Lowercase h
    0x0100 | _LOWER,				//	105	69	i	Lowercase i
    0x0100 | _LOWER,				//	106	6A	j	Lowercase j
    0x0100 | _LOWER,				//	107	6B	k	Lowercase k
    0x0100 | _LOWER,				//	108	6C	l	Lowercase l
    0x0100 | _LOWER,				//	109	6D	m	Lowercase m
    0x0100 | _LOWER,				//	110	6E	n	Lowercase n
    0x0100 | _LOWER,				//	111	6F	o	Lowercase o
    0x0100 | _LOWER,				//	112	70	p	Lowercase p
    0x0100 | _LOWER,				//	113	71	q	Lowercase q
    0x0100 | _LOWER,				//	114	72	r	Lowercase r
    0x0100 | _LOWER,				//	115	73	s	Lowercase s
    0x0100 | _LOWER,				//	116	74	t	Lowercase t
    0x0100 | _LOWER,				//	117	75	u	Lowercase u
    0x0100 | _LOWER,				//	118	76	v	Lowercase v
    0x0100 | _LOWER,				//	119	77	w	Lowercase w
    0x0100 | _LOWER,				//	120	78	x	Lowercase x
    0x0100 | _LOWER,				//	121	79	y	Lowercase y
    0x0100 | _LOWER,				//	122	7A	z	Lowercase z
    _PUNCT,							//	123	7B	{	Opening brace
    _PUNCT,							//	124	7C	|	Vertical bar
    _PUNCT,							//	125	7D	}	Closing brace
    _PUNCT,							//	126	7E	~	Equivalency sign - tilde
    _CONTROL,						//	127	7F	DEL	Delete
    _CONTROL,						//	128	80		Control
    _CONTROL,						//	129	81		Control
    _CONTROL,						//	130	82		Control
    _CONTROL,						//	131	83		Control
    _CONTROL,						//	132	84		Control
    _CONTROL,						//	133	85		Control
    _CONTROL,						//	134	86		Control
    _CONTROL,						//	135	87		Control
    _CONTROL,						//	136	88		Control
    _CONTROL,						//	137	89		Control
    _CONTROL,						//	138	8A		Control
    _CONTROL,						//	139	8B		Control
    _CONTROL,						//	140	8C		Control
    _CONTROL,						//	141	8D		Control
    _CONTROL,						//	142	8E		Control
    _CONTROL,						//	143	8F		Control
    _CONTROL,						//	144	90		Control
    _CONTROL,						//	145	91		Control
    _CONTROL,						//	146	92		Control
    _CONTROL,						//	147	93		Control
    _CONTROL,						//	148	94		Control
    _CONTROL,						//	149	95		Control
    _CONTROL,						//	150	96		Control
    _CONTROL,						//	151	97		Control
    _CONTROL,						//	152	98		Control
    _CONTROL,						//	153	99		Control
    _CONTROL,						//	154	9A		Control
    _CONTROL,						//	155	9B		Control
    _CONTROL,						//	156	9C		Control
    _CONTROL,						//	157	9D		Control
    _CONTROL,						//	158	9E		Control
    _CONTROL,						//	159	9F		Control
    _BLANK | _SPACE,				//	160	A0	NBSP	Non-breaking space
    _PUNCT,							//	161	A1	¡	Inverted exclamation mark
    _PUNCT,							//	162	A2	¢	Cent sign
    _PUNCT,							//	163	A3	£	Pound sign
    _PUNCT,							//	164	A4	¤	Currency sign
    _PUNCT,							//	165	A5	¥	Yen sign
    _PUNCT,							//	166	A6	¦	Pipe, broken vertical bar
    _PUNCT,							//	167	A7	§	Section sign
    _PUNCT,							//	168	A8	¨	Spacing diaeresis - umlaut
    _PUNCT,							//	169	A9	©	Copyright sign
    _PUNCT,							//	170	AA	ª	Feminine ordinal indicator
    _PUNCT,							//	171	AB	«	Left double angle quotes
    _PUNCT,							//	172	AC	¬	Negation
    _PUNCT,							//	173	AD	SHY	Soft hyphen
    _PUNCT,							//	174	AE	®	Registered trade mark sign
    _PUNCT,							//	175	AF	¯	Spacing macron - overline
    _PUNCT,							//	176	B0	°	Degree sign
    _PUNCT,							//	177	B1	±	Plus-or-minus sign
    _PUNCT | _DIGIT,				//	178	B2	²	Superscript two - squared
    _PUNCT | _DIGIT,				//	179	B3	³	Superscript three - cubed
    _PUNCT,							//	180	B4	´	Acute accent - spacing acute
    _PUNCT,							//	181	B5	µ	Micro sign
    _PUNCT,							//	182	B6	¶	Pilcrow sign - paragraph sign
    _PUNCT,							//	183	B7	·	Middle dot - Georgian comma
    _PUNCT,							//	184	B8	¸	Spacing cedilla
    _PUNCT | _DIGIT,				//	185	B9	¹	Superscript one
    _PUNCT,							//	186	BA	º	Masculine ordinal indicator
    _PUNCT,							//	187	BB	»	Right double angle quotes
    _PUNCT,							//	188	BC	¼	Fraction one quarter
    _PUNCT,							//	189	BD	½	Fraction one half
    _PUNCT,							//	190	BE	¾	Fraction three quarters
    _PUNCT,							//	191	BF	¿	Inverted question mark
    0x0100 | _UPPER,				//	192	C0	À	Latin capital letter A with grave
    0x0100 | _UPPER,				//	193	C1	Á	Latin capital letter A with acute
    0x0100 | _UPPER,				//	194	C2	Â	Latin capital letter A with circumflex
    0x0100 | _UPPER,				//	195	C3	Ã	Latin capital letter A with tilde
    0x0100 | _UPPER,				//	196	C4	Ä	Latin capital letter A with diaeresis
    0x0100 | _UPPER,				//	197	C5	Å	Latin capital letter A with ring above
    0x0100 | _UPPER,				//	198	C6	Æ	Latin capital letter AE
    0x0100 | _UPPER,				//	199	C7	Ç	Latin capital letter C with cedilla
    0x0100 | _UPPER,				//	200	C8	È	Latin capital letter E with grave
    0x0100 | _UPPER,				//	201	C9	É	Latin capital letter E with acute
    0x0100 | _UPPER,				//	202	CA	Ê	Latin capital letter E with circumflex
    0x0100 | _UPPER,				//	203	CB	Ë	Latin capital letter E with diaeresis
    0x0100 | _UPPER,				//	204	CC	Ì	Latin capital letter I with grave
    0x0100 | _UPPER,				//	205	CD	Í	Latin capital letter I with acute
    0x0100 | _UPPER,				//	206	CE	Î	Latin capital letter I with circumflex
    0x0100 | _UPPER,				//	207	CF	Ï	Latin capital letter I with diaeresis
    0x0100 | _UPPER,				//	208	D0	Ð	Latin capital letter ETH
    0x0100 | _UPPER,				//	209	D1	Ñ	Latin capital letter N with tilde
    0x0100 | _UPPER,				//	210	D2	Ò	Latin capital letter O with grave
    0x0100 | _UPPER,				//	211	D3	Ó	Latin capital letter O with acute
    0x0100 | _UPPER,				//	212	D4	Ô	Latin capital letter O with circumflex
    0x0100 | _UPPER,				//	213	D5	Õ	Latin capital letter O with tilde
    0x0100 | _UPPER,				//	214	D6	Ö	Latin capital letter O with diaeresis
    _PUNCT,							//	215	D7	×	Multiplication sign
    0x0100 | _UPPER,				//	216	D8	Ø	Latin capital letter O with slash
    0x0100 | _UPPER,				//	217	D9	Ù	Latin capital letter U with grave
    0x0100 | _UPPER,				//	218	DA	Ú	Latin capital letter U with acute
    0x0100 | _UPPER,				//	219	DB	Û	Latin capital letter U with circumflex
    0x0100 | _UPPER,				//	220	DC	Ü	Latin capital letter U with diaeresis
    0x0100 | _UPPER,				//	221	DD	Ý	Latin capital letter Y with acute
    0x0100 | _UPPER,				//	222	DE	Þ	Latin capital letter THORN
    0x0100 | _LOWER,				//	223	DF	ß	Latin small letter sharp s - ess-zed
    0x0100 | _LOWER,				//	224	E0	à	Latin small letter a with grave
    0x0100 | _LOWER,				//	225	E1	á	Latin small letter a with acute
    0x0100 | _LOWER,				//	226	E2	â	Latin small letter a with circumflex
    0x0100 | _LOWER,				//	227	E3	ã	Latin small letter a with tilde
    0x0100 | _LOWER,				//	228	E4	ä	Latin small letter a with diaeresis
    0x0100 | _LOWER,				//	229	E5	å	Latin small letter a with ring above
    0x0100 | _LOWER,				//	230	E6	æ	Latin small letter ae
    0x0100 | _LOWER,				//	231	E7	ç	Latin small letter c with cedilla
    0x0100 | _LOWER,				//	232	E8	è	Latin small letter e with grave
    0x0100 | _LOWER,				//	233	E9	é	Latin small letter e with acute
    0x0100 | _LOWER,				//	234	EA	ê	Latin small letter e with circumflex
    0x0100 | _LOWER,				//	235	EB	ë	Latin small letter e with diaeresis
    0x0100 | _LOWER,				//	236	EC	ì	Latin small letter i with grave
    0x0100 | _LOWER,				//	237	ED	í	Latin small letter i with acute
    0x0100 | _LOWER,				//	238	EE	î	Latin small letter i with circumflex
    0x0100 | _LOWER,				//	239	EF	ï	Latin small letter i with diaeresis
    0x0100 | _LOWER,				//	240	F0	ð	Latin small letter eth
    0x0100 | _LOWER,				//	241	F1	ñ	Latin small letter n with tilde
    0x0100 | _LOWER,				//	242	F2	ò	Latin small letter o with grave
    0x0100 | _LOWER,				//	243	F3	ó	Latin small letter o with acute
    0x0100 | _LOWER,				//	244	F4	ô	Latin small letter o with circumflex
    0x0100 | _LOWER,				//	245	F5	õ	Latin small letter o with tilde
    0x0100 | _LOWER,				//	246	F6	ö	Latin small letter o with diaeresis
    _PUNCT,							//	247	F7	÷	Division sign
    0x0100 | _LOWER,				//	248	F8	ø	Latin small letter o with slash
    0x0100 | _LOWER,				//	249	F9	ù	Latin small letter u with grave
    0x0100 | _LOWER,				//	250	FA	ú	Latin small letter u with acute
    0x0100 | _LOWER,				//	251	FB	û	Latin small letter u with circumflex
    0x0100 | _LOWER,				//	252	FC	ü	Latin small letter u with diaeresis
    0x0100 | _LOWER,				//	253	FD	ý	Latin small letter y with acute
    0x0100 | _LOWER,				//	254	FE	þ	Latin small letter thorn
    0x0100 | _LOWER,				//	255	FF	ÿ	Latin small letter y with diaeresis
};

#undef _pwctype
const unsigned short* _pwctype = &_wctype[1];

const unsigned short * __cdecl __pwctype_func(void)
{
    return &_wctype[1];
}
