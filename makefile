asan:
	clang++ \
		-fsanitize=address \
		-fno-omit-frame-pointer -g \
		-std=c++17 \
		-lncurses \
		src/*.cpp -o bin/asan
