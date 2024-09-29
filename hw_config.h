/*
 * RPTERM - Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART
 * Daniel Quadros, https://dqsoft.blogspot.com
 * 
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * This file defines the hardware configurarion for my prototype with the RP2040-Zero
 * You may have to change it if using a different board
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

// UART
#define UART_ID         uart0
#define UART_TX_PIN     12
#define UART_RX_PIN     13

// BUZZER for Beep
#define BUZZER_PIN      9      // undefine if no buzzer
                                // 9 for the Pi Pico
				// 29 for RP2040 Zero

// Status LED
#define STATUS_LED      25      // undefine if no LED
				// 25 for Pi Pico
				// 11 for RP2040 Zero

// VGA
// Look for VGA_GPIO in vga_config.h

// Keyboard
// Keyboard language is defined in CMakeLists.xtx
