CPUDEFS += -DHAS_CYCLONE=1 -DHAS_DRZ80=0
OBJDIRS += $(OBJ)/cpu/m68000_cyclone
CPUOBJS += $(OBJ)/cpu/m68000_cyclone/cyclone.o $(OBJ)/cpu/m68000_cyclone/c68000.o

#OBJDIRS += $(OBJ)/cpu/m68000_cyclone $(OBJ)/cpu/z80_drz80
#CPUOBJS += $(OBJ)/cpu/m68000_cyclone/cyclone.o $(OBJ)/cpu/m68000_cyclone/c68000.o
#CPUOBJS += $(OBJ)/cpu/z80_drz80/drz80.o $(OBJ)/cpu/z80_drz80/drz80_z80.o

#OBJDIRS += $(OBJ)/cpu/nec_armnec
#CPUDEFS += -DHAS_ARMNEC

CPUDEFS += -DUSE_VCHIQ_ARM -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS  -D_LINUX -fPIC -DPIC -D_REENTRANT -DHAVE_LIBOPENMAX=2 -DOMX -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM 

#CPUOBJS += $(OBJ)/cpu/nec_armnec/armV30.o $(OBJ)/cpu/nec_armnec/armV33.o $(OBJ)/cpu/nec_armnec/armnecintrf.o

OSOBJS = $(OBJ)/rpi/minimal.o \
	$(OBJ)/rpi/gp2x.o $(OBJ)/rpi/video.o $(OBJ)/rpi/blit.o \
	$(OBJ)/rpi/sound.o $(OBJ)/rpi/input.o $(OBJ)/rpi/fileio.o \
	$(OBJ)/rpi/usbjoy.o $(OBJ)/rpi/usbjoy_mame.o \
	$(OBJ)/rpi/config.o $(OBJ)/rpi/fronthlp.o \
	$(OBJ)/rpi/fifo_buffer.o $(OBJ)/rpi/thread.o \
	$(OBJ)/rpi/gp2x_frontend.o

mame.gpe:
	$(LD) $(LDFLAGS) \
		src/rpi/minimal.cpp \
		src/rpi/usbjoy.cpp \
		src/rpi/usbjoy_mame.cpp  \
		src/rpi/gp2x_frontend.cpp $(LIBS) -o $@
