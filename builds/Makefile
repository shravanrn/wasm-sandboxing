.PHONY: build64

.DEFAULT_GOAL = build64

build64:
	$(MAKE) -C ../ gcc-release
	cd ../wasm2c/ && g++ -fPIC -shared wasm_sandbox.cpp -o ../bin/libwasm_sandbox.a
	bash -c "source ../../emsdk/emsdk_env.sh && cd ../wasm2c/tests && emcc test_dyn_lib.c -s WASM=1 -O3 -s SIDE_MODULE=1 -o test_dyn_lib.wasm"
	cd ../wasm2c/tests && ../../bin/wasm2c test_dyn_lib.wasm -o wasm_test_dyn_lib.c
	cd ../wasm2c/tests && gcc ../wasm-rt-impl.c wasm_test_dyn_lib.c -I ../ -fPIC -shared -O3 -o libwasm_test_dyn_lib.so
	cd ../wasm2c/tests && g++ test_app.cpp -o test_app ../../bin/libwasm_sandbox.a -ldl

run64:
	cd ../wasm2c/tests && ./test_app