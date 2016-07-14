# .SILENT:
C_FILES := $(shell find src | grep -v third_party | grep '\.[ch]$$')

all: debug release

lint: FORCE
	@# TODO use clang-tidy

format: FORCE
	$(info clang-format)
	@clang-format -i $(C_FILES)

CMAKE = cmake
ifdef CC
	CMAKE += -DCMAKE_C_COMPILER=$(CC)
endif

debug: FORCE
	test -e build/debug || ( \
	  mkdir -p build/debug \
	    && cd build/debug \
	    && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug ../.. \
	)
	$(MAKE) -C build/debug
release: FORCE
	test -e build/release || ( \
	  mkdir -p build/release \
	    && cd build/release \
	    && $(CMAKE) -DCMAKE_BUILD_TYPE=RelWithDebInfo ../.. \
	)
	$(MAKE) -C build/release

test: all FORCE
	CTEST_OUTPUT_ON_FAILURE=1 $(MAKE) -C build/debug test
	@echo '----------------'
	@echo 'PASSED ALL TESTS'

# This uses https://github.com/clibs/clib
update-deps: FORCE
	clib install silentbicycle/greatest -o src/third_party

clean: FORCE
	rm -rf build lib
	git clean -fdx -e .idea

FORCE:
