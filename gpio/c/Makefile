OBJECTS := main.o

gpio: $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) gpio

.PHONY: clean
