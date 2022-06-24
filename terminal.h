/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * Definitions for terminal emulation
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

#ifndef _TERMINAL_H
#define _TERMINAL_H

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

extern u8 *linAddr[TEXTH];

extern u8 color_chr, color_bkg, color_sl_chr, color_sl_bkg;
extern bool autowrap, bserases, cr_crlf, lf_crlf;
extern bool show_sl;

extern void terminal_init(void);
extern void terminal_handle_rx(u8 chrx);
extern void send_key(uint8_t ch);
extern void receive_key(uint8_t ch);

extern void cls(void);
extern void cls(u8 clr_bkg, u8 clr_chr);
extern void home(void);
extern void show_cursor(void);

extern void init_sl(void);
extern void update_sl_mode();

#endif
