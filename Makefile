# A Makefile for both Linux and Windows, 06-dec-2017

#define all executables here
app_name= llem


all: ${app_name}


#define compiler options	
CC=g++

ifneq ($(OS),Windows_NT)			#linux?
#	CFLAGS=-g -O0 -ffast-math -fno-inline -Dbuild_date="\"`date +%Y-%b-%d`\"" #-Dbuild_date="\"2016-Mar-23\""
	CFLAGS=-g -O3 -Wfatal-errors -fpermissive -Dbuild_date="\"`date +%Y-%b-%d`\"" #-Dbuild_date="\"2016-Mar-23\""			#64 bit
#	LIBS=-L/usr/X11/lib -L/usr/local/lib -lfltk_images /usr/local/lib/libfltk.a -lpng -lz -ljpeg -lrt -lm -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lpthread -ldl -lX11 -lfftw3 #-ljack
#	INCLUDE= -I/usr/local/include
	LIBS=-lfltk -lfltk_images -lX11 -lpng -lz -ljpeg -lrt -lm -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lXrender -lpthread -ldl -lX11 -lasound -ljack -lasound `pkg-config --libs rtaudio` -ljack	#64 bit
	INCLUDE= -I/usr/include/cairo	#64 bit

else								#windows?
	CFLAGS=-g -DWIN32 -mms-bitfields -Dcompile_for_windows -Dbuild_date="\"`date +%Y\ %b\ %d`\""
LIBS= -L/usr/local/lib -static -mwindows -lfltk_images -lfltk -lfltk_png -lfltk_jpeg -lole32 -luuid -lcomctl32 -lwsock32 -lWs2_32 -lm -lfftw3 -lwinmm
	INCLUDE= -I/usr/local/include
endif



#define object files for each executable, see dependancy list at bottom
obj1= llem_main.o GCProfile.o pref.o GCLed.o GCCol.o mgraph.o gc_rtaudio.o rt_code.o filter_code.o antialias_code.o llem_code.o line_clip_code.o freeverb_code.o audio_formats.o
#obj2= backprop.o layer.o



#linker definition
llem: $(obj1)
	$(CC) $(CFLAGS) -o $@ $(obj1) $(LIBS)


#linker definition
#backprop: $(obj2)
#	$(CC) $(CFLAGS) -o $@ $(obj2) $(LIBS)



#compile definition for all cpp files to be complied into .o files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.cxx
	$(CC) $(CFLAGS) $(INCLUDE) -c $<



#dependancy list per each .o file
llem_main.o: llem_main.h globals.h GCProfile.h pref.h GCCol.h GCLed.h mgraph.h gc_rtaudio.h rt_code.h filter_code.h antialias_code.h llem_code.h line_clip_code.h freeverb_code.h audio_formats.h
GCProfile.o: GCProfile.h
pref.o: pref.h GCCol.h GCLed.h
GCCol.o:  GCCol.h
GCLed.o: GCLed.h
my_input_wheel.o: my_input_wheel.h GCProfile.h
mgraph.o: mgraph.h GCProfile.h
gc_rtaudio.o: gc_rtaudio.h globals.h
rt_code.o: rt_code.h globals.h GCProfile.h
filter_code.o: filter_code.h globals.h GCProfile.h
antialias_code.o: antialias_code.h globals.h GCProfile.h mgraph.h 
llem_code.o: llem_code.h globals.h GCProfile.h mgraph.h line_clip_code.h 
line_clip_code.o: line_clip_code.h GCProfile.h
freeverb_code.o: freeverb_code.h GCProfile.h
audio_formats.o: audio_formats.h GCProfile.h
#layer.o: layer.h


.PHONY : clean
clean : 
		-rm $(obj1)					#remove obj files
ifneq ($(OS),Windows_NT)
		-rm ${app_name}				#remove linux exec
else
		-rm ${app_name}.exe			#remove windows exec
endif


