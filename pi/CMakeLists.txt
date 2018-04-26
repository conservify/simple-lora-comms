find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
find_path(WIRINGPI_INCLUDE_DIRS NAMES wiringPi.h)

if(EXISTS ${WIRINGPI_LIBRARIES})
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  include_directories(.)
  include_directories(../common)
  include_directories(../protocol/src)
  include_directories(${WIRINGPI_INCLUDE_DIRS})

  include_directories(../gitdeps/nanopb)

  file(GLOB SOURCE_FILES *.cpp ../common/*.cpp ../protocol/src/*.c ../gitdeps/nanopb/*.c)

  add_executable(lora-pi-test ${SOURCE_FILES})
  set_target_properties(lora-pi-test PROPERTIES COMPILE_FLAGS "-Wall -ggdb")
  target_link_libraries(lora-pi-test pthread)
  target_link_libraries(lora-pi-test ${WIRINGPI_LIBRARIES})
else()
  message("** [WARN] No wiringPi build files found, skipping PI")
endif()

