




ring_buffer.o: ring_buffer.c

test_ring_buffer.o: test_ring_buffer.c

test_ring_buffer: test_ring_buffer.o ring_buffer.o
	gcc -lpthread test_ring_buffer.o ring_buffer.o -o test_ring_buffer
clean: 
	rm *.o