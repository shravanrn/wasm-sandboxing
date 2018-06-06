.PHONY: build64 wabt_bin wasm_sandbox_lib

.DEFAULT_GOAL = build64

CFLAGS:=$(CFLAGS)

wabt_bin: 
	$(MAKE) -C ../ gcc-release

wasm_sandbox_lib:
	cd ../wasm2c/ && g++ $(CFLAGS) -O3 -fPIC -shared wasm_sandbox.cpp -o ../bin/libwasm_sandbox.a

.ONESHELL:
SHELL=/bin/bash
build64: wabt_bin wasm_sandbox_lib
	source ../../emsdk/emsdk_env.sh
	cd ../wasm2c/tests
	emcc test_dyn_lib.c -s WASM=1 -O3 -s SIDE_MODULE=1 -o test_dyn_lib_my.wasm
	../../bin/wasm2wat --inline-exports --inline-imports -f test_dyn_lib_my.wasm -o test_dyn_lib_my.wat
	bash -c 'emcc test_dyn_lib.c -O3 -s WASM=1 -s "EXPORTED_FUNCTIONS=[$$(../../builds/getWastFunctions ./test_dyn_lib_my.wat), '_malloc', '_free']" -o wasm_test_dyn_lib.js'
	../../bin/wasm2c wasm_test_dyn_lib.wasm -o wasm_test_dyn_lib.c
	../../builds/generateModuleSpecificConstants wasm_test_dyn_lib.js > wasm_test_dyn_lib_rt.c
	gcc $(CFLAGS) ../wasm-rt-impl.c wasm_test_dyn_lib_rt.c wasm_test_dyn_lib.c -I ../ -fPIC -shared -O3 -o libwasm_test_dyn_lib.so
	g++ $(CFLAGS) test_app.cpp ../../bin/libwasm_sandbox.a -O3 -o test_app -ldl

run64:
	cd ../wasm2c/tests && ./test_app

debug64:
	cd ../wasm2c/tests && gdb --args ./test_app