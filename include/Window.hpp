#pragma once
#pragma once

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

class Window
{
public:
	Window() = default;

	bool Create(const char* name, int width, int height);
	void Destroy();
	
	const int GetWindowWidth() const;
	const int GetWindowHeight() const;
	const char* GetWindowName() const;
	GLFWwindow* GetWindowInstance();

private:
	int m_width;
	int m_height;
	char* m_name;
	GLFWwindow* m_instance;
};