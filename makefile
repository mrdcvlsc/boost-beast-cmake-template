.PHONY: default config build

default:
	@echo make config - configure with cmake
	@echo make build - compile program

config:
	cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_SHARED_LIBS=FALSE

build:
	cmake --build build --config Release

pack:
	cmake --build build --target package_project

clean:
	rm -rf build

test:
	cd build/main && ./bin/main-sync
	cd build/main && ./bin/main-async