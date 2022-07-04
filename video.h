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


#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef _VIDEO_H

#define VIDEO_H

typedef struct scrpos {
  int x;
  int y;
} scrpos;

// Cursor position
extern scrpos csr;

// The screen
extern u8 TextBuf[TEXTSIZE];

// Status line control
extern bool show_sl;

// Initialization
extern void video_init(void);

// Cursor control
extern void home(void);
extern void show_cursor(void);
extern void advance_line(void);
extern void make_cursor_visible(bool v);
extern void constrain_cursor_values(void);
extern void clear_cursor(void);

// Clear screen
extern void cls(void);
extern void cls(u8 clr_bkg, u8 clr_chr);
extern void clear_line_from_cursor(void);
extern void clear_line_to_cursor(void);
extern void clear_entire_line(void);
extern void clear_screen_from_csr(void);
extern void clear_screen_to_csr(void);
extern void clear_sl(void);

// Status line control
extern void show_statusline (bool show);

// Scroll screen
extern void scroll_up(int n);

// Write char and string
extern void slip_character(unsigned char ch);
extern void write_sl (int col, const char *str);
extern void draw_box(int l, int c, int nl, int nc);
extern void write_str(int l, int c, const char *str);
extern void write_str_atr(int l, int c, const char *str, uint8_t clr_bkg, uint8_t clr_chr);

#endif
