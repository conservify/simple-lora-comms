set(CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH})

include(Arduino)

if(EXISTS ${ARDUINO_IDE})
  enable_arduino_toolchain()

  include_directories(../src)

  file(GLOB sources *.cpp ../src/*.cpp ../src/*.c)

  find_package(SPI)
  find_package(Wire)
  find_package(lwstreams)
  find_package(arduino-logging)
  find_package(RadioHead)
  find_package(nanopb)

  add_arduino_library(lora-mcu-test "${sources}")

  target_link_libraries(lora-mcu-test Wire)
  target_link_libraries(lora-mcu-test lwstreams)
  target_link_libraries(lora-mcu-test arduino-logging)
  target_link_libraries(lora-mcu-test RadioHead)
  target_link_libraries(lora-mcu-test nanopb)
  target_link_libraries(lora-mcu-test SPI)

  add_arduino_firmware(lora-mcu-test)
else()
  message("** [WARN] No Arduino build files found, skipping MCU (${ARDUINO_IDE})")
endif()
