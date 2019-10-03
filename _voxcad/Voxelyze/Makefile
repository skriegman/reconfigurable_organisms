# This is the directory in which to find subdirectories to install/find headers and libs:
USER_HOME_PATH = $(HOME)


VOXELYZE_NAME = voxelyze.0.9.92
VOXELYZE_VERSION = voxelyze.0.9
VOXELYZE_LIB_NAME = lib$(VOXELYZE_NAME).a
VOXELYZE_LIB_VERSION = lib$(VOXELYZE_VERSION).a


# compiler flags
# -g --Enable debugging
# -Wall --Turn on all warnings
# -D_USE_FIXED_PROTOTYPES_
# --Force the compiler to use the correct headers
# -ansi --Don't use GNU ext; do use ansi standard.
#
# may have to run "source /opt/intel/composerxe/bin/compilervars.sh intel64" to get intel compiler?

# CXX=icpc
# CC=icpc
CXX=g++
CC=g++
#INCLUDE=-I./zlib-1.2.5 -I./Voxelyze -I./TinyXML
# INCLUDE= -I./Voxelyze -I./Voxelyze/Utils -l./Voxelyze/Utils/zlib
INCLUDE= -I./Voxelyze -I./Voxelyze/Utils


# CFLAGS= -xhost -O3 -Wall $(INCLUDE)
CFLAGS= -O3 -Wall -std=c++11 $(INCLUDE)
#CFLAGS= -g -Wall $(INCLUDE)

CPPFLAGS= $(CFLAGS)

Vox_src = \
	VX_Benchmark.cpp \
	VX_Bond.cpp \
	VX_Environment.cpp \
	VX_FEA.cpp \
	VX_FRegion.cpp \
	VX_MeshUtil.cpp \
	VX_Object.cpp \
	VX_Sim.cpp \
	VX_SimGA.cpp \
	VX_Voxel.cpp \
	VXS_BondCollision.cpp \
	VXS_Bond.cpp \
	VXS_BondInternal.cpp \
	VXS_Voxel.cpp \
	Utils/Array3D.cpp \
	Utils/MarchCube.cpp \
	Utils/Mesh.cpp \
	Utils/XML_Rip.cpp
	# main.cpp


tinyxml_src = \
	Utils/tinyxml.cpp\
	Utils/tinyxmlerror.cpp\
	Utils/tinyxmlparser.cpp

VOXELYZE_OBJS = \
	VX_Benchmark.o \
	VX_Bond.o \
	VX_Environment.o \
	VX_FEA.o \
	VX_FRegion.o \
	VX_MeshUtil.o \
	VX_Object.o \
	VX_Sim.o \
	VX_SimGA.o \
	VX_Voxel.o \
	VXS_BondCollision.o \
	VXS_Bond.o \
	VXS_BondInternal.o \
	VXS_Voxel.o \
	Utils/Array3D.o \
	Utils/MarchCube.o \
	Utils/Mesh.o \
	Utils/XML_Rip.o \
	Utils/tinyxml.o \
	Utils/tinyxmlerror.o \
	Utils/tinyxmlparser.o
# VXS_SimGLView.o \



all: $(VOXELYZE_LIB_VERSION)


# Auto sorts out dependencies (but leaves .d files):
%.o: %.cpp
	@echo making $@ and dependencies for $< at the same time
	@$(CC) -c $(CXXFLAGS) -o $@ $<
	@$(CC) -MM -MP $(CXXFLAGS) $< -o $*.d

-include *.d


#%.o: %.cpp
#	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

#Utils/%.o: Utils/%.cpp
#	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<




$(VOXELYZE_LIB_VERSION):	$(VOXELYZE_OBJS)
	ar rcv $(VOXELYZE_LIB_VERSION) $(VOXELYZE_OBJS)


voxelize: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

#vox.o: $(Vox_src)#
#	$(CC) $(CFLAGS) $(INCLUDE) $(Vox_src)

#zlib.o: $(zlib_src)
#	$(CC) $(CFLAGS) $(INCLUDE) $(zlib_src)

clean:
	rm -rf *.o */*.o *.d */*.d $(VOXELYZE_LIB_VERSION)


##################################################


$(USER_HOME_PATH)/include:
		mkdir $(USER_HOME_PATH)/include

$(USER_HOME_PATH)/lib:
		mkdir $(USER_HOME_PATH)/lib


installusr:     $(USER_HOME_PATH)/include $(USER_HOME_PATH)/lib
		cp $(VOXELYZE_LIB_VERSION) $(USER_HOME_PATH)/lib/$(VOXELYZE_LIB_VERSION)
		rm -f $(USER_HOME_PATH)/lib/$(VOXELYZE_LIB_NAME)
		ln -s $(VOXELYZE_LIB_VERSION) $(USER_HOME_PATH)/lib/$(VOXELYZE_LIB_NAME)
		rm -rf $(USER_HOME_PATH)/include/$(VOXELYZE_VERSION)
		-mkdir $(USER_HOME_PATH)/include/$(VOXELYZE_VERSION)
		cp *.h $(USER_HOME_PATH)/include/$(VOXELYZE_VERSION)
		-mkdir $(USER_HOME_PATH)/include/$(VOXELYZE_VERSION)/Utils
		cp Utils/*.h $(USER_HOME_PATH)/include/$(VOXELYZE_VERSION)/Utils
		rm -f $(USER_HOME_PATH)/include/$(VOXELYZE_NAME)
		ln -s $(VOXELYZE_VERSION) $(USER_HOME_PATH)/include/$(VOXELYZE_NAME)



installglobal:
		cp $(VOXELYZE_LIB_VERSION) $(GLOBAL_PATH)/lib/$(VOXELYZE_LIB_VERSION)
		rm -f $(GLOBAL_PATH)/lib/$(VOXELYZE_LIB_NAME)
		ln -s $(VOXELYZE_LIB_VERSION) $(GLOBAL_PATH)/lib/$(VOXELYZE_LIB_NAME)
		rm -rf $(GLOBAL_PATH)/include/$(VOXELYZE_VERSION)
		-mkdir $(GLOBAL_PATH)/include/$(VOXELYZE_VERSION)
		cp *.h $(GLOBAL_PATH)/include/$(VOXELYZE_VERSION)
		-mkdir $(GLOBAL_PATH)/include/$(VOXELYZE_VERSION)/Utils
		cp Utils/*.h $(GLOBAL_PATH)/include/$(VOXELYZE_VERSION)/Utils
		rm -f $(GLOBAL_PATH)/include/$(VOXELYZE_NAME)
		ln -s $(VOXELYZE_VERSION) $(GLOBAL_PATH)/include/$(VOXELYZE_NAME)




