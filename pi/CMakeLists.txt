find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
find_path(WIRINGPI_INCLUDE_DIRS NAMES wiringPi.h)

if(EXISTS ${WIRINGPI_LIBRARIES})
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  include_directories(.)
  include_directories(../common)
  include_directories(${WIRINGPI_INCLUDE_DIRS})

  add_executable(lora-pi-test lora-receiver.cpp lora_radio_pi.cpp)
  set_target_properties(lora-pi-test PROPERTIES COMPILE_FLAGS "-Wall -ggdb")
  target_link_libraries(lora-pi-test pthread)
  target_link_libraries(lora-pi-test ${WIRINGPI_LIBRARIES})
else()
  message("** [WARN] No wiringPi build files found, skipping PI")
endif()

