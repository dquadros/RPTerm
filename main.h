
// ****************************************************************************
//
//                                 Main code
//
// ****************************************************************************

#ifndef _MAIN_H
#define _MAIN_H

#define WIDTH	640	// screen width in pixels
#define HEIGHT	480	// screen height

// This font will give 60 lines of text
//#define FONT    FontBold8x8
//#define FONTW	8	// font width
//#define FONTH	8	// font height

// This font will give 30 lines of text
#define FONT    FontBold8x16
#define FONTW	8	// font width
#define FONTH	16	// font height

// Text sizes
// Each character in screen uses three bytes in memory
// (character + backgound coler + foreground color, format GF_ATEXT)
#define TEXTW	(WIDTH/FONTW)   // text width (=80)
#define TEXTH	(HEIGHT/FONTH)  // text height (=60)
#define TEXTWB	(TEXTW*3)       // text width byte (=240)
#define TEXTSIZE (TEXTWB*TEXTH) // text box size in bytes (=9600)

extern void beep();

#endif // _MAIN_H
