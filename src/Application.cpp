#include <iostream>
#include <stdexcept>

#include <MyMath.hpp>
#include <MyUtils.hpp>
#include <Application.hpp>

void Application::Create(const char* windowName, int windowWidth, int windowHeight)
{
	if (m_window.Create(windowName, windowWidth, windowHeight))
		std::runtime_error("Unable to create a window");
	m_engine.Create(&m_window);
}

void Application::Destroy()
{
	m_engine.Destroy();
	m_window.Destroy();
}

int Application::Run()
{
	try
	{
		GLFWwindow* window = m_window.GetWindowInstance();
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);
			m_engine.Update(&m_window);
			m_engine.Draw();
		}
		vkDeviceWaitIdle(m_engine.GetLogicalDevice());
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}