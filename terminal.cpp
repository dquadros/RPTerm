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

extern u8 TextBuf[TEXTSIZE];

// Screen dimensions
#define COLUMNS     80
#define ROWS        60

// ASCII chars
#define SPC         0x20
#define ESC         0x1b
#define DEL         0x7f
#define BSP         0x08
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

// Cursor
typedef struct point {
  int x;
  int y;
} point;
struct point csr = {0,0};
struct point saved_csr = {0,0};


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

// Cursor control
static void make_cursor_visible(bool v){
    cursor_visible = v;
}

static void clear_escape_parameters(){
    for(int i=0;i<MAX_ESC_PARAMS;i++){
        esc_parameters[i]=0;
    }
    esc_parameter_count = 0;
}

static void reset_escape_sequence(){
    clear_escape_parameters();
    esc_state=ESC_READY;
    esc_c1=0;
    esc_final_byte=0;
    parameter_q=false;
}




static void constrain_cursor_values(){
    if(csr.x<0) csr.x=0;
    if(csr.x>=COLUMNS) csr.x=COLUMNS-1;    
    if(csr.y<0) csr.y=0;
    if(csr.y>=ROWS) csr.y=ROWS-1;    
}

// Put char in the screen memory, taking in account the reverse flag
static void slip_character(unsigned char ch,int x,int y){
    u8 *p = &TextBuf[3*(COLUMNS*y + x)];
    *p++ = ch;
    *p++ = color_bkg;
    *p++ = color_chr;
}

static void store_character(unsigned char ch,int x,int y){
    TextBuf[3*(COLUMNS*y + x)] = ch;
}

static unsigned char slop_character(int x,int y){
    return TextBuf[3*(COLUMNS*y + x)];
}

static void shuffle(){
    memmove (TextBuf+3*COLUMNS, TextBuf, TEXTSIZE-3*COLUMNS);
    for (int i = TEXTSIZE - 3*COLUMNS; i < TEXTSIZE; ) {
		TextBuf[i++] = ' ';
		TextBuf[i++] = color_bkg;
		TextBuf[i++] = color_chr;
    }
}

// Show cursor (if visible)
static void show_cursor(){
    // save character under the cursor
    chr_under_csr = slop_character(csr.x,csr.y);

    // nothing to do if cursor is invisible
    if(!cursor_visible) {
        return;
    }

    store_character('_',csr.x,csr.y);
}

// Restore the character under the cursor
// (if it was saved)
static void clear_cursor(){
    if (chr_under_csr != 0xFF) {
        store_character(chr_under_csr,csr.x,csr.y);
    }
}


static void clear_line_from_cursor(){
    // TODO
}

static void clear_line_to_cursor(){
    // TODO
}

static void clear_entire_line(){
    // TODO
}


static void clear_screen_from_csr(){
    // TODO
}

static void clear_screen_to_csr(){
    // TODO
}


static void esc_sequence_received(){
/*
// these should now be populated:
    static int esc_parameters[MAX_ESC_PARAMS];
    static int esc_parameter_count;
    static unsigned char esc_c1;
    static unsigned char esc_final_byte;       
*/


int n,m; 
if(esc_c1=='['){
    // CSI
    switch(esc_final_byte){
    case 'H':
        // Moves the cursor to row n, column m
        // The values are 1-based, and default to 1
        
        n = esc_parameters[0];
        m = esc_parameters[1];
        n--; 
        m--;

        // these are zero based
        csr.x = m;
        csr.y = n;
        constrain_cursor_values();
    break;

    case 'h':
        if(parameter_q && esc_parameters[0]==25){
            // show csr
            make_cursor_visible(true);
        }
    break;
    case 'l':
        if(parameter_q && esc_parameters[0]==25){
            // hide csr
            make_cursor_visible(false);
        }
    break;


    case 'm':
        //SGR
        // Sets colors and style of the characters following this code
        //TODO: allows multiple paramters
        switch(esc_parameters[0]){
            case 0:
            // reset / normal
            color_chr = COL_WHITE;
            color_bkg = COL_SEMIBLUE;
        break;
            case 7:
            color_chr = COL_SEMIBLUE;
            color_bkg = COL_WHITE;
        break;
        }
    break;

    case 's':
        // save cursor position
        saved_csr.x = csr.x;
        saved_csr.y = csr.y;
    break;
    case 'u':
        // move to saved cursor position
        csr.x = saved_csr.x;
        csr.y = saved_csr.y;
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


    case 'A':
    // Cursor Up
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        csr.y -= n;
        constrain_cursor_values();
    break;
    case 'B':
    // Cursor Down
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;
        csr.y += n;
        constrain_cursor_values();  // todo: should possibly do a scroll up?
    break;
    case 'C':
    // Cursor Forward
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;   
        csr.x += n;
        constrain_cursor_values();
    break;
    case 'D':
    // Cursor Backward
    //Moves the cursor n (default 1) cells
        n = esc_parameters[0];
        if(n==0)n=1;    
        csr.x -= n;
        constrain_cursor_values();
    break;
    case 'S':
    // Scroll whole page up by n (default 1) lines. New lines are added at the bottom. (not ANSI.SYS)
        n = esc_parameters[0];
        if(n==0)n=1;
        for(int i=0;i<n;i++){
            shuffle();
        }
    break;

    // MORE

 

    }





}
else{
    // ignore everything else
}


// our work here is done
reset_escape_sequence();

}


