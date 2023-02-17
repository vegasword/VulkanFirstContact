#pragma once

#include <Window.hpp>
#include <Engine.hpp>

class Application
{
public:
	Application() = default;

	void Create(const char* windowName, int windowWidth, int windowHeight);
	void Destroy();
	int Run();

private:
	Engine m_engine;
	Window m_window;
};