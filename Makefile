# only tested in mingw
PNAME = bfcl
OBJS = $(PNAME).o ocl_util.o utils.o sha1_16.o aes_128.o ocl_test.o ocl_brute.o
CFLAGS += -std=c11 -Wall -Werror -O2 -mrdrnd

ifeq ($(OS),Windows_NT)
	CFLAGS += -I$(INTELOCLSDKROOT)/include
	LDFLAGS += -L$(INTELOCLSDKROOT)/lib/x64
else
	# default Intel's Linux OpenCL SDK installation location
	CFLAGS += /opt/intel/opencl-sdk/include
	LDFLAGS += -L/opt/intel/opencl-sdk/lib64
endif

all : $(PNAME)

$(PNAME) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL -Wl,-Bstatic -lmbedcrypto -Wl,-Bdynamic

clean :
	rm $(PNAME) *.o

