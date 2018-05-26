LAB=malloc

include Makefile.git

.PHONY: build run submit clean

build: $(LAB).c test.c
	$(call git_commit, "compile")
	gcc -std=c99 -O1 -Wall -ggdb -o malloc $^ -lpthread
	g++ -std=c++11 -o check-test check-test.cpp

run: build
	./malloc && ./check-test

submit:
	cd .. && tar cj $(LAB) > submission.tar.bz2
	curl -F "task=M4" -F "id=$(STUID)" -F "name=$(STUNAME)" -F "submission=@../submission.tar.bz2" 114.212.81.90:5000/upload

clean:
	-rm malloc
	-rm mem.log
	-rm check-test
