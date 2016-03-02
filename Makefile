# .SILENT:
PY_FILES := *.py $(shell find src | grep '\.py$$')
C_FILES := $(shell find src | grep '\.[ch]$$')

all: python debug release

lint: FORCE
	$(info pyflakes)
	@pyflakes $(PY_FILES)
	$(info pep8)
	@pep8 --ignore=E402 $(PY_FILES)

format: FORCE
	pyformat -i $(PY_FILES)
	clang-format -i $(C_FILES)

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
	mkdir -p build/debug
	cd build/debug \
	  && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug ../.. \
	  && $(MAKE)

release: FORCE
	mkdir -p build/release
	cd build/release \
	  && $(CMAKE) -DCMAKE_BUILD_TYPE=RelWithDebInfo ../.. \
	  && $(MAKE)

test: all FORCE
	HSTAR_DEBUG=1 nosetests -v pomagma
	@echo '----------------'
	@echo 'PASSED ALL TESTS'

clean: FORCE
	rm -rf build lib
	git clean -fdx -e hstar.egg-info -e node_modules -e data

FORCE:
