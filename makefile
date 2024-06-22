asan:
	clang++ \
		-fsanitize=address \
		-fno-omit-frame-pointer -g \
		-std=c++17 \
		-lncursesw \
		src/*.cpp -o bin/asan

dbg:
	clang++ \
		-fsanitize=address \
		-fno-omit-frame-pointer -g -O0 \
		-std=c++17 \
		-lncursesw \
		src/*.cpp -o bin/dbg
