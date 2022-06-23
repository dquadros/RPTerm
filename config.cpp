/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * Config screen
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

// Configuration screen colors
static u8 color_cfg_chr = COL_WHITE;
static u8 color_cfg_bkg = COL_SEMIGREEN;

// indexes of current serial configuration
static int baud, fmt;

// config field definition
typedef enum { FLD_BOOL, FLD_OPT, FLD_COLOR } FLD_TYPE;
typedef struct {
    int l;
    int c;
    const char *name;
    FLD_TYPE type;
    void *value;
    const char **options;
} FLD_DEF;

// options for the fields
static const char *opt_baud[] = { "9600  ", "19200 ", "38400 ", "57600 ", "115200", NULL };
static const char *opt_fmt[] = { "7E1", "7O1", "8N1", NULL };
static const char *opt_yn[] = { "YES", "NO ", NULL };

// config screen fields
static FLD_DEF fields[] = {
    { 3, 10, "baud", FLD_OPT, &baud,  opt_baud },
    { 4, 10, "format", FLD_OPT, &fmt,  opt_fmt },
    { 8, 15, "autowrap", FLD_BOOL, &autowrap, opt_yn },
    { 9, 15, "BS erases", FLD_BOOL, &bserases, opt_yn },
    { 10, 15, "CR = CR LF", FLD_BOOL, &cr_crlf, opt_yn },
    { 11, 15, "LF = CR LF", FLD_BOOL, &lf_crlf, opt_yn },
    {12, 15, "Status Line", FLD_BOOL, &show_sl, opt_yn },
    { 16, 14, "Screen Bkg", FLD_COLOR, &color_bkg, NULL },
    { 17, 14, "Screen Chr", FLD_COLOR, &color_chr, NULL },
    { 18, 14, "Status Bkg", FLD_COLOR, &color_sl_bkg, NULL },
    { 19, 14, "Status Chr", FLD_COLOR, &color_sl_chr, NULL }
};
#define NFIELDS (sizeof(fields)/sizeof(FLD_DEF))

// current field
static uint curfield;

// Local rotines
static void config_write(int l, int c, const char *str);
static void config_write_atr(int l, int c, const char *str, uint8_t clr_bkg, uint8_t clr_chr);
static void draw_box(int l, int c, int nl, int nc);
static void label_field(FLD_DEF *fld);
static void update_field(FLD_DEF *fld, bool selected);

// Enter local configuration mode
void config_enter() {
    cls(color_cfg_bkg, color_cfg_chr);
    config_write(0, 0, "TERMINAL CONFIGURATION (ESC to exit)");
    curfield = 0;
    // draw boxes
    draw_box(1, 0, 5, TEXTW);
    draw_box(6, 0, 8, TEXTW);
    draw_box(14, 0, 7, TEXTW);
    // write titles
    config_write(2, 2, "SERIAL");
    config_write(7, 2, "TERMINAL EMULATION");
    config_write(15, 2, "COLORS");
    // draw fields
    for (uint ifld = 0; ifld < NFIELDS; ifld++) {
        label_field(&fields[ifld]);
        update_field(&fields[ifld], ifld == curfield);
    }
}

// Write string to configuration screen
static void config_write(int l, int c, const char *str) {
    uint8_t *pos = linAddr[l] + 3*c;
    for(int i=0; str[i] != '\0'; i++){
        *pos = str[i];
        pos += 3;
    }
}

// Write string to configuration screen, with atributtes
static void config_write_atr(int l, int c, const char *str, uint8_t clr_bkg, uint8_t clr_chr) {
    uint8_t *pos = linAddr[l] + 3*c;
    for(int i=0; str[i] != '\0'; i++){
        *pos++ = str[i];
        *pos++ = clr_bkg;
        *pos++ = clr_chr;
    }
}

// Draw a box on the screen
static void draw_box(int l, int c, int nl, int nc) {
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

// Label a field
//   name: x
//         ^ c
static void label_field(FLD_DEF *fld) {
    int len = strlen(fld->name);
    config_write (fld->l, fld->c - len - 2, fld->name);
    config_write (fld->l, fld->c - 2, ":");
}

// Write a field value
static void update_field(FLD_DEF *fld, bool selected) {
    switch (fld->type)
    {
        case FLD_OPT: {
                 int opt = *((int *) fld->value);
                if (selected) {
                    config_write_atr(fld->l, fld->c, fld->options[opt],
                        color_cfg_chr, color_cfg_bkg);
                } else {
                    config_write_atr(fld->l, fld->c, fld->options[opt],
                        color_cfg_bkg, color_cfg_chr);
                }
            }
            break;
        
        case FLD_BOOL: {
                bool bval = *((bool *) fld->value);
                int val = bval? 1 : 0;
                if (selected) {
                    config_write_atr(fld->l, fld->c, fld->options[val],
                        color_cfg_chr, color_cfg_bkg);
                } else {
                    config_write_atr(fld->l, fld->c, fld->options[val],
                        color_cfg_bkg, color_cfg_chr);
                }
            }
            break;
        
        case FLD_COLOR: {
                uint8_t color = *((uint8_t *) fld->value);
                if (selected) {
                    config_write(fld->l, fld->c, "[ ]");
                } else {
                    config_write(fld->l, fld->c, "   ");
                }
                config_write_atr(fld->l, fld->c+1, " ", color, 0);
            }
            break;
        
        default:
            break;
    }
}

// Handle keys in config screen
void config_key(u8 key){
    FLD_DEF *fld = &fields[curfield];
    if (key == KEY_DWN) {
        update_field(fld, false);
        if (++curfield == NFIELDS) {
            curfield = 0;
        }
        fld = &fields[curfield];
        update_field(fld, true);
    } else if (key == KEY_UP) {
        update_field(fld, false);
        if (curfield-- == 0) {
            curfield = NFIELDS-1;
        }
        fld = &fields[curfield];
        update_field(fld, true);
    } else {
        switch (fld->type) {
            case FLD_OPT: {
                int opc = *((int *) fld->value);
                int nopc = 0;
                while (fld->options[nopc] != NULL) {
                    nopc++;
                }
                if ((key == ' ' ) || (key == '+')) {
                    if (++opc == nopc) {
                        opc = 0;
                    }
                    *((int *) fld->value) = opc;
                    update_field(fld, true);
                }  else if (key == '-') {
                    if (opc-- == 0) {
                        opc = nopc-1;
                    }
                    *((int *) fld->value) = opc;
                    update_field(fld, true);
                }
            }
            break;
            case FLD_BOOL: {
                if ((key == '+') || (key == '-') || (key == ' ')) {
                    *(bool *)fld->value = !(*(bool *)fld->value);
                    update_field(fld, true);
                }
            }
            break;
            case FLD_COLOR: {
                uint8_t color = *((uint8_t *) fld->value);
                if (key == '+') {
                    *((uint8_t *) fld->value) = color + 1;
                    update_field(fld, true);
                }  else if (key == '-') {
                    *((uint8_t *) fld->value) = color - 1;
                    update_field(fld, true);
                }
            }
            break;
        }
    }
}

