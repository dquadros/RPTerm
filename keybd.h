/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Shiela Dixon, https://peacockmedia.software  
 *
 * Keyboard handling defines
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

#ifndef _KEYBD_H
#define _KEYBD_H

// Special keys
#define KEY_UP 0x80
#define KEY_DWN 0x81
#define KEY_LFT 0x82
#define KEY_RGT 0x83
#define KEY_HOME 0x84
#define KEY_END 0x85
#define KEY_F1 0x86
#define KEY_F2 0x87
#define KEY_F3 0x88
#define KEY_F4 0x89
#define KEY_F5 0x8A
#define KEY_F6 0x8B
#define KEY_F7 0x8C
#define KEY_F8 0x8D
#define KEY_F9 0x8E
#define KEY_F10 0x8F

// Alt Keys
#define KEY_ALT_C 0xF0      // Config
#define KEY_ALT_L 0xF1      // Local <-> on Line
#define KEY_ALT_R 0xF2      // Record file
#define KEY_ALT_T 0xF3      // Transmit file


// Keyboard buffer access
extern void keyb_init(void);
extern bool has_kbd(void);
extern uint8_t get_kbd(void);

// "Tasks" (rotines that will be continuous called in the main loop)
extern void cdc_task(void);
extern void hid_app_task(void);

#endif
