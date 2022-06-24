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
u8 *linAddr[ROWS];

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

// configurations
u8 color_chr = COL_WHITE;
u8 color_bkg = COL_SEMIBLUE;
u8 color_sl_chr = COL_WHITE;
u8 color_sl_bkg = COL_BLUE;
bool autowrap = true, bserases = false, cr_crlf = false, lf_crlf = false;
bool show_sl = true;

// color available to ANSI commands
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

// Special keys sequences
static char const *keysequence[] = {
    "\x1B[A",  // UP
    "\x1B[B",  // DOWN
    "\x1B[D",  // LEFT
    "\x1B[C",  // RIGHT
    "\x1B[H",  // HOME
    "\x1B[K",  // END
    "\x1B[OP", // F1
    "\x1B[OQ", // F2
    "\x1B[OR", // F3
    "\x1B[OS", // F4
    "\x1B[OT", // F5
    "\x1B[OU", // F6
    "\x1B[OV", // F7
    "\x1B[OW", // F8
    "\x1B[OX", // F9
    "\x1B[OY"  // F10
};

// Status line control
// .123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789
// MODE      BAUD      ID                                                 L=XX C=XX
#define SL_MODE 0
#define SL_BAUD 10
#define SL_ID   20
#define SL_LC   71
static int nlines = ROWS-1;

// local rotines
static void print_string(char *str);
static void advance_line(void);
static void scroll_up(int n);
static void update_sl_lc(void);
static void write_sl(int col, const char *str);

// Send a key, expanding sequences
void send_key (uint8_t ch)
{
  if (ch > 0x7F)
  {
    // special key
    char const *seq = keysequence[ch - 0x80];
    while (*seq)
    {
      put_tx(*seq);
      seq++;
    }
  }
  else if (ch != 0)
  {
    // normal key
    put_tx(ch);
  }
}

// Simulate reception of a key
void receive_key  (uint8_t ch)
{
  if (ch > 0x7F)
  {
    // special key
    char const *seq = keysequence[ch - 0x80];
    while (*seq)
    {
      put_rx(*seq);
      seq++;
    }
  }
  else if (ch != 0)
  {
    // normal key
    put_rx(ch);
  }
}

// Move cursor to home
void home() {
    csr.x = csr.y = 0;
}

// Move cursor to next line
// scroll up if at last line
static void advance_line() {
    if (csr.y < (ROWS-1)) {
        csr.y++;
    } else {
        scroll_up(1);
    }
}

// Clear screen
void cls() {
    cls(color_bkg, color_chr);
}

void cls(u8 clr_bkg, u8 clr_chr) {
    int end = TEXTSIZE;
    if (show_sl) {
        end -= 3* COLUMNS;
    }
	for (int i = 0; i < end; ) {
		TextBuf[i++] = ' ';
		TextBuf[i++] = clr_bkg;
    	TextBuf[i++] = clr_chr;
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
    while (l < nlines) {
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
    if (csr.y >= nlines) {
        csr.y = nlines-1;
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

// Scroll up screen n lines
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
void show_cursor(){
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
                    case 3:
                        // clear entire screen
                        cls();
                        home();
                        break;
                }
                break;
            case 'S':
            // Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
                n = esc_parameters[0];
                if (n == 0) {
                    n = 1;
                }
                if (n >= nlines) {
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
    home();
    init_sl();
    reset_escape_sequence();

    // Show cursor
    make_cursor_visible(true);
    show_cursor();
}

static char ident[] = "RPTerm v0.6  DQ";

// Initialize status line
void init_sl() {
    if (show_sl) {
        // clear status line
        for (int i = TEXTSIZE - 3*COLUMNS; i < TEXTSIZE; ) {
            TextBuf[i++] = ' ';
            TextBuf[i++] = color_sl_bkg;
            TextBuf[i++] = color_sl_chr;
        }

        // fill the fields
        update_sl_mode();
        write_sl(SL_BAUD, config_getbaud());
        write_sl(SL_ID, ident);
        update_sl_lc();
    }
}

// update terminal mode in status line
void update_sl_mode() {
    if (show_sl) {
        switch (term_mode) {
            case ONLINE:
                write_sl(SL_MODE, "ONLINE");
                break;
            case LOCAL:
                write_sl(SL_MODE, "LOCAL ");
                break;
            case CONFIG:
                write_sl(SL_MODE, "CONFIG");
                break;
        }
    }
}

// update cursor pos in status line
static void update_sl_lc() {
    if (show_sl) {
        char buf[10];
        sprintf(buf, "L=%02d C=%02d", csr.y+1, csr.x+1);
        write_sl(SL_LC, buf);
    }
}

// Write string to status line
static void write_sl (int col, const char *str) {
    uint8_t *pos = linAddr[nlines] + 3*col;
    for(int i=0; str[i] != '\0'; i++){
        *pos = str[i];
        pos += 3;
    }
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
            if (csr.x < (COLUMNS-1)) {
                csr.x++;
            } else if (autowrap) {
                csr.x=0;
                advance_line();
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
                    if (bserases) {
                        slip_character(' ', csr.x, csr.y);
                    }
                    break; 
                case HT:
                    if (csr.x < (COLUMNS-7)) {
                        csr.x = (csr.x + 8) & 0xF8;
                    }
                    break;
                case CR:
                    csr.x = 0;
                    if (cr_crlf) {
                        advance_line();
                    }
                    break; 
                case LF:
                    if (lf_crlf) {
                        csr.x = 0;
                    }
                    advance_line();
                    break; 
                case FF:
                    cls(); 
                    home();
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

    update_sl_lc();
    show_cursor();
}