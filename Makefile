# only tested in mingw
PNAME = bfcl
OBJS = $(PNAME).o ocl_util.o sha1_16.o utils.o
CFLAGS += -std=c11 -Wall -I$(INTELOCLSDKROOT)/include
LDFLAGS += -L$(INTELOCLSDKROOT)/lib/x64 -g

all : $(PNAME)

$(PNAME) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL

clean :
	rm $(PNAME) *.o
