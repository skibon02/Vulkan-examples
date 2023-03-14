
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

void initVulkan(GLFWwindow * window) {
    VkInstance instance;

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

    const char* validationLayers[] = {
            "VK_LAYER_KHRONOS_validation"
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    const char* debugUtilsExtension = "VK_EXT_debug_utils";
    extensions.push_back(debugUtilsExtension);

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;


    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        cout << "Failed to create instance" << endl;
        exit(-1);
    }

    //create debug messenger
    VkDebugUtilsMessengerEXT debugMessenger;

    //create mesenger
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        cout << "Failed to create debug messenger" << endl;
        exit(-1);
    }


    //find physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        cout << "Failed to find GPUs with Vulkan support" << endl;
        exit(-1);
    }
    vector<VkPhysicalDevice> devices(deviceCount);
    vector<VkPhysicalDeviceProperties> deviceProperties(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDevice physicalDevice = devices[0];

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

    cout << "Physical device: " << deviceProperties[physicalDeviceIndex].deviceName << endl;


    //create surface
    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        cout << "Failed to create window surface" << endl;
        exit(-1);
    }

    //find queue family
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, surface, &presentSupport);

        if (presentSupport) {
            presentQueueFamilyIndex = i;
        }

        if (graphicsQueueFamilyIndex != UINT32_MAX && presentQueueFamilyIndex != UINT32_MAX) {
            break;
        }
        i++;
    }

    if (graphicsQueueFamilyIndex == UINT32_MAX) {
        cout << "Failed to find graphics queue family" << endl;
        exit(-1);
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

    VkDevice device;
    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = 0;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        cout << "Failed to create logical device" << endl;
        exit(-1);
    }

    // get queues
    VkQueue graphicsQueue, presentQueue;

    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);


}

int main() {
    GLFWwindow * window;
    if(!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    if(!window) {
        cout << "Failed to create window" << endl;
        return -1;
    }

    if (glfwVulkanSupported()) {
        cout << "Vulkan is supported" << endl;
    }
    else {
        cout << "Vulkan is not supported" << endl;
        return -1;
    }

    initVulkan(window);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        //drawFrame();
    }


    return 0;
}
