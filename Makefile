GO=env GOOS=linux GOARCH=arm go

default: gitdeps cmake
	cd build && make

build/golora-arm-test: pi/*.go
	$(GO) build -o build/golora-arm-test pi/*.go

cmake:
	mkdir -p build
	cd build && cmake ../

gitdeps:
	simple-deps --config mcu/arduino-libraries

clean:
	rm -rf build
