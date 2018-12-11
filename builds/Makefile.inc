THIS_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
WASM_SANDBOX_DIR=$(shell realpath $(THIS_DIR)/..)

# 1 - abs path to .o/.a input lib file
# 2 - abs path to .js output file
# 3 - CFLAGS
# 4 - libs
define convert_to_wasm =
	input_lib="$(1)"; \
	wasm_output="$(patsubst %.js,%.wasm,$(2))"; \
	wat_output="$(patsubst %.js,%.wat,$(2))"; \
	converted_c="$(patsubst %.js,%.c,$(2))"; \
	runtime_cpp="$(patsubst %.js,%_rt.cpp,$(2))"; \
	object_file="$(patsubst %.js,%.o,$(2))"; \
	shared_library="$(patsubst %.js,%.so,$(2))"; \
	rm -rf $(join $(dir $${input_lib}), mainCombine) && mkdir $(join $(dir $${input_lib}), mainCombine) && \
	if [[ $${input_lib} == *.o ]]; then \
		cp $${input_lib} $(join $(dir $${input_lib}), mainCombine)/; \
	else \
		cd $(join $(dir $${input_lib}), mainCombine)/ && llvm-ar -x $${input_lib} && cd ..; \
	fi && \
	for i in $$(echo $(4) | sed "s/,/ /g"); do \
		cd $(join $(dir $${input_lib}), mainCombine)/ && llvm-ar -x $${i} && cd ..; \
	done && \
	emcc $(join $(dir $${input_lib}), mainCombine)/*.o -s WASM=1 -s TOTAL_MEMORY=2147418112 -s ALLOW_MEMORY_GROWTH=0 -s LEGALIZE_JS_FFI=0 -s EMULATED_FUNCTION_POINTERS=1 -s "EXPORTED_FUNCTIONS=[$$($(WASM_SANDBOX_DIR)/builds/getLLVMFileFunctions $${input_lib}), '_malloc', '_free']" -o $(2) && \
	$(WASM_SANDBOX_DIR)/bin/wasm2wat --inline-exports --inline-imports -f $${wasm_output} -o $${wat_output} && \
	$(WASM_SANDBOX_DIR)/bin/wasm2c $${wasm_output} -o $${converted_c} && \
	$(WASM_SANDBOX_DIR)/builds/generateModuleSpecificConstants $(2) > $${runtime_cpp} && \
	gcc $(3) -fPIC -I $(WASM_SANDBOX_DIR)/wasm2c -c $${converted_c} -o $${object_file} && \
	g++ $(3) -std=c++11 -fPIC $(WASM_SANDBOX_DIR)/wasm2c/wasm-rt-impl.cpp $(WASM_SANDBOX_DIR)/wasm2c/wasm-rt-syscall-impl.cpp $${runtime_cpp} $${object_file} -I $(WASM_SANDBOX_DIR)/wasm2c -fPIC -shared -o $${shared_library}
endef