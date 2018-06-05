.PHONY: build64

.DEFAULT_GOAL = build64

.ONESHELL:
SHELL=/bin/bash
build64:
	$(MAKE) -C ../ gcc-release
	cd ../wasm2c/
	g++ -O3 -fPIC -shared wasm_sandbox.cpp -o ../bin/libwasm_sandbox.a
	source ../../emsdk/emsdk_env.sh
	cd tests
	emcc test_dyn_lib.c -s WASM=1 -O3 -s SIDE_MODULE=1 -o test_dyn_lib_my.wasm
	../../bin/wasm2wat --inline-exports --inline-imports -f test_dyn_lib_my.wasm -o test_dyn_lib_my.wat
	bash -c 'emcc test_dyn_lib.c -s WASM=1 -O3 -s ASSERTIONS=2 -s STACK_OVERFLOW_CHECK=2 -s "EXPORTED_FUNCTIONS=[$$(../../builds/getWastFunctions ./test_dyn_lib_my.wat)]" -o wasm_test_dyn_lib.js'
	../../bin/wasm2c wasm_test_dyn_lib.wasm -o wasm_test_dyn_lib.c
	gcc ../wasm-rt-impl.c wasm_test_dyn_lib.c -I ../ -fPIC -shared -O3 -o libwasm_test_dyn_lib.so

run64:
	cd ../wasm2c/tests && ./test_app