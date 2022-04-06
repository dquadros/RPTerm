/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * Terminal emulation
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "include.h"

// Screen dimensions
#define COLUMNS     TEXTW
#define ROWS        TEXTH

// The screen
extern u8 TextBuf[TEXTSIZE];
static u8 *linAddr[ROWS];

// ASCII chars
#define SPC         0x20
#define ESC         0x1b
#define DEL         0x7f
#define BEL         0x07
#define BSP         0x08
#define HT          0x09
#define LF          0x0a
#define CR          0x0d 
#define FF          0x0c

// escape sequence state
#define ESC_READY               0
#define ESC_ESC_RECEIVED        1
#define ESC_PARAMETER_READY     2

#define MAX_ESC_PARAMS          5
static int esc_state = ESC_READY;
static int esc_parameters[MAX_ESC_PARAMS];
static bool parameter_q;
static int esc_parameter_count;
static unsigned char esc_c1;
static unsigned char esc_final_byte;

// screen control
static bool cursor_visible;
static unsigned char chr_under_csr = 0xFF;
static u8 color_chr = COL_WHITE;
static u8 color_bkg = COL_SEMIBLUE;

static const u8 ansi_pallet[] = {
    COL_BLACK, COL_RED, COL_GREEN, COL_YELLOW, COL_BLUE, COL_MAGENTA, COL_CYAN, COL_WHITE
};

// Cursor
typedef struct scrpos {
  int x;
  int y;
} scrpos;
struct scrpos csr = {0,0};
struct scrpos saved_csr = {0,0};


// local rotines
static void print_string(char *str);


// Clear screen
static void cls() {
	for (int i = 0; i < TEXTSIZE; ) {
		TextBuf[i++] = ' ';
		TextBuf[i++] = color_bkg;
    	TextBuf[i++] = color_chr;
	}
}

// clear line from cursor to end of line
static void clear_line_from_cursor(){
    u8* p= linAddr[csr.y] + 3* csr.x;
    for (int i = csr.x; i < COLUMNS; i++) {
		*p++ = ' ';
		*p++ = color_bkg;
    	*p++ = color_chr;
    }
}

// clear line from start of line to cursor
static void clear_line_to_cursor(){
    u8* p= linAddr[csr.y];
    for (int i = 0; i <= csr.x; i++) {
		*p++ = ' ';
		*p++ = color_bkg;
    	*p++ = color_chr;
    }
}

// clear line
static void clear_entire_line(){
    u8* p= linAddr[csr.y];
    for (int i = 0; i < COLUMNS; i++) {
		*p++ = ' ';
		*p++ = color_bkg;
    	*p++ = color_chr;
    }
}

// clear screen from cursor to end of screen
static void clear_screen_from_csr(){
    int l = csr.y;
    while (l < ROWS) {
        int start = (l == csr.y)? csr.x : 0;
        u8 *p = linAddr[l] + 3*start;
        for (int c = start; c < COLUMNS; c++) {
            *p++ = ' ';
            *p++ = color_bkg;
            *p++ = color_chr;
        }
        l++;
    }
}

// clear screen from start of screen to cursor
static void clear_screen_to_csr(){
    int l = 0;
    while (l <= csr.y) {
        int end = (l == csr.y)? csr.x+1 : COLUMNS;
        u8 *p = linAddr[l];
        for (int c = 0; c < end; c++) {
            *p++ = ' ';
            *p++ = color_bkg;
            *p++ = color_chr;
        }
        l++;
    }
}


// Cursor control
static void make_cursor_visible(bool v){
    cursor_visible = v;
}

// Clear escape sequence parameters
static void clear_escape_parameters(){
    for(int i=0; i<MAX_ESC_PARAMS; i++){
        esc_parameters[i] = 0;
    }
    esc_parameter_count = 0;
}

// Reset escape sequence processing
static void reset_escape_sequence(){
    clear_escape_parameters();
    esc_state = ESC_READY;
    esc_c1 = 0;
    esc_final_byte = 0;
    parameter_q = false;
}

// Check that cursor in valid
static void constrain_cursor_values(){
    if (csr.x < 0) {
        csr.x = 0;
    }
    if (csr.x >= COLUMNS) {
        csr.x = COLUMNS-1;
    }
    if (csr.y < 0) {
        csr.y=0;
    }
    if (csr.y >= ROWS) {
        csr.y = ROWS-1;
    }
}

// Put char in the screen memory, taking in account the color
static void slip_character(unsigned char ch,int x,int y){
    u8 *p = linAddr[y]+3*x;
    *p++ = ch;
    *p++ = color_bkg;
    *p++ = color_chr;
}

// Put char in screen memory, without changing color
static void put_character(unsigned char ch,int x,int y){
    u8 *p = linAddr[y]+3*x;
    *p = ch;
}

// Get char from screen
static unsigned char get_character(int x,int y){
    u8 *p = linAddr[y]+3*x;
    return *p;
}

// Scroll screen n lines
// TODO: change rendering to use linAddr and just move pointers
static void scroll_up(int n) {
    int size = n*3*COLUMNS;
    memmove (TextBuf, TextBuf+size, TEXTSIZE-size);
    for (int i = TEXTSIZE - size; i < TEXTSIZE; ) {
		TextBuf[i++] = ' ';
		TextBuf[i++] = color_bkg;
		TextBuf[i++] = color_chr;
    }
}

// Show cursor (if visible)
static void show_cursor(){
    // save character under the cursor
    chr_under_csr = get_character(csr.x,csr.y);

    // nothing to do if cursor is invisible
    if(!cursor_visible) {
        return;
    }

    put_character('_',csr.x,csr.y);
}

// Restore the character under the cursor
// (if it was saved)
static void clear_cursor(){
    if (chr_under_csr != 0xFF) {
        put_character(chr_under_csr,csr.x,csr.y);
    }
}

// Treat ESC sequence received
/*
// these should now be populated:
    static int esc_parameters[MAX_ESC_PARAMS];
    static int esc_parameter_count;
    static unsigned char esc_c1;
    static unsigned char esc_final_byte;       
*/
static void esc_sequence_received(){

    int n,m; 

    if (esc_c1 == '[') {
        // CSI
        switch(esc_final_byte){
            case 'A':
            // Cursor Up
            //Moves the cursor n (default 1) cells
                n = esc_parameters[0];
                if (n == 0) {
                    n = 1;
                }
                csr.y -= n;
                constrain_cursor_values();
                break;
            case 'B':
            // Cursor Down
            //Moves the cursor n (default 1) cells
                n = esc_parameters[0];
                if (n == 0) {
                    n = 1;
                }
                csr.y += n;
                constrain_cursor_values();
                break;
            case 'C':
            // Cursor Forward
            //Moves the cursor n (default 1) cells
                n = esc_parameters[0];
                if (n == 0) {
                    n = 1;
                }
                csr.x += n;
                constrain_cursor_values();
                break;
            case 'D':
            // Cursor Backward
            //Moves the cursor n (default 1) cells
                n = esc_parameters[0];
                if (n == 0) {
                    n = 1;
                }
                csr.x -= n;
                constrain_cursor_values();
                break;
            case 'H':
            // Moves the cursor to row n, column m
            // The parameters are 1-based, and default to 1
            // these are zero based
                csr.x = esc_parameters[0]-1;
                csr.y = esc_parameters[1]-1;
                constrain_cursor_values();
                break;
            case 'K':
            // Erases part of the line. If n is 0 (or missing), clear from cursor to the end of the line. 
            // If n is 1, clear from cursor to beginning of the line. If n is 2, clear entire line. 
            // Cursor position does not change.
                switch(esc_parameters[0]){
                    case 0:
                        // clear from cursor to the end of the line
                        clear_line_from_cursor();
                        break;
                    case 1:
                        // clear from cursor to beginning of the line
                        clear_line_to_cursor();
                        break;
                    case 2:
                        // clear entire line
                        clear_entire_line();
                        break;
                }
                break;
            case 'J':
            // Clears part of the screen. If n is 0 (or missing), clear from cursor to end of screen. 
            // If n is 1, clear from cursor to beginning of the screen. If n is 2, clear entire screen 
            // (and moves cursor to upper left on DOS ANSI.SYS). 
            // If n is 3, clear entire screen and delete all lines saved in the scrollback buffer 
            // (this feature was added for xterm and is supported by other terminal applications).
                switch(esc_parameters[0]){
                    case 0:
                        // clear from cursor to end of screen
                        clear_screen_from_csr();
                        break;
                    case 1:
                        // clear from cursor to beginning of the screen
                        clear_screen_to_csr();
                        break;
                    case 2:
                        // clear entire screen
                        cls();
                        csr.x=0; csr.y=0;
                        break;
                    case 3:
                        // clear entire screen
                        cls();
                        csr.x=0; csr.y=0;
                        break;
                }
                break;
            case 'S':
            // Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
                n = esc_parameters[0];
                if (n == 0) {
                    n = 1;
                }
                if (n >= ROWS) {
                    cls();
                } else {
                    scroll_up(n);
                }
                break;
            case 'h':
                if (parameter_q && (esc_parameters[0]==25)) {
                    // show csr
                    make_cursor_visible(true);
                }
                break;
            case 'l':
                if (parameter_q && (esc_parameters[0]==25)) {
                    // hide csr
                    make_cursor_visible(false);
                }
                break;
            case 'm':
                //SGR
                // Sets colors and style of the characters following this code
                //TODO: implement color selection
                n = esc_parameters[0];
                if (n == 0) {
                    // reset / normal
                    // TODO: configure normal colors
                    color_chr = COL_WHITE;
                    color_bkg = COL_SEMIBLUE;
                } else if (n == 7) {
                    // reverse
                    u8 aux = color_chr;
                    color_chr = color_bkg;
                    color_bkg = aux;
                } else if ((n >= 30) && (n <= 37)) {
                    // set foreground to ANSI color
                    color_chr = ansi_pallet[n-30];
                } else if ((n == 38) && (esc_parameters[1] == 5)) {
                    // set foreground to rgb colot
                    color_chr = esc_parameters[2] & 0xFF;
                } else if ((n >= 40) && (n <= 47)) {
                    // set background to ANSI color
                    color_bkg = ansi_pallet[n-40];
                } else if ((n == 48) && (esc_parameters[1] == 5)) {
                    // set background to rgb colot
                    color_bkg = esc_parameters[2] & 0xFF;
                }
                break;
            case 'u':
            // move to saved cursor position
                csr.x = saved_csr.x;
                csr.y = saved_csr.y;
                break;
            case 's':
            // save cursor position
                saved_csr.x = csr.x;
                saved_csr.y = csr.y;
                break;
        }
    }
    else {
        // ignore everything else
    }

    // our work here is done
    reset_escape_sequence();
}

