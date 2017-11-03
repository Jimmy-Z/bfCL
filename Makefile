# only tested in mingw
PNAME = bfcl
OBJS = $(PNAME).o ocl_util.o utils.o sha1_16.o aes_128.o ocl_test.o ocl_brute.o
CFLAGS += -std=c11 -Wall -Werror -O2 -mrdrnd -I$(INTELOCLSDKROOT)/include
LDFLAGS += -L$(INTELOCLSDKROOT)/lib/x64

all : $(PNAME)

$(PNAME) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL -static -lmbedcrypto

clean :
	rm $(PNAME) *.o

