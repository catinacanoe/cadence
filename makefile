asan:
	clang++ \
		-fsanitize=address \
		-fno-omit-frame-pointer -g \
		-Wall \
		-std=c++17 \
		-lncursesw \
		-Wl,--verbose \
		src/*.cpp -o bin/asan

nosan:
	clang++ \
		-fno-omit-frame-pointer -g -O0 \
		-D_GLIBCXX_DEBUG \
		-std=c++17 \
		-Wall \
		-lncursesw \
		src/*.cpp -o bin/nosan
