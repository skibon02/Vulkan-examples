
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <set>
#include <optional>
#include <fstream>

using namespace std;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

struct VulkanStuff {
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR surfaceFormat;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence frameFence;
};

VkShaderModule createShaderModule(VkDevice device, const char* filename) {
    ifstream file(filename, ios::ate | ios::binary);
    if (!file.is_open()) {
        cout << "Failed to open file" << endl;
        exit(-1);
    }
    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        cout << "Failed to create shader module" << endl;
        exit(-1);
    }

    return shaderModule;
}

VulkanStuff initVulkan(GLFWwindow* window) {
    VulkanStuff vk;

    //validation layers
    const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    //create instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    //add debugUtils extension
    const char* debugUtilsExtension = "VK_EXT_debug_utils";
    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(debugUtilsExtension);

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;

    if (vkCreateInstance(&createInfo, nullptr, &vk.instance) != VK_SUCCESS) {
        cout << "Failed to create instance" << endl;
        exit(-1);
    }

    //create surface
    if (glfwCreateWindowSurface(vk.instance, window, nullptr, &vk.surface) != VK_SUCCESS) {
        cout << "Failed to create surface" << endl;
        exit(-1);
    }

    //create mesenger
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(vk.instance, &debugCreateInfo, nullptr, &vk.debugMessenger) != VK_SUCCESS) {
        cout << "Failed to create debug messenger" << endl;
        exit(-1);
    }


    //find physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        cout << "Failed to find GPUs with Vulkan support" << endl;
        exit(-1);
    }
    vector<VkPhysicalDevice> devices(deviceCount);
    vector<VkPhysicalDeviceProperties> deviceProperties(deviceCount);
    vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices.data());
    
    vk.physicalDevice = devices[0];

        //select appropriate device
        for (int i = 0; i < deviceCount; i++) {
            vkGetPhysicalDeviceProperties(devices[i], &deviceProperties[i]);
        }

        int physicalDeviceIndex = -1;
        // priority 1:  discrete gpu
        for (int i = 0; i < deviceCount; i++) {
            if (deviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                physicalDeviceIndex = i;
                cout << "Discrete GPU found!" << endl;
                break;
            }
        }

        // priority 2:  integrated gpu
        if (physicalDeviceIndex == -1)
            for (int i = 0; i < deviceCount; i++) {
                if (deviceProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                    physicalDeviceIndex = i;
                    cout << "Integrated GPU found!" << endl;
                    break;
                }
            }

        if(physicalDeviceIndex == -1) {
            cout << "No discrete/integrated GPU found!" << endl;
        }

    vk.physicalDevice = devices[physicalDeviceIndex];
    cout << "Selected GPU: " << deviceProperties[physicalDeviceIndex].deviceName << endl;

    //find queue families
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( vk.physicalDevice, i, vk.surface, &presentSupport);

        if (presentSupport) {
            presentQueueFamilyIndex = i;
        }

        if (graphicsQueueFamilyIndex != UINT32_MAX && presentQueueFamilyIndex != UINT32_MAX) {
            break;
        }
        i++;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    //add swapchain extension
    const char* swapchainExtension = "VK_KHR_swapchain";
    vector<const char*> deviceExtensions;
    deviceExtensions.push_back(swapchainExtension);


    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.ppEnabledLayerNames = validationLayers;

    if (vkCreateDevice(vk.physicalDevice, &deviceCreateInfo, nullptr, &vk.device) != VK_SUCCESS) {
        cout << "Failed to create logical device" << endl;
        exit(-1);
    }

    vkGetDeviceQueue(vk.device, graphicsQueueFamilyIndex, 0, &vk.graphicsQueue);
    vkGetDeviceQueue(vk.device, presentQueueFamilyIndex, 0, &vk.presentQueue);

    //create swapchain
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &vk.surfaceCapabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, nullptr);
    vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, formats.data());
    //find surfaceFormat
    vk.surfaceFormat = formats[0];
    for(auto &format : formats) {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            vk.surfaceFormat = format;
            break;
        }
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, nullptr);
    vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, presentModes.data());

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = vk.surface;
    swapchainCreateInfo.minImageCount = vk.surfaceCapabilities.minImageCount;
    swapchainCreateInfo.imageFormat = vk.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = vk.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = vk.surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if( graphicsQueueFamilyIndex == presentQueueFamilyIndex) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    swapchainCreateInfo.preTransform = vk.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    auto create_res = vkCreateSwapchainKHR(vk.device, &swapchainCreateInfo, nullptr, &vk.swapchain);
    if (create_res != VK_SUCCESS) {
        cout << "Failed to create swapchain" << endl;
        exit(-1);
    }

    //create image views
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &imageCount, nullptr);
    vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &imageCount, images.data());

    vk.swapchainImageViews.resize(imageCount);
    for (int i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = vk.surfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vk.device, &imageViewCreateInfo, nullptr, &vk.swapchainImageViews[i]) != VK_SUCCESS) {
            cout << "Failed to create image view" << endl;
        }
    }

    //create render pass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = vk.surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(vk.device, &renderPassCreateInfo, nullptr, &vk.renderPass) != VK_SUCCESS) {
        cout << "Failed to create render pass" << endl;
        exit(-1);
    }

    //create graphics pipeline
    VkShaderModule vertShaderModule = createShaderModule(vk.device, "shaders/vert.spv");
    VkShaderModule fragShaderModule = createShaderModule(vk.device, "shaders/frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vk.surfaceCapabilities.currentExtent.width;
    viewport.height = (float)vk.surfaceCapabilities.currentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = vk.surfaceCapabilities.currentExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &vk.pipelineLayout) != VK_SUCCESS) {
        cout << "Failed to create pipeline layout" << endl;
        exit(-1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = vk.pipelineLayout;
    pipelineInfo.renderPass = vk.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk.graphicsPipeline) != VK_SUCCESS) {
        cout << "Failed to create graphics pipeline" << endl;
        exit(-1);
    }

    vkDestroyShaderModule(vk.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(vk.device, fragShaderModule, nullptr);

    //create framebuffers
    vk.swapchainFramebuffers.resize(vk.swapchainImageViews.size());
    for (size_t i = 0; i < vk.swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
            vk.swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vk.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vk.surfaceCapabilities.currentExtent.width;
        framebufferInfo.height = vk.surfaceCapabilities.currentExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &vk.swapchainFramebuffers[i]) != VK_SUCCESS) {
            cout << "Failed to create framebuffer" << endl;
            exit(-1);
        }
    }

    //create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(vk.device, &poolInfo, nullptr, &vk.commandPool) != VK_SUCCESS) {
        cout << "Failed to create command pool" << endl;
        exit(-1);
    }

    //create command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vk.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(vk.device, &allocInfo, &vk.commandBuffer) != VK_SUCCESS) {
        cout << "Failed to allocate command buffer" << endl;
        exit(-1);
    }

    //create sync objects
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vk.device, &fenceInfo, nullptr, &vk.frameFence) != VK_SUCCESS) {
        cout << "Failed to create sync objects" << endl;
        exit(-1);
    }

    return vk;
}

void draw(VulkanStuff& vk) {
    vkWaitForFences(vk.device, 1, &vk.frameFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.frameFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);



    //record command buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(vk.commandBuffer, &beginInfo) != VK_SUCCESS) {
        cout << "Failed to begin recording command buffer" << endl;
        exit(-1);
    }

    VkClearColorValue clearColor = {0.95f, 0.7f, 0.8f, 1.0f};
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vk.renderPass;
    renderPassInfo.framebuffer = vk.swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vk.surfaceCapabilities.currentExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphicsPipeline);
    vkCmdDraw(vk.commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(vk.commandBuffer);

    if (vkEndCommandBuffer(vk.commandBuffer) != VK_SUCCESS) {
        cout << "Failed to record command buffer" << endl;
        exit(-1);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { vk.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk.commandBuffer;

    VkSemaphore signalSemaphores[] = { vk.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.frameFence) != VK_SUCCESS) {
        cout << "Failed to submit draw command buffer" << endl;
        exit(-1);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { vk.swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(vk.presentQueue, &presentInfo);
}

int main() {
    GLFWwindow* window;
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }
    //query vulkan support
    if (glfwVulkanSupported()) {
        cout << "Vulkan is supported" << endl;
    }
    else {
        cout << "Vulkan is not supported" << endl;
        return -1;
    }
    auto vk = initVulkan(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        draw(vk);

    }


    return 0;
}