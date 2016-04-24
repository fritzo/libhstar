# .SILENT:
PY_FILES := *.py $(shell find src | grep '\.py$$')
C_FILES := $(shell find src | grep '\.[ch]$$')

all: python debug release

lint: FORCE
	$(info flake8)
	@flake8 --ignore=E402 $(PY_FILES)

format: FORCE
	pyformat -i $(PY_FILES)
	clang-format -i $(C_FILES)
	@# TODO use clang-tidy

python: lint FORCE
	pip install -e .

CMAKE = cmake
ifdef CC
	CMAKE += -DCMAKE_C_COMPILER=$(CC)
endif
ifdef CXX
	CMAKE += -DCMAKE_CXX_COMPILER=$(CXX)
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
	HSTAR_LOG_LEVEL=10 py.test -v hstar  # 10 = DEBUG
	@echo '----------------'
	@echo 'PASSED ALL TESTS'

clean: FORCE
	rm -rf build lib
	git clean -fdx -e hstar.egg-info -e .idea

FORCE:
