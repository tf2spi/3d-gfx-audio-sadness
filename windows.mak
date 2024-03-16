BUILD=build
VKT=gfx\vulkan\khronos-tutorial
GLFW_LIBS=$(BUILD)\glfw3dll.lib
VKT_SOURCES=$(VKT)\khronos-tutorial.c $(GLFW_LIBS)

DEBUGFLAGS=/Zi /I ..\include
$(BUILD)\khronos-tutorial.exe : $(VKT_SOURCES)
	cd build & cl.exe $(patsubst %,..\\%, $**) $(DEBUGFLAGS) 
