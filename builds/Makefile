.PHONY: build64 wabt_bin wasm_sandbox_lib

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

.ONESHELL:
SHELL=/bin/bash
build64: wabt_bin wasm_sandbox_lib
	source ../../emsdk/emsdk_env.sh
	cd ../wasm2c/tests
	emcc test_dyn_lib.c $(EMSCRIPTEN_CFLAGS) -s WASM=1 -s SIDE_MODULE=1 -s EMULATED_FUNCTION_POINTERS=1 -o test_dyn_lib_my.wasm
	../../bin/wasm2wat --inline-exports --inline-imports -f test_dyn_lib_my.wasm -o test_dyn_lib_my.wat
	# There is a tool failure if we try setting the total memory to 4GB (causing a u32 overflow) and 
	# a runtime failure of signed vs unsigned if we set this to more that 2GB. So we set it to (2GB - 64KB)
	bash -c 'emcc test_dyn_lib.c $(EMSCRIPTEN_CFLAGS) -s WASM=1 -s TOTAL_MEMORY=2147418112 -s ALLOW_MEMORY_GROWTH=0 -s LEGALIZE_JS_FFI=0 -s EMULATED_FUNCTION_POINTERS=1 -s "EXPORTED_FUNCTIONS=[$$(../../builds/getWastFunctions ./test_dyn_lib_my.wat), '_malloc', '_free']" -o wasm_test_dyn_lib.js'
	../../bin/wasm2wat --inline-exports --inline-imports -f wasm_test_dyn_lib.wasm -o wasm_test_dyn_lib.wat
	../../bin/wasm2c wasm_test_dyn_lib.wasm -o wasm_test_dyn_lib.c
	../../builds/generateModuleSpecificConstants wasm_test_dyn_lib.js > wasm_test_dyn_lib_rt.cpp
	g++ $(CFLAGS) ../wasm-rt-impl.cpp ../wasm-rt-syscall-impl.cpp wasm_test_dyn_lib_rt.cpp wasm_test_dyn_lib.c -I ../ -fPIC -shared -o libwasm_test_dyn_lib.so
	g++ $(CFLAGS) test_app.cpp ../../bin/libwasm_sandbox.a -o test_app -ldl

run64:
	cd ../wasm2c/tests && ./test_app

debug64:
	cd ../wasm2c/tests && gdb --args ./test_app