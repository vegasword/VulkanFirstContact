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

#include "my_math.hpp"
#include "my_utils.hpp"

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

    // Devices
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    const std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };

    // Queues
    u32 graphicsFamily;
    VkQueue graphicsQueue;
    u32 presentFamily;
    VkQueue presentQueue;

    // Windowing and rendering
    GLFWwindow* window;
    VkSurfaceKHR surface;

    // Swap Chain
    VkSurfaceCapabilitiesKHR supportedSurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats;
    std::vector<VkPresentModeKHR> supportedPresentModes;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    // Graphics pipeline
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

        
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
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
        
        // We finally create the Vulkan instance.
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
        
        return validProperties
            && requiredFeatures 
            && graphicsFamily != -1
            && presentFamily != -1
            && extensionsSupported
            && swapChainAdequate;
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
        * Surface Format entry contains a format and a colorspace memeber.
        * The format member specifies the color channels and types.
        * The colorSpace member indcates if the SRGB color space is supported.
        * NOTE: For the color space we'll use SRGB if it is available, because
        *       it results in more accurate perceived colors.
        */
        VkSurfaceFormatKHR surfaceFormat{};
        for (const auto& availableFormat : supportedSurfaceFormats)
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
        for (const auto& availablePresentMode : supportedPresentModes)
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
        size_t swapChainImageSize = swapChainImages.size();
        swapChainImageViews.resize(swapChainImageSize);
        for (size_t i = 0; i < swapChainImageSize; i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            // The components field allows us to swizzle the color channels around. 
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // The subresourceRange field describes what the image's purpose is 
            // and which part of the image should be accessed.
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image views");
        }
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

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile("data/vert.spv");
        auto fragShaderCode = readFile("data/frag.spv");

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
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

        //TODO(vegasword): Pass uniform values in shaders (Buffers chapters)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

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
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");

        vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
    }

    void createRenderPass()
    {
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
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
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
        VkRenderPassCreateInfo renderPassInfo{};

        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass");
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
        * The graphics pipeline is the sequence of operations that take the
        * vertices and textures of your meshes all the way to the pixels in
        * the render targets. (https://vulkan-tutorial.com/images/vulkan_simplified_pipeline.svg)
        */
        createGraphicsPipeline();
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
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        for (auto imageView : swapChainImageViews)
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

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