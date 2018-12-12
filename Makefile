GO=env GOOS=linux GOARCH=arm go

default: protocol cmake
	$(MAKE) -C build

protocol:
	$(MAKE) -C protocol

build/golora-arm-test: pi/*.go
	$(GO) build -o build/golora-arm-test pi/*.go

cmake: gitdeps
	mkdir -p build
	cd build && cmake ../

gitdeps:
	simple-deps --config mcu/dependencies.sd

clean:
	rm -rf build

veryclean:
	rm -rf build gitdeps

.PHONY: protocol