static char ident[] = "\r\n\r\nRPTerm 0.0  DQ\r\n";

void terminal_init(){
    cls();
    reset_escape_sequence();
    print_string(ident);

    // print cursor
    make_cursor_visible(true);
    show_cursor();  // turns on
}

static void print_string(char *str){
    for(int i=0; str[i] != '\0'; i++){
        terminal_handle_rx(str[i]);
    }
}


void terminal_handle_rx(u8 chrx) {

    // handle escape sequences
    if(esc_state != ESC_READY){
        switch(esc_state){
            case ESC_ESC_RECEIVED:
                // waiting on c1 character
                if(chrx>='N' && chrx<'_'){ 
                    // 0x9B = CSI, that's the only one we're interested in atm
                    // the others are 'Fe Escape sequences'
                    // usually two bytes, ie we have them already. 
                    if(chrx=='['){    // ESC+[ =  0x9B){
                        // move forward

                        esc_c1 = chrx;
                        esc_state=ESC_PARAMETER_READY;
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
                // waiting on parameter character, semicolon or final byte
                if(chrx>='0' && chrx<='9'){ 
                    // parameter value
                    if(esc_parameter_count<MAX_ESC_PARAMS){
                        unsigned char digit_value = chrx - 0x30; // '0'
                        esc_parameters[esc_parameter_count] *= 10;
                        esc_parameters[esc_parameter_count] += digit_value;
                    }
                    
                }
                else if(chrx==';'){ 
                    // move to next param
                    esc_parameter_count++;
                    if(esc_parameter_count>MAX_ESC_PARAMS) esc_parameter_count=MAX_ESC_PARAMS;
                }
                else if(chrx=='?'){ 
                    parameter_q=true;
                }
                else if(chrx>=0x40 && chrx<0x7E){ 
                    // final byte. Log and handle
                    esc_final_byte = chrx;
                    esc_sequence_received();
                }
                else{
                    // unexpected value, undefined
                }
                break; 
        }




    }
    else{
        // regular characters - 
        if(chrx>=0x20 && chrx<0x7f){  
  
            slip_character(chrx,csr.x,csr.y);
            csr.x++;

            if(csr.x>=COLUMNS){
                csr.x=0;
                if(csr.y==ROWS){
                    shuffle();
                }
                else{
                    csr.y++;
                }
            }


        }
        //is it esc?
        else if(chrx==0x1B){
            esc_state=ESC_ESC_RECEIVED;
        }
        else{
            // return, backspace etc
            switch (chrx){
                case BSP:
                if(csr.x>0){
                    csr.x--;
                }
                break; 
                case LF:
                
                    if(csr.y==ROWS-1){   // visiblerows is the count, csr is zero based
                        shuffle();
                    }
                    else{
                    csr.y++;
                    }
                break; 
                case CR:
                    csr.x=0;

                break; 
                case FF:
                    cls(); 
                    csr.x=0; csr.y=0;

                break; 
            }

        }

    } // not esc sequence


}