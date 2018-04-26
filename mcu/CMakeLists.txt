set(ARDUINO_IDE "${CMAKE_CURRENT_SOURCE_DIR}/../../arduino-1.8.3")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../arduino-cmake-minimal/cmake/")

if(EXISTS ${ARDUINO_IDE})
  include(ArduinoMinimal)

  include_directories(../common)

  file(GLOB SOURCE_FILES *.cpp ../common/*.cpp)

  set(LIBRARIES SPI Wire)

  arduino(lora-mcu-test "${SOURCE_FILES}" "${LIBRARIES}")
else()
  message("** [WARN] No Arduino build files found, skipping MCU (${ARDUINO_IDE})")
endif()
