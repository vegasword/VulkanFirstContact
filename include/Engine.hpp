#pragma once

#include <vulkan/vulkan.h>
#include <MyMath.hpp>

#define MAX_FRAMES_IN_FLIGHT 2

class Window;

class Engine
{
public:
    Engine() = default;

	void Create(Window* window);
	void Destroy();
	void Update(Window* window);
	void Draw();

    VkDevice GetLogicalDevice();

private:
    // Instance
    VkInstance m_instance;
    VkApplicationInfo m_appInfo{};

    void createInstance();

    // Devices
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_logicalDevice;
    const std::vector<const char*> m_deviceExtensions = { "VK_KHR_swapchain" };

    bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
    void pickPhysicalDevice();
    void createLogicalDevice();

    // Queue
    u32 m_graphicsFamily;
    VkQueue m_graphicsQueue;
    u32 m_presentFamily;
    VkQueue m_presentQueue;

    void createSurface(Window* window);

    // Windowing
    VkSurfaceKHR m_surface;

    // Depth buffering
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;

    // Texturing
    VkBuffer m_stagingBuffer;
    VkDeviceMemory m_stagingBufferMemory;
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    // Model Buffers
    std::vector<Vertex> m_vertices;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    std::vector<u32> m_indices;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    void createBuffer(VkDeviceSize size, 
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, 
        VkBuffer& buffer, 
        VkDeviceMemory& bufferMemory);

    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();

    // Swap Chain
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    VkSurfaceCapabilitiesKHR m_supportedSurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> m_supportedSurfaceFormats;
    std::vector<VkPresentModeKHR> m_supportedPresentModes;

    void createSwapChain(Window* window);
    void recreateSwapChain(Window* window);
    void cleanupSwapChain();

    // Graphics pipeline
    VkRenderPass m_renderPass;
    VkPipeline m_graphicsPipeline;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkPipelineLayout m_pipelineLayout;

    void createImageViews();
    void createRenderPass();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createCommandPool();
    void createDepthResources();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);
    u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    // Drawing
    u32 m_currentFrame = 0, m_imageIndex = 0;
    
    void createFramebuffers();

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    void createCommandBuffers();
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void loadModel(const char* path);
    void createImage(u32 width, u32 height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image,
        VkDeviceMemory& imageMemory);
    void createTextureImage(const char* path);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createTextureImageView();
    void createTextureSampler();
    void copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height);

    void createDescriptorPool();
    void createDescriptorSets();

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    
    void createSyncObjects();
    void transitionImageLayout(VkImage image, 
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);
};
