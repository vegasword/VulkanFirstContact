#include <Window.hpp>

bool Window::Create(const char* name, int width, int height)
{
	m_width = width;
	m_height = height;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_instance = glfwCreateWindow(width, height, name, 0, nullptr);
	return (m_instance != nullptr);
}

void Window::Destroy()
{
	glfwDestroyWindow(m_instance);
	glfwTerminate();
}

const char* Window::GetWindowName() const
{
	return m_name;
}

const int Window::GetWindowWidth() const
{
	return m_width;
}

const int Window::GetWindowHeight() const
{
	return m_height;
}

GLFWwindow* Window::GetWindowInstance()
{
	return m_instance;
}
