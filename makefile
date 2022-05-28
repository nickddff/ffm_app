MIPSGCC 				:= mips-linux-gnu-gcc
MIPSGPP                 := mips-linux-gnu-g++
MIPSLD				:= mips-linux-gnu-ld
MIPSOBJCOPY 		:= mips-linux-gnu-objcopy
MIPSOBJDUMP 		:= mips-linux-gnu-objdump

INCDIRS                 :=  project \
							ffmpeg \
                            mylib \
                        	v4l2dec \

SRCDIRS                 := 	project \
							ffmpeg \
                            mylib \
                            v4l2dec \

TARGET		  	?= ffm_app_2
CFLAG		    ?=  

#//-I ffmpeg \ -I mylib \ -I v4l2dec
INCLUDE         := $(patsubst %, -I %, $(INCDIRS)) -I /home/nick/M300/usr/include     

# project/main.c      v4l2dec/color_convert.c v4l2dec/v4l2.c v4l2dec/v4l2h264dec.c
CFILESDIR                  := $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))

# ffmpeg/ffmpegprocess.cpp  mylib/mythread.cpp mylib/cyclebuffer.cpp 
CPPFILESDIR				:= $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))

#main.c color_convert.c v4l2.c v4l2h264dec.c  
CFILE		:= $(notdir  $(CFILESDIR))
#ffmpegprocess.cpp mythread.cpp cyclebuffer.cpp   
CPPFILE		:= $(notdir  $(CPPFILESDIR))

COBJS			:= $(patsubst %, obj/%, $(CFILE:.c=.o))
CPPOBJS			:= $(patsubst %, obj/%, $(CPPFILE:.cpp=.o))
#obj/ffmpeg/ffmpegprocess.o  obj/mylib/mythread.o  obj/mylib/cyclebuffer.o...
OBJS			:= $(COBJS) $(CPPOBJS)

VPATH			:= $(SRCDIRS)

.PHONY: clean

$(TARGET) : $(OBJS)
	$(MIPSGCC) -Wall -o $(TARGET) $^   -L/home/nick/M300/usr/lib -lpthread -lz -lssl -lcrypto -ldrm -lavcodec -lavformat  -lavutil -lswscale -lswresample -lstdc++
$(COBJS) : obj/%.o : %.c
	$(MIPSGCC) -c $(INCLUDE) -o $@ $<
$(CPPOBJS) : obj/%.o : %.cpp
	$(MIPSGCC) -c $(INCLUDE) -o $@ $< -lstdc++

clean:
	rm -rf  $(TARGET) $(OBJS)
