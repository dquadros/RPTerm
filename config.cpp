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

// Configuration screen colors
static u8 color_cfg_chr = COL_WHITE;
static u8 color_cfg_bkg = COL_DARKGREEN;

// Local rotines
static void config_write(int l, int c, const char *str);


// Enter local configuration mode
void config_enter() {
    cls(color_cfg_bkg, color_cfg_chr);
    config_write(0, 0, "TERMINAL CONFIGURATION");
}

// Write string to configuration screen
static void config_write(int l, int c, const char *str) {
    uint8_t *pos = linAddr[l] + 3*c;
    for(int i=0; str[i] != '\0'; i++){
        *pos = str[i];
        pos += 3;
    }
}

// Handle keys in config screen
void config_key(u8 key){
    // TODO
}

