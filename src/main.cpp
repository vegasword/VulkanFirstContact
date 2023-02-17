#include <Application.hpp>

int main()
{
    Application app;
    app.Create("Rotato PotatOS", 1920, 1080);
    app.Run();
    app.Destroy();
}