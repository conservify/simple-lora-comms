set(ARDUINO_IDE "${CMAKE_CURRENT_SOURCE_DIR}/../../arduino-1.8.3")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../arduino-cmake-minimal/cmake/")

if(EXISTS ${ARDUINO_IDE})
  include(ArduinoMinimal)

  include_directories(../common)

  set(SOURCE_FILES main.cpp lora_radio.cpp)

  set(LIBRARIES SPI)

  arduino(lora-mcu-test "${SOURCE_FILES}" "${LIBRARIES}")
else()
  message("** [WARN] No Arduino build files found, skipping MCU (${ARDUINO_IDE})")
endif()
