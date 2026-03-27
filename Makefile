.PHONY: run
run:
	make classify_input
	./classify_input

.PHONY: clean
clean:
	rm classify_input

classify_input: classify_input.c
	clang classify_input.c -o classify_input
