all:ctrl

ctrl: ctrl.o ctrlproc.o
	g++ $^ -o $@

clean:
	rm -rf *.o *.gch ctrl
