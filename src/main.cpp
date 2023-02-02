#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

#include "vulkan_callbacks.hpp"

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
    VkDebugUtilsMessengerEXT debugMessenger;

#ifdef _DEBUG
    const std::vector<const  char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#endif
    
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void createInstance()
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

        /* (Instance / VkInstanceCreateInfo)
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
        u32 availableExtensionsCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data());

        // Check if the required extensions are supported
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef _DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        const u32 extensionsCount = static_cast<u32>(extensions.size());
        
        for (u32 i = 0; i < extensionsCount; i++)
            for (u32 j = 0; j < availableExtensionsCount; j++)
                if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0)
                    counter++;
        if (counter != extensionsCount)
            throw std::runtime_error("An extension is not supported");
        

        // Enable extensions
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions.data();

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
        
        /*
         * This variable is placed outside the if statement to ensure that it 
         * is not destroyed before the vkCreateInstance call. By creating an 
         * additional debug messenger this way it will automatically be used 
           during vkCreateInstance and vkDestroyInstance and cleaned up after that.
        */
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

#ifdef _DEBUG
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

        // Create the debug messenger context
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif
        
        // We finally create the Vulkan instance.
        if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan instance");
    }

    void setupDebugMessenger()
    {
        // Specifies details about the debug messenger and its callback.
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            throw std::runtime_error("Failed to set up debug messenger!");
    }
    
    void initVulkan()
    {
        createInstance();
#ifdef _DEBUG
        setupDebugMessenger();
#endif
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
#ifdef _DEBUG
        DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
#endif
        vkDestroyInstance(vkInstance, nullptr);
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Application app;

    try
    {
        app.run();
    } catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}