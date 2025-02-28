VPATH=${SRC_PATH}:.
CC=gcc
ODIR=obj
_OBJ=main.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	@mkdir -p $(ODIR)
	$(CC) -c -O3 -o $@ $< $(CFLAGS)

nethash-example: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) ../libnethash.a -lxml2 -lcurl -lcrypto

clean:
	rm -f $(ODIR)/*.o
