# only tested in mingw
PNAME = bfcl
OBJS = $(PNAME).o ocl_util.o utils.o sha1_16.o aes128.o
CFLAGS += -std=c11 -Wall -O2 -I$(INTELOCLSDKROOT)/include
LDFLAGS += -L$(INTELOCLSDKROOT)/lib/x64

all : $(PNAME)

$(PNAME) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL

clean :
	rm $(PNAME) *.o
