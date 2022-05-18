cmake_minimum_required(VERSION 3.14)


include(pico_sdk_import.cmake)       

pico_sdk_init()

project(rpterm VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall
    -Wno-format          
    -Wno-unused-function 
    -Wno-unused-variable
    -Wno-maybe-uninitialized
    -Wno-psabi
    -O3
    -ffunction-sections -fdata-sections
    -funsigned-char
    -fno-exceptions -fno-unwind-tables
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
    -DLOCALISE_BR
)

add_executable(rpterm
               main.cpp
               serial.cpp
               terminal.cpp
               keybd.cpp
               config.cpp

               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_atext.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_attrib8.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_color.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_ctext.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_dtext.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_fastsprite.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_ftext.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_graph1.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_graph2.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_graph4.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_graph8.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_graph8mat.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_graph8persp.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_gtext.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_level.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_levelgrad.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_mtext.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_oscil.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_oscline.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_persp.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_persp2.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_plane2.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_progress.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_sprite.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tile.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tile2.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tilepersp.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tilepersp15.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tilepersp2.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tilepersp3.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/render/vga_tilepersp4.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_blitkey.S
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_render.S

               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_layer.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_pal.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_screen.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_util.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga_vmode.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/util/canvas.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/util/mat2d.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/util/overclock.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/util/print.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/util/rand.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/util/pwmsnd.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_bold_8x8.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_bold_8x14.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_bold_8x16.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_boldB_8x14.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_boldB_8x16.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_game_8x8.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_ibm_8x8.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_ibm_8x14.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_ibm_8x16.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_ibmtiny_8x8.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_italic_8x8.cpp
               ${CMAKE_CURRENT_LIST_DIR}/_picovga/font/font_thin_8x8.cpp
               )

pico_enable_stdio_usb(rpterm 0)
pico_enable_stdio_uart(rpterm 0)
               

target_include_directories(rpterm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

pico_generate_pio_header(rpterm ${CMAKE_CURRENT_LIST_DIR}/_picovga/vga.pio)

pico_add_extra_outputs(rpterm)

target_link_libraries(rpterm
                      pico_multicore
                      pico_stdlib
                      hardware_gpio
                      hardware_uart
                      hardware_irq
                      hardware_adc
                      hardware_dma
                      hardware_flash
                      hardware_i2c
                      hardware_interp
                      hardware_pio
                      hardware_pwm
                      hardware_resets
                      hardware_rtc
                      hardware_spi
                      tinyusb_board
                      tinyusb_host
                      )
