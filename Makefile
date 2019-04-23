BUILD ?= $(abspath build)
GO=env GOOS=linux GOARCH=arm go

default: protocol cmake
	$(MAKE) -C $(BUILD)

protocol:
	$(MAKE) -C protocol

$(BUILD)/golora-arm-test: pi/*.go
	$(GO) build -o $(BUILD)/golora-arm-test pi/*.go

cmake: gitdeps
	mkdir -p $(BUILD)
	cd $(BUILD) && cmake ../

gitdeps:
	simple-deps --config mcu/dependencies.sd

clean:
	rm -rf $(BUILD)

veryclean:
	rm -rf $(BUILD) gitdeps

.PHONY: protocol