static char ident[] = "RPTerm v0.0  DQ";

// Terminal emulation initialization
void terminal_init(){

    // Calcule starting address for the lines
    u8 *p = TextBuf;
    for (int i = 0; i < ROWS; i++) {
        linAddr[i] =  p;
        p += TEXTWB;
    }

    // Init screen and emulation state
    cls();
    reset_escape_sequence();

    // Print identification
    csr.x = (COLUMNS-strlen(ident))/2;
    print_string(ident);
    csr.x = 0;
    csr.y = 2;

    // Show cursor
    make_cursor_visible(true);
    show_cursor();
}

// Aux rotine to print a message
static void print_string(char *str){
    for(int i=0; str[i] != '\0'; i++){
        terminal_handle_rx(str[i]);
    }
}

// Collect escape sequence info
void collect_sequence(u8 chrx) {
    // waiting on parameter character, semicolon or final byte
    if((chrx >= '0') && (chrx <= '9')) { 
        // parameter value
        if(esc_parameter_count < MAX_ESC_PARAMS) {
            esc_parameters[esc_parameter_count] *= 10;
            esc_parameters[esc_parameter_count] += chrx - 0x30;
        }
    } 
    else if (chrx == ';') { 
        // move to next param
        if (esc_parameter_count < MAX_ESC_PARAMS) {
            esc_parameter_count++;
        }
    }
    else if (chrx == '?') { 
        parameter_q=true;
    }
    else if ((chrx >= 0x40) && (chrx < 0x7E)) { 
        // final byte, register and handle
        esc_final_byte = chrx;
        esc_sequence_received();
    }
    else{
        // unexpected value, just ignore
    }
}

// Handle received char
void terminal_handle_rx(u8 chrx) {

    clear_cursor();

    // handle escape sequences
    if (esc_state == ESC_READY) {
        if ((chrx >= 0x20) && (chrx < 0x7f)) {  
            // regular characters
            slip_character(chrx,csr.x,csr.y);
            // advance cursor
            if (++csr.x >= COLUMNS) {
                csr.x=0;
                if (++csr.y == ROWS) {
                    csr.y--;    // stay in last line
                    scroll_up(1);
                }
            }
        }
        else if (chrx == ESC) {
            esc_state=ESC_ESC_RECEIVED;
        }
        else {
            // control characters
            switch (chrx) {
                case BEL:
                    beep();
                    break;
                case BSP:
                    if(csr.x > 0) {
                        csr.x--;
                    }
                    break; 
                case HT:
                    if (csr.x < (COLUMNS-7)) {
                        csr.x = (csr.x + 8) & 0xF8;
                    }
                    break;
                case LF:
                    if (++csr.y == ROWS) {
                        csr.y--;    // stay in last line
                        scroll_up(1);
                    }
                    break; 
                case CR:
                    csr.x = 0;
                    break; 
                case FF:
                    cls(); 
                    csr.x = 0; 
                    csr.y = 0;
                    break; 
            }
        }
     } else {
        switch(esc_state){
            case ESC_ESC_RECEIVED:
                // waiting on c1 character
                if ((chrx >= 'N') && (chrx < '_')) { 
                    // 0x9B = CSI, that's the only one we're interested in atm
                    // the others are 'Fe Escape sequences'
                    // usually two bytes, ie we have them already. 
                    if(chrx=='[') {    // ESC+[ =  0x9B){
                        // move forward
                        esc_c1 = chrx;
                        esc_state = ESC_PARAMETER_READY;
                        clear_escape_parameters();
                    }
                    // other type Fe sequences go here
                    else{
                        // for now, do nothing
                        reset_escape_sequence();
                    }
                }
                else{
                    // unrecognised character after escape. 
                    reset_escape_sequence();
                }
                break; 
            case ESC_PARAMETER_READY:
                collect_sequence(chrx);
                break; 
        }
    }

    show_cursor();
}