#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <set>

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
    // Instancing
    VkInstance instance;
    VkApplicationInfo appInfo{};
    VkDebugUtilsMessengerEXT debugMessenger;

    // Devices
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    // Queues
    u32 graphicsFamily;
    VkQueue graphicsQueue;
    u32 presentFamily;
    VkQueue presentQueue;

    // Windowing and rendering
    GLFWwindow* window;
    VkSurfaceKHR surface;

#ifdef _DEBUG
    u32 validationLayerCount = 0;
    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
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
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Template";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Vulkan Template";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        /* (Instance / VkInstanceCreateInfo)
         * Tells Vulkan driver which global extensions and validation layers
         * we want to use.
         *
         * Global extensions are a set of additional Vulkan features.
         *
         * Validation layers are optional components that hook into Vulkan
         * function calls to apply additional operations like checking the
         * values of parameters against the specification to detect misuse or
         * the thread safety...
         * There are two different types of validation layers:
         * - Instance layers only checking calls related to global Vulkan
         *   objects like instances
         * - Device specific layers checking only calls related to a specific
         *   GPU.
         *
         *  Even if device specific layers have now been deprecated, it still
         *  recommended to enable validation layers at devices level as well
         *  for compatibility, which is required by some implementations.
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

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

#ifdef _DEBUG
        // Get and count the available validation layers from the Vulkan instance
        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Check if all of the layers are available
        counter = 0;
        validationLayerCount = static_cast<u32>(validationLayers.size());
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
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan instance");
    }

    void setupDebugMessenger()
    {
        // Specifies details about the debug messenger and its callback.
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            throw std::runtime_error("Failed to set up debug messenger!");
    }
    
    void createSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
    }

    bool isPhysicalDeviceSuitable(VkPhysicalDevice device)
    {
        // Get physical device hardware properties and Vulkan compatibility
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // I want to choose a compatible and a not integrated GPU
        bool validProperties =
            deviceProperties.apiVersion >= appInfo.apiVersion &&
            deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        // Get the optional features and shaders compatibilities
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        bool requiredFeatures = deviceFeatures.geometryShader;

        /* (About queue families)
        * It has been briefly touched upon before that almost every operation in 
        * Vulkan, anything from drawing to uploading textures, requires commands 
        * to be submitted to a queue. There are different types of queues that 
        * originate from different queue families and each family of queues allows
        * only a subset of commands.
        */ 

        // Check supported queues by the physical device
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
      
        graphicsFamily = presentFamily = -1;
        for (int i = 0; i < queueFamilyCount; i++)
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsFamily = i;

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport) presentFamily = i;

                break;
            }
        
        return validProperties && requiredFeatures && graphicsFamily != -1 && presentFamily != -1;
    }

    void pickPhysicalDevice()
    {       
        // Listing the GPUs
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount <= 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Check devices suitability
        for (const auto& device : devices)
        {
            if (isPhysicalDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find a suitable GPU");
    }

    void createLogicalDevice()
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<u32> uniqueQueueFamilies = { graphicsFamily, presentFamily };
        
        /* (About queue parallelization)
        * We can assign priorities to queues to influence the scheduling of 
        * command buffer execution using floating point numbers between 0.0 and 1.0.
        */
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        for (u32 queueFamiliy : uniqueQueueFamilies)
        {
            queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamiliy;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

#ifdef _DEBUG
        /*
        * Even if Vulkan no longer make a distinction between instance and
        * device specific validation layers, it's still a good idea to set
        * those properties anyway.
        */
        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers.data();
#else
        createInfo.enabledLayerCount = 0;
#endif
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
            throw std::runtime_error("Failed to create logical device");

        vkGetDeviceQueue(logicalDevice, graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, presentFamily, 0, &presentQueue);
    }

    void initVulkan()
    {
        createInstance();

#ifdef _DEBUG
        setupDebugMessenger();
#endif

        /*
        * To establish the connection between Vulkan and the window system to 
        * present results to the screen, we need to use the Window System
        * Integration extensions. We'll use a surface object to present 
        * rendered images to. The surface in our program will be backed by the 
        * window that we've already opened with GLFW.
        */

        createSurface();

        // Vulkan separates the concept of physical and logical devices.
        
        /* 
        * A physical device usually represents a single complete implementation 
        * of Vulkan (excluding instance-level functionality) available to the host,
        * of which there are a finite number.
        */
        pickPhysicalDevice();

        /* 
        * A logical device represents an instance of that implementation with its
        * own state and resources independent of other logical devices.
        */
        createLogicalDevice();
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
        vkDestroyDevice(logicalDevice, nullptr);
#ifdef _DEBUG
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Application app;

    try
    {
        app.run();
    } catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}