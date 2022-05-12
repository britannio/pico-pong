add_executable(pong
        main.c
        painting.c
        logic.c
        lib/fonts.c
        lib/st7735.c
        lib/DEV_Config.c
        lib/ICM20948.c
        )

# pull in common dependencies
target_link_libraries(
  pong
  hardware_pwm
  hardware_pio
  hardware_spi
  hardware_i2c
  pico_stdlib
  pico_multicore
  pico_time
)

pico_enable_stdio_usb(pong 1)
pico_enable_stdio_uart(pong 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(pong)

# add url via pico_set_program_url
example_auto_set_url(pong)
