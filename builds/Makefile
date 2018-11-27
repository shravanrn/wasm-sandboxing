.PHONY: build64 clean wabt_bin wasm_sandbox_lib

.DEFAULT_GOAL = build64

ifeq ($(DEBUG),1)
	CFLAGS = -g3 -O0
else
	CFLAGS = -O3
endif

# Note edit choice of backend - llvm vs emscripten in ~/.emscripten file by editing llvm_root

wabt_bin: 
	$(MAKE) -C ../ gcc-release

wasm_sandbox_lib:
	cd ../wasm2c/ && \
	g++ $(CFLAGS) -fPIC -shared wasm_sandbox.cpp -o ../bin/libwasm_sandbox.o && \
	ar crf ../bin/libwasm_sandbox.a ../bin/libwasm_sandbox.o

.ONESHELL:
SHELL=/bin/bash
build64: wabt_bin wasm_sandbox_lib

clean:
	rm -rf ../bin/
	rm -rf ../wasm2c/tests/out/