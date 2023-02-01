#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

#define VALIDATION_LAYERS 1
#if VALIDATION_LAYERS
const std::vector<const  char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};
#endif

typedef uint32_t u32;

class Application
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance vkInstance;
    
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    }

    VkResult createInstance()
    {
        // Temporary redundant variables
        u32 counter = 0;
        
        // Metadata of the application (mostly use by the driver for config).
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        /* (Instance)
         * Tells Vulkan driver which global extensions and validations layers
         * we want to use.
         *
         * Global extensions are a set of additional Vulkan features.
         *
         * Validations layers are optional components that hook into Vulkan
         * function calls to apply additional operations like checking the
         * values of parameters against the specification to detect misuse or
         * the thread safety...
        */
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Get and count the available extensions from the Vulkan instance
        // NOTE: To get the number of enumerated properties we can leave
        // blank the properties parameter in the  call (last parameter).
        u32 extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        // Check if the required extensions for GLFW are supported
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        for (u32 i = 0; i < glfwExtensionCount; i++)
            for (u32 j = 0; j < extensionCount; j++)
                if (strcmp(glfwExtensions[i], extensions[j].extensionName) == 0)
                    counter++;
        if (counter != glfwExtensionCount)
            throw std::runtime_error("A GLFW required instance extension is not supported");

        // Send GLFW extensions
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        /* (Validation layers)
         * There are two different types of validation layers:
         * - Instance layers only checking calls related to global Vulkan
         *   objects like instances
         * - Device specific layers checking only calls related to a specific
         *   GPU.
         *
         *  Even if device specific layers have now been deprecated, it sill
         *  recommended to enable validation layers at devices level as well
         *  for compatibility, which is required by some implementations.
         */
#if VALIDATION_LAYERS
        // Get and count the available layers  from the Vulkan instance
        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Check if all of the layers are available
        counter = 0;
        u32 validationLayerCount = static_cast<u32>(validationLayers.size());
        for (const char* layerName : validationLayers)
            for (const VkLayerProperties& layerProperties : availableLayers)
                if (strcmp(layerName, layerProperties.layerName) == 0)
                    counter++;
        if (counter != validationLayerCount)
            throw std::runtime_error("The specified validation layers is not available");

        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers.data();

        std::cout << "Validation layer enabled" << std::endl;
#else
        createInfo.enabledLayerCount = 0;
#endif
        //TODO(vegasword): https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
        
        // We finally create the Vulkan instance.
        VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan instance");

        return result;
    }
    
    void initVulkan()
    {
        createInstance();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyInstance(vkInstance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}