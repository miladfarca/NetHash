VPATH=${SRC_PATH}:./src
CC=gcc
AR=ar
ODIR=obj
_LIB_OBJ=nethash.o sha256.o flag.o
_TEST_OBJ=test.o nethash.o sha256.o flag.o
LIB_OBJ=$(patsubst %,$(ODIR)/%,$(_LIB_OBJ))
TEST_OBJ=$(patsubst %,$(ODIR)/%,$(_TEST_OBJ))

$(ODIR)/%.o: %.c
	@mkdir -p $(ODIR)
	$(CC) -c -Wall -O3 -o $@ $< $(CFLAGS) -I/usr/include/libxml2

libnethash.a: $(LIB_OBJ)
	$(AR) rcs $@ $^

nethash-tests: $(TEST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lxml2 -lcurl -lcrypto

clean:
	rm -f $(ODIR)/*.o
