.PHONY: build64 run64 debug64 clean wabt_bin wasm_sandbox_lib

.DEFAULT_GOAL = build64

ifeq ($(DEBUG),1)
	CFLAGS = -g3 -O0
	EMSCRIPTEN_CFLAGS = -g3 -O0
else
	CFLAGS = -O3
	#Emscripten's optimization optimize as a whole program optimization style
	#This obviously destroys certain behaviour of functions when the wasm module is used as a library
	#So we do not use optimization flags in the emscripten output
	EMSCRIPTEN_CFLAGS = -O0
	#We could use the linkable flag, but this doesn't remove unused syscalls very well
	#In the future if we implement syscalls we can also use this option
	#EMSCRIPTEN_CFLAGS = -O3 -s LINKABLE=1
endif

# Note edit choice of backend - llvm vs emscripten in ~/.emscripten file by editing llvm_root

wabt_bin: 
	$(MAKE) -C ../ gcc-release

wasm_sandbox_lib:
	cd ../wasm2c/ && g++ $(CFLAGS) -fPIC -shared wasm_sandbox.cpp -o ../bin/libwasm_sandbox.a

WASM_SANDBOX_DIR=$(shell realpath ../../wasm-sandboxing)
CURRPATH=$(shell realpath .)

# 1 - .a input lib file
# 2 - .js output file
define convert_to_wasm =
	emcc $(1) -O0 -s WASM=1 -s TOTAL_MEMORY=2147418112 -s ALLOW_MEMORY_GROWTH=0 -s LEGALIZE_JS_FFI=0 -s EMULATED_FUNCTION_POINTERS=1 -s "EXPORTED_FUNCTIONS=[$$($(WASM_SANDBOX_DIR)/builds/getLLVMFileFunctions $(1)), '_malloc', '_free']" -o $(2) && \
	$(WASM_SANDBOX_DIR)/bin/wasm2wat --inline-exports --inline-imports -f $(patsubst %.js,%.wasm,$(2)) -o $(patsubst %.js,%.wat,$(2)) && \
	$(WASM_SANDBOX_DIR)/bin/wasm2c $(patsubst %.js,%.wasm,$(2)) -o $(patsubst %.js,%.c,$(2)) && \
	$(WASM_SANDBOX_DIR)/builds/generateModuleSpecificConstants $(2) > $(patsubst %.js,%_rt.cpp,$(2)) && \
	gcc $(CFLAGS) -fPIC -I $(WASM_SANDBOX_DIR)/wasm2c -c $(patsubst %.js,%.c,$(2)) -o $(patsubst %.js,%.o,$(2)) && \
	g++ $(CFLAGS) -fPIC -std=c++11 $(WASM_SANDBOX_DIR)/wasm2c/wasm-rt-impl.cpp $(WASM_SANDBOX_DIR)/wasm2c/wasm-rt-syscall-impl.cpp $(patsubst %.js,%_rt.cpp,$(2)) $(patsubst %.js,%.o,$(2)) -I $(WASM_SANDBOX_DIR)/wasm2c -fPIC -shared -o $(patsubst %.js,%.so,$(2))
endef

.ONESHELL:
SHELL=/bin/bash
build64: wabt_bin wasm_sandbox_lib
	source ../../emsdk/emsdk_env.sh
	# export EMCC_EXPERIMENTAL_USE_LLD=0
	mkdir -p ../wasm2c/tests/out
	cd ../wasm2c/tests/out
	emcc ../test_dyn_lib.c $(EMSCRIPTEN_CFLAGS) -c -o test_dyn_lib.o
	$(call convert_to_wasm,$(join $(shell realpath $(CURRPATH)/../wasm2c/tests/out), /test_dyn_lib.o),$(join $(shell realpath $(CURRPATH)/../wasm2c/tests/out), /libwasm_test_dyn_lib.js))
	g++ $(CFLAGS) ../test_app.cpp ../../../bin/libwasm_sandbox.a -o test_app -ldl

run64:
	cd ../wasm2c/tests/out && ./test_app

debug64:
	cd ../wasm2c/tests/out && gdb --args ./test_app

clean:
	rm -rf ../bin/
	rm -rf ../wasm2c/tests/out/