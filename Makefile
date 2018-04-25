default: gitdeps cmake
	cd build && make

cmake:
	mkdir -p build
	cd build && cmake ../

gitdeps:
	simple-deps --config mcu/arduino-libraries

clean:
	rm -rf build
