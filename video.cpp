/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * Video manipulation routines
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

// semigraphic chars
#define CHAR_UL    0x1C
#define CHAR_UR    0x19
#define CHAR_DL    0x16
#define CHAR_DR    0x13
#define CHAR_HORIZ 0x15
#define CHAR_VERT  0x1A

// Screen dimensions
#define COLUMNS     TEXTW
#define ROWS        TEXTH
static int nlines = ROWS-1;

// The screen
extern u8 TextBuf[TEXTSIZE];
static u8 *linAddr[ROWS];

// screen control
bool show_sl = true;
static bool cursor_visible;
static unsigned char chr_under_csr = 0xFF;

// Cursor
struct scrpos csr = {0,0};

// Video initialization
void video_init() {
    // Calcule starting address for the lines
    u8 *p = TextBuf;
    for (int i = 0; i < ROWS; i++) {
        linAddr[i] =  p;
        p += TEXTWB;
    }

    // Init screen
    cls();
    home();
    init_sl();
}


// Move cursor to home
void home() {
    csr.x = csr.y = 0;
}

// Move cursor to next line
// scroll up if at last line
void advance_line() {
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
void clear_line_from_cursor() {
    u8* p= linAddr[csr.y] + 3* csr.x;
    for (int i = csr.x; i < COLUMNS; i++) {
		*p++ = ' ';
		*p++ = color_bkg;
    	*p++ = color_chr;
    }
}

// clear line from start of line to cursor
void clear_line_to_cursor() {
    u8* p= linAddr[csr.y];
    for (int i = 0; i <= csr.x; i++) {
		*p++ = ' ';
		*p++ = color_bkg;
    	*p++ = color_chr;
    }
}

// clear line
void clear_entire_line() {
    u8* p= linAddr[csr.y];
    for (int i = 0; i < COLUMNS; i++) {
		*p++ = ' ';
		*p++ = color_bkg;
    	*p++ = color_chr;
    }
}

// clear screen from cursor to end of screen
void clear_screen_from_csr() {
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
void clear_screen_to_csr() {
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
void make_cursor_visible(bool v) {
    cursor_visible = v;
}

// Check that cursor in valid
void constrain_cursor_values() {
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

// Put char in the screen memory at cursor, taking in account the color
void slip_character(unsigned char ch) {
    u8 *p = linAddr[csr.y]+3*csr.x;
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
void scroll_up(int n) {
    int size = n*3*COLUMNS;
    memmove (TextBuf, TextBuf+size, TEXTSIZE-size);
    for (int i = TEXTSIZE - size; i < TEXTSIZE; ) {
		TextBuf[i++] = ' ';
		TextBuf[i++] = color_bkg;
		TextBuf[i++] = color_chr;
    }
}

// Show cursor (if visible)
void show_cursor() {
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
void clear_cursor() {
    if (chr_under_csr != 0xFF) {
        put_character(chr_under_csr,csr.x,csr.y);
    }
}

// Write string to status line
void write_sl (int col, const char *str) {
    uint8_t *pos = linAddr[nlines] + 3*col;
    for(int i=0; str[i] != '\0'; i++){
        *pos = str[i];
        pos += 3;
    }
}

// Write string
void write_str(int l, int c, const char *str) {
    uint8_t *pos = linAddr[l] + 3*c;
    for(int i=0; str[i] != '\0'; i++){
        *pos = str[i];
        pos += 3;
    }
}

// Write string, with atributtes
void write_str_atr(int l, int c, const char *str, uint8_t clr_bkg, uint8_t clr_chr) {
    uint8_t *pos = linAddr[l] + 3*c;
    for(int i=0; str[i] != '\0'; i++){
        *pos++ = str[i];
        *pos++ = clr_bkg;
        *pos++ = clr_chr;
    }
}


// Draw a box on the screen
void draw_box(int l, int c, int nl, int nc) {
    uint8_t *pos = linAddr[l] + 3*c;
    *pos = CHAR_UL; 
    pos += 3;
    for (int i = 2; i < nc; i++) {
        *pos = CHAR_HORIZ; 
        pos += 3;
    }
    *pos = CHAR_UR;
    for (int i = 2; i < nl; i++) {
        pos = linAddr[l+i-1] + 3*c;
        *pos = CHAR_VERT;
        pos += 3*(nc - 1);
        *pos = CHAR_VERT;
    }
    pos = linAddr[l+nl-1] + 3*c;
    *pos = CHAR_DL; 
    pos += 3;
    for (int i = 2; i < nc; i++) {
        *pos = CHAR_HORIZ;
        pos += 3;
    }
    *pos = CHAR_DR;
}

// control the status line
void show_statusline (bool show) {
    show_sl = show;
    nlines = show? ROWS-1 : ROWS;
}

void clear_sl() {
    for (int i = TEXTSIZE - 3*COLUMNS; i < TEXTSIZE; ) {
        TextBuf[i++] = ' ';
        TextBuf[i++] = color_sl_bkg;
        TextBuf[i++] = color_sl_chr;
    }
}