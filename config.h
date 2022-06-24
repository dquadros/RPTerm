/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Shiela Dixon, https://peacockmedia.software  
 *
 * Local configuration defines
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

#ifndef _CONFIG_H
#define _CONFIG_H

extern void config_enter(void);
extern void config_leave(void);
extern void config_key(u8 key);

extern const char *config_getbaud(void);
extern uint config_getbaudrate(void);
extern SERIAL_FMT config_getfmt(void);

#endif
