#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <set>
#include <array>
#include <chrono>

#include "engine_math.hpp"
#include "utils.hpp"

#define MAX_FRAMES_IN_FLIGHT 2

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
    
    // Instancing
    VkInstance instance;
    VkApplicationInfo appInfo{};
    
    // Devices
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    const std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };
    
    // Queues
    u32 graphicsFamily;
    VkQueue graphicsQueue;
    u32 presentFamily;
    VkQueue presentQueue;
    
    // Windowing
    GLFWwindow* window;
    VkSurfaceKHR surface;
    
    // Basic buffering
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    
    // Texturing
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    
    // TMP buffers
    const std::vector<Vertex> vertices = 
    {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };
    
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
    
    // Swap Chain
    VkSurfaceCapabilitiesKHR supportedSurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats;
    std::vector<VkPresentModeKHR> supportedPresentModes;
    
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    
    // Graphics pipeline
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    VkPipelineLayout pipelineLayout;
    
    // Drawing
    u32 currentFrame = 0;
    bool framebufferResized = false;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
        glfwMaximizeWindow(window);
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
        appInfo.apiVersion = VK_API_VERSION_1_3;
        
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
        
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
        
        // We finally create the Vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan instance");
    }
    
    void createSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);
        
        if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface");
    }
    
    bool isPhysicalDeviceSuitable(VkPhysicalDevice device)
    {
        // Get physical device hardware properties and Vulkan compatibility
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        // I want to choose a Vulkan compatible GPU and not an integrated one
        bool validProperties =
            deviceProperties.apiVersion >= appInfo.apiVersion &&
            deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        
        // Get the optional features and shaders compatibilities
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        bool requiredFeatures = deviceFeatures.geometryShader;
        
        /* (About queue families)
        * Every operation in Vulkan, anything from drawing to uploading textures, 
        * requires commands to be submitted to a queue. 
        * There are different types of queues that originate from different queue
        * families and each family of queues allows only a subset of commands.
        */ 
        
        // Check supported queue families by the physical device
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
        graphicsFamily = presentFamily = -1;
        for (u32 i = 0; i < queueFamilyCount; i++)
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) presentFamily = i;
            
            break;
        }
        
        // Check supported device extensions
        u32 availableExtensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions.data());
        
        u32 counter = 0, extensionCount = static_cast<u32>(deviceExtensions.size());
        for (u32 i = 0; i < extensionCount; i++)
            for (u32 j = 0; j < availableExtensionCount; j++)
            if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0)
            counter++;
        bool extensionsSupported = (counter == extensionCount);
        
        // Check swap chain support
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &supportedSurfaceCapabilities);
            
            u32 surfaceFormatCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, nullptr);
            supportedSurfaceFormats.resize(surfaceFormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, supportedSurfaceFormats.data());
            u32 presentModeCount = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
            supportedPresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, supportedPresentModes.data());
            
            swapChainAdequate = surfaceFormatCount != 0 && presentModeCount != 0;
        }
        
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        
        return validProperties
            && requiredFeatures 
            && graphicsFamily != -1
            && presentFamily != -1
            && extensionsSupported
            && swapChainAdequate
            && supportedFeatures.samplerAnisotropy;
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
        for (const VkPhysicalDevice& device : devices)
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
        createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.enabledLayerCount = 0;
        
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
            throw std::runtime_error("Failed to create logical device");
        
        vkGetDeviceQueue(logicalDevice, graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, presentFamily, 0, &presentQueue);
    }
    
    void createSwapChain()
    {
        /*
        * Surface Format entry contains a format and a colorspace member.
        * The format member specifies the color channels and types.
        * The colorSpace member indcates if the SRGB color space is supported.
        * NOTE: For the color space we'll use SRGB if it is available, because
        *       it results in more accurate perceived colors.
        */
        VkSurfaceFormatKHR surfaceFormat{};
        for (const VkSurfaceFormatKHR& availableFormat : supportedSurfaceFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surfaceFormat = availableFormat;
                break;
            }
        }
        if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
            surfaceFormat = supportedSurfaceFormats[0];
        
        /*
        * Presentation mode is arguably the most important setting for the
        * swap chain, because it represents the actual conditions for showing
        * images to the screen. There are four possible modes available in Vulkan:
        * 
        * - IMMEDIATE: Images are transfered to the screen directly, which may
        *   result in tearing.
        * 
        * - FIFO: Similar to vertical sync, it takes an image from the front of
        *   the queue when the display is refreshed and insert rendered images at the back.
        *   The moment that the display is refreshed is known as "vertical blank".
        * 
        * - FIFO_RELAXED: If the application is late and the queue was empty,
        *   instead of waiting for the next vertical blank, the image is trasferred
        *   right away when it finally arrives. This may result in visible tearing.
        * 
        * - MAILBOX: Instead of blocking the application when the queue is full,
        *   the image that are already queued are simply replaced with the newer ones.
        *   This mode can be used to render frames as fast as possible while still 
        *   avoiding tearing, resulting in fewer latency, commonly know as "triple buffering".
        */
        VkPresentModeKHR presentMode{};
        for (const VkPresentModeKHR& availablePresentMode : supportedPresentModes)
        {
            // We check if MAILBOX is available, because that mode is 
            // the best trade-off if energy usage is not a concern.
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = availablePresentMode;
                break;
            }
        }
        // Otherwise we choose the FIFO mode where energy usage is important
        if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
        
        /*
        * Swap extent is the resolution of the swap chain images and it's almost
        * exactly equal to the resolution of the window that we're drawing in pixels.
        * In some cases, due to high pixel density on a device, the resolution
        * of the window in pixel will be largen than the resolution in screen
        * coordinates.
        */
        VkExtent2D swapExtent{};
        if (supportedSurfaceCapabilities.currentExtent.width != 0xffffffff)
        {
            swapExtent = supportedSurfaceCapabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            swapExtent.width = Clamp(width, 
                                     supportedSurfaceCapabilities.currentExtent.width, 
                                     supportedSurfaceCapabilities.maxImageExtent.width);
            swapExtent.height =Clamp(height,
                                     supportedSurfaceCapabilities.currentExtent.height,
                                     supportedSurfaceCapabilities.maxImageExtent.height);
        }
        
        // We decide how many images we could like to have in the swap chain
        u32 imageCount = supportedSurfaceCapabilities.minImageCount + 1;
        if (supportedSurfaceCapabilities.maxImageCount > 0 &&
            imageCount > supportedSurfaceCapabilities.maxImageCount)
        {
            // It's recommended to request one more image to avoid delay cause by 
            // driver internal operations
            imageCount = supportedSurfaceCapabilities.maxImageCount + 1;
        }
        
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapExtent;
        
        /*
        * The imageArrayLayers specifies the amount of layers each image consists of.
        * This is always 1 unless you are developing a stereoscopic 3D application.
        * The imageUsage bit field specifies what kind of operations we'll use
        * the images in the swap chain for.
        */
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        /*
        * We need to specify how to handle swap chain images that will be used 
        * across multiple queue families.
        * There are two ways to handle images that are accessed from multiple queues:
        * - EXCLUSIVE: An image is owned by one queue family at a time and ownership
        *   must be explicitly transferred before using it in another queue family.
        * - CONCURRENT: Images can be used across multiple queue families without
        *   explicit ownership transfers.
        */
        if (graphicsFamily != presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            u32 queueFamilyIndices[] = { graphicsFamily, presentFamily };
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        
        // We can specify that a certain transform should be applied to images in
        // the swap chain, like clockwise rotation or horizontal flip.
        if (supportedSurfaceCapabilities.supportedTransforms)
            createInfo.preTransform = supportedSurfaceCapabilities.currentTransform;
        
        // The compositeAlpha field specifies if the alpha channel should be used for
        // blending with other windows in the window system.
        if (supportedSurfaceCapabilities.supportedCompositeAlpha)
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE; // Does not care about pixels obstructed by overlapping windows
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        
        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swap chain");
        
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
        
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = swapExtent;
    }
    
    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());
        
        for (uint32_t i = 0; i < swapChainImages.size(); i++)
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
    
    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const u32*>(code.data());
        
        // The shader module is an object wrapping shader code.
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module");
        return shaderModule;
    }
    
    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<u32>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        
        if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout");
    }
    
    void createGraphicsPipeline()
    {
        std::vector<char> vertShaderCode = readFile("data/shaders/vertex_shader.spv");
        std::vector<char> fragShaderCode = readFile("data/shaders/fragment_shader.spv");
        
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        
        // Assign the vertex and fragment shader to its pipeline stage
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        
        VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();
        
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        
        /*
        * The input assembler collects the raw vertex data from the buffers you
        * specify and may also use an index buffer to repeat certain elements
        * without having to duplicate the vertex data itself.
        */
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        /*
        * A viewport basically describes the region of the framebuffer that the 
        * output will be rendered to. While viewports define the transformation 
        * from the image to the framebuffer, scissor rectangles define in which 
        * regions pixels will actually be stored.
        */
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        
        /*
        * The rasterization stage discretizes the primitives into fragments. 
        * These are the pixel elements that they fill on the framebuffer.
        * Any fragments that fall outside the screen are discarded and the 
        * attributes outputted by the vertex shader are interpolated across the fragments
        */
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode =  0; //VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        
        //TODO(vegasword): Multisampling chapter
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        /*
        * The color blending stage applies operations to mix different 
        * fragments that map to the same pixel in the framebuffer.
        * Fragments can simply overwrite each other, add up or be mixed based 
        * upon transparency.
        */
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT | 
            VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT | 
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
        
        // Dynamic states keep properties like viewport size, line width or 
        // blend constants dynamic. Any specified values bellow will be 
        // editable at drawing time.
        std::vector<VkDynamicState> dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        
        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");
        
        // We bind shader stages, fixed-function, pipeline layout and the 
        // render pass to the graphics pipeline.
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        
        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");
        
        vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
    }
    
    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        
        /*
        * The loadOp and storeOp determine what to do with the swap chain images 
        * data in the attachment before rendering and after rendering. 
        * We have the following choices for loadOp and storeOp:
        *   VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
        *   VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
        *   VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
        *   VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
        *   VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
        */
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
        /*
        * Textures and framebuffers in Vulkan are represented by VkImage objects 
        * with a certain pixel format, however the layout of the pixels in memory 
        * can change based on what you're trying to do with an image.
        * Some of the most common layouts are:
        *   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
        *   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
        *   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
        */
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        /*
        * A single render pass can consist of multiple subpasses. 
        * Subpasses are subsequent rendering operations that depend on the contents
        * of framebuffers in previous passes, for example a sequence of post-processing
        * effects that are applied one after another.
        * If you group these rendering operations into one render pass, then Vulkan is 
        * able to reorder the operations and conserve memory bandwidth for possibly better performance.
        * Every subpass references one or more of the attachments.
        */
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        
        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass");
    }
    
    void createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        
        // We iterate through the image views and create framebuffers from them
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = { swapChainImageViews[i] };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            
            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
        
    }
    
    void createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = graphicsFamily;
        
        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create command pool!");
    }
    
    void recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer");
        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;
        
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffer, static_cast<u32>(indices.size()), 1, 0, 0, 0);
        
        vkCmdEndRenderPass(commandBuffer);
        
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer");
    }
    
    u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
    {
        /*
        * The VkPhysicalDeviceMemoryProperties structure has two arrays
        * memoryTypes and memoryHeaps. Memory heaps are distinct memory
        * resources like dedicated VRAM and swap space in RAM for when
        * VRAM runs out. The different types of memory exist within these heaps.
        */
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
        throw std::runtime_error("Failed to find suitable memory type");
    }
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer");
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        
        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer memory");
        
        vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
    }
    
    VkCommandBuffer beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        return commandBuffer;
    }
    
    void endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    }
    
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        endSingleTimeCommands(commandBuffer);
    }
    
    void transitionImageLayout(VkImage image, VkFormat format, 
                               VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        /*
        * Pipeline barriers are primarily used for synchronizing access to resources,
        * like making sure that an image was written to before it is read, but they 
        * can also be used to transition layouts.
        */
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else 
        {
            throw std::invalid_argument("Unsupported layout transition");
        }
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        endSingleTimeCommands(commandBuffer);
    }
    
    void copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1};
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        endSingleTimeCommands(commandBuffer);
    }
    
    void createImage(u32 width, u32 height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image,
                     VkDeviceMemory& imageMemory) 
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        
        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate image memory");
        
        vkBindImageMemory(logicalDevice, image, imageMemory, 0);
    }
    
    void createTextureImage(const char* path)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        if (!pixels) throw std::runtime_error("Failed to load texture image");
        
        createBuffer(imageSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     stagingBuffer, stagingBufferMemory);
        
        void* data; vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice, stagingBufferMemory);
        stbi_image_free(pixels);
        
        createImage(texWidth, texHeight, 
                    VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    textureImage, textureImageMemory);
        
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, textureImage, static_cast<u32>(texWidth), static_cast<u32>(texHeight));
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }
    
    VkImageView createImageView(VkImage image, VkFormat format)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        VkImageView imageView;
        if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image view");
        }
        
        return imageView;
    }
    
    void createTextureImageView()
    {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
    }
    
    void createTextureSampler()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        
        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture sampler!");
    }
    
    void createVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        
        // We're creating a staging buffer to upload vertex array to the device
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, 
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);
        
        // Move the vertex data to the device local buffer
        // using a transfer operation
        createBuffer(bufferSize, 
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                     vertexBuffer, vertexBufferMemory);
        
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
        
        // After copying the data from the staging buffer 
        // to the device buffer, we should clean it up
        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }
    
    void createIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, 
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);
        
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer, indexBufferMemory);
        
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);
        
        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }
    
    void createUniformBuffers() 
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(bufferSize, 
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                         uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(logicalDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }
    
    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        
        if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool");
    }
    
    void createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();
        
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets");
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;
            
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            
            vkUpdateDescriptorSets(logicalDevice, static_cast<u32>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
    
    void createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (u32)commandBuffers.size();
        
        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers");
    }
    
    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphores");
    }
    
    void initVulkan()
    {
        /*
        * The instance is the connection between the application and the Vulkan
        * library depending on the GPU driver configuration.
        */
        createInstance();
        
        /*
        * To establish the connection between Vulkan and the window system to 
        * present results to the screen, we need to use the Window System
        * Integration extensions. We'll use a surface object to present 
        * rendered images to. The surface in our program will be backed by the 
        * window that we've already opened with GLFW.
        */
        createSurface();
        
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
        
        /*
        * The swap chain is essentially a queue of images that are waiting to be 
        * presented to the screen. The general purpose of the swap chain is to 
        * synchronize the presentation of images with the refresh rate of the screen.
        */
        createSwapChain();
        
        /*
        * An image view is quite literally a view into an image. It describes 
        * how to access the image and which part of the image to access, for
        * example if it should be treated as a 2D texture depth texture without
        * any mipmapping levels.
        */
        createImageViews();
        
        // The render pass is an object specifying how images data are managed by the GPU
        createRenderPass();
        
        /*
        * A descriptor is a way for shaders to freely access resources like buffers and images.
        * Usage of descriptors consists of three parts:
        *   - Create a descriptor layout specifying the types of resources
              that are going to be accessed by the pipeline. 
        *   - Allocate a descriptor set from a descriptor pool specifiying 
              the actual buffer or image resources that will be bound
        *   - Bind the descriptor set during rendering
        */
        createDescriptorSetLayout();
        
        /*
        * The graphics pipeline is the sequence of operations that take the
        * vertices and textures of your meshes all the way to the pixels in
        * the render targets. (https://vulkan-tutorial.com/images/vulkan_simplified_pipeline.svg)
        */
        createGraphicsPipeline();
        
        /*
        *  Framebuffers are the combination of all used graphics pipeline buffers
        *  like the color, depth and stencil buffers for example.
        */
        createFramebuffers();
        
        /*
        * Command pools manage the memory that is used to store the buffers and 
        * command buffers are allocated from them. 
        */
        createCommandPool();
        
        createTextureImage("data/textures/fish.png");
        createTextureImageView();
        createTextureSampler();
        
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        
        /*
        * Descriptor sets can't be created directly, they must be allocated from 
        * a pool like command buffers. The equivalent for descriptor sets is
        * called a descriptor pool
        */
        createDescriptorPool();
        createDescriptorSets();
        
        /*
        * Commands in Vulkan, like drawing operations and memory transfers, are
        * not executed directly using function calls. You have to record all of
        * the operations you want to perform in command buffer objects.
        * The advantage of this is that when we are ready to tell the Vulkan what
        * we want to do, all of the commands are submitted together and Vulkan can
        * more efficiently process the commands since all of them are available together.
        * In addition, this allows command recording to happen in multiple threads if so desired.
        */
        createCommandBuffers();
        
        // Create graphic semaphore and inflight fences for rendering.
        createSyncObjects();
    }
    
    void recreateSwapChain() 
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) 
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        
        vkDeviceWaitIdle(logicalDevice);
        createSwapChain();
        createImageViews();
        createFramebuffers();
    }
    
    void updateUniformBuffer(u32 currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        
        UniformBufferObject ubo{};
        float cameraDist = 1.f, rotatoSpeed = 2.f;
        ubo.model = glm::rotate(glm::mat4(1.0f), time * rotatoSpeed * glm::radians(90.0f), glm::vec3(-2.5f, -1.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(cameraDist, cameraDist, cameraDist), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        
        /*
        * GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
        * The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the 
        * projection matrix. If we don't do this, then the image will be rendered upside down.
        */
        ubo.proj[1][1] *= -1;
        
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
    
    void drawFrame()
    {
        vkWaitForFences(logicalDevice, 1, inFlightFences.data(), VK_TRUE, UINT64_MAX);
        
        u32 imageIndex;
        VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image");
        }
        
        updateUniformBuffer(currentFrame);
        
        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer");
        
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        
        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        
        presentInfo.pImageIndices = &imageIndex;
        
        vkQueuePresentKHR(presentQueue, &presentInfo);
        
        // By using the modulo (%) operator, we ensure that the frame index loops
        // around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    
    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            
            /*
            * Rendering a frame in Vulkan consists of a common set of steps:
            *   Wait for the previous frame to finish
            *   Acquire an image from the swap chain
            *   Record a command buffer which draws the scene onto that image
            *   Submit the recorded command buffer
            *   Present the swap chain image
            */
            drawFrame();
        }
        vkDeviceWaitIdle(logicalDevice);
    }
    
    void cleanupSwapChain() 
    {
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
            vkDestroyFramebuffer(logicalDevice, swapChainFramebuffers[i], nullptr);
        
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
            vkDestroyImageView(logicalDevice, swapChainImageViews[i], nullptr);
        
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
    }
    
    void cleanup()
    {
        cleanupSwapChain();
        
        vkDestroySampler(logicalDevice, textureSampler, nullptr);
        vkDestroyImageView(logicalDevice, textureImageView, nullptr);
        vkDestroyImage(logicalDevice, textureImage, nullptr);
        vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, uniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
        
        vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
        
        vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
        vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
        
        vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
        vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
        
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
        }
        
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        
        vkDestroyDevice(logicalDevice, nullptr);
        
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        
        glfwDestroyWindow(window);
        
        glfwTerminate();
    }
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

int main()
{
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

//TODO: Create one VkBuffer for vertices and indices (single memory allocation)