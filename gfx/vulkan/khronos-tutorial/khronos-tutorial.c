#include <GLFW/glfw3.h>
#include <stddef.h>

#define WIDTH 640
#define HEIGHT 480

int main()
{
	glfwInit();
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Khronos Tutorial", NULL, NULL);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
	return 0;
}
