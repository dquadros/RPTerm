/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication via UART on GPIO 12 & 13
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software  
 * - Miroslav Nemecek (picovga)  http://www.breatharian.eu/hw/picovga/index_en.html
 *
 * This module handles the UART
 * Written on feb/22 by Daniel Quadros, https:dqsoft.blogspot.com
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

/*
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pico.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
*/

#ifndef _SERIAL_H
#define _SERIAL_H

bool has_rx(void);
void put_rx(uint8_t ch);
uint8_t get_rx(void);
void put_tx(uint8_t ch);
void serial_init(void);
void serial_tx_task(void);

#endif