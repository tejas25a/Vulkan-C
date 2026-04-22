#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN

#include "img_lib.h"
#include "obj_lib.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define true 1
#define false 0

/*Validation Layer*/
#define NumberOfValidationLayer                                                \
  1 /*Must be updated if validationLayers array is changed*/

const char *validationLayers[NumberOfValidationLayer] = {
    "VK_LAYER_KHRONOS_validation"}; /*Must be updated if validationLayers
                                       array is changed*/

#define NumberOfDeviceExtension 1
const char *deviceExtensions[NumberOfDeviceExtension] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const int enableValidationLayers = false;
#else
const int enableValidationLayers = true;
#endif

/*Global Variable*/

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char *MODLE_PATH = "models/model.obj";
const char *TEXTURE_PATH = "texture/model_tex.bmp";

GLFWwindow *window;
VkInstance instance;
VkPhysicalDevice dev = VK_NULL_HANDLE;
VkDevice Ldevice;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSurfaceKHR surface;
VkSwapchainKHR swapChain;
VkImage *swapChainImages;
uint32_t swapChainImageCount;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImageView *swapChainImageViews;
VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;
VkRenderPass renderPass;
VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkFramebuffer *swapChainFramebuffers;
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers;
VkSemaphore *imageAvailableSemaphores;
VkSemaphore *renderFinishedSemaphores;
VkFence *inFlightFence;
const int MAX_FRAME_IN_FLIGHT = 2;
uint32_t currentFrame = 0;
int framebufferResized = false;
VkBuffer VertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
VkBuffer *uniformBuffers;
VkDeviceMemory *uniformBuffersMemory;
void **uniformBuffersMapped;
VkDescriptorPool descriptorPool;
VkDescriptorSet *descriptorSets;
VkImage textureImage;
VkImageView textureImageView;
VkSampler textureSampler;
VkDeviceMemory textureImageMemory;
VkDynamicState *dynamicStates;
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

typedef struct QueueFamilyIndices {
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  int hasGraphicsFamily;
  int hasPresentFamily;
} QueueFamilyIndices;

typedef struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  uint32_t formatCount;
  VkPresentModeKHR *presentModes;
  uint32_t presentModeCount;
} SwapChainSupportDetails;

typedef struct FileBuffer {
  char *data;
  size_t size;
} FileBuffer;

typedef struct Vertex {
  vec3 pos;
  vec3 color;
  vec2 texCoord;
} Vertex;

typedef struct UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} UniformBufferObject;

Vertex *vertices;
uint32_t vertexCount;
uint32_t *indices;
uint32_t indexCount;

VkVertexInputBindingDescription getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription = {0};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

VkVertexInputAttributeDescription *getAttributeDescription() {
  VkVertexInputAttributeDescription *attributeDescription =
      malloc(3 * sizeof(VkVertexInputAttributeDescription));
  attributeDescription[0].binding = 0;
  attributeDescription[0].location = 0;
  attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[0].offset = offsetof(Vertex, pos);

  attributeDescription[1].binding = 0;
  attributeDescription[1].location = 1;
  attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[1].offset = offsetof(Vertex, color);

  attributeDescription[2].binding = 0;
  attributeDescription[2].location = 2;
  attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescription[2].offset = offsetof(Vertex, texCoord);

  return attributeDescription;
}

FileBuffer readFile(const char *filename) {
  FileBuffer result = {NULL, 0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open file %s\n", filename);
    return result;
  }

  fseek(file, 0, SEEK_END);
  result.size = ftell(file);
  rewind(file); // ssek beginning

  result.data = malloc(result.size);
  if (!result.data) {
    fprintf(stderr, "Failed to allocate buffer for file\n");
    fclose(file);
    return result;
  }

  fread(result.data, 1, result.size, file);
  fclose(file);

  return result;
}

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  framebufferResized = true;
}

int checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);

  VkLayerProperties availableLayers[layerCount];
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

  printf("\nActive Validation Layer:\n");
  for (int i = 0; i < NumberOfValidationLayer; ++i) {
    int layerFound = false;

    for (uint32_t j = 0; j < layerCount; ++j) {
      if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
        layerFound = true;
        printf("Name:%s\nSpec Version:%d\nImplementation "
               "Version:%d\nDesc:%s\n\n",
               availableLayers[j].layerName, availableLayers[j].specVersion,
               availableLayers[j].implementationVersion,
               availableLayers[j].description);
        printf("\n");
        break;
      }
    }

    if (!layerFound)
      return false;
  }

  return true;
}

VkApplicationInfo createInstance() {

  if (enableValidationLayers && !checkValidationLayerSupport()) {
    fprintf(stderr, "Validation layer requested, but not available!");
  }

  VkApplicationInfo appInfo;
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = NULL;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;
  return appInfo;
}

void createSurface() {
  if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create window surface!\n");
  }
  printf("created surface\n");
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices = {0};
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies);

  // Finding atleast one family which supports VK_QUEUE_GRAPHICS_BIT
  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      //  VK_QUEUE_GRAPHICS_BIT = 0x00000001,
      indices.graphicsFamily = i;
      indices.hasGraphicsFamily = true;
    }
    if (presentSupport) {
      indices.presentFamily = i;
      indices.hasPresentFamily = true;
    }
  }
  return indices;
}

int checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
  VkExtensionProperties availableExtensions[extensionCount];
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount,
                                       availableExtensions);

  for (uint32_t i = 0; i < NumberOfDeviceExtension; ++i) {
    int found_flag = false;

    for (uint32_t j = 0; j < extensionCount; ++j) {
      if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) ==
          0) {
        found_flag = true;
        break;
      }
    }

    if (!found_flag) {
      printf("Extension not found: %s\n", deviceExtensions[i]);
      return false;
    }
  }

  printf("All device extensions found\n");
  return true;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details = {0};

  // Surface capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  // Surface formats
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

  if (formatCount != 0) {
    details.formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    if (!details.formats) {
      fprintf(stderr, "Error allocating space for details.format");
      return details;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats);
    details.formatCount = formatCount;
  }

  // presentation support
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            NULL);

  if (presentModeCount != 0) {
    details.presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
    if (!details.presentModes) {
      fprintf(stderr, "Error allocating space for details.presentModes");
      return details;
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes);
    details.presentModeCount = presentModeCount;
  }
  return details;
}

int isDeviceSuitable(VkPhysicalDevice device) {

  VkPhysicalDeviceProperties properties = {0};
  vkGetPhysicalDeviceProperties(device, &properties);

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  QueueFamilyIndices indices = findQueueFamilies(device);

  // if (indices.hasGraphicsFamily && indices.hasPresentFamily &&
  //     checkDeviceExtensionSupport(device)) {
  //   // check for swapchain support
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
  int swapChainCheck = 0;
  if (swapChainSupport.formatCount && swapChainSupport.presentModeCount) {
    free(swapChainSupport.presentModes);
    free(swapChainSupport.formats);
    swapChainCheck = true;
  }

  else {
    free(swapChainSupport.presentModes);
    free(swapChainSupport.formats);
  }
  return indices.hasGraphicsFamily && indices.hasPresentFamily &&
         checkDeviceExtensionSupport(device) &&
         supportedFeatures.samplerAnisotropy && swapChainCheck;
}

void pickPhysicalDevice() {

  printf("Entered");
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

  if (deviceCount == 0) {
    fprintf(stderr, "Failed to find GPUs with Vulkan support!");
  }

  VkPhysicalDevice devices[deviceCount];
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

  if (deviceCount == 1) {
    dev = devices[0];
  }

  else {
    for (uint32_t i = 0; i < deviceCount; ++i) {
      if (isDeviceSuitable(devices[i])) {
        dev = devices[i];
        break;
      }
    }
  }

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(dev, &deviceProperties);

  if (enableValidationLayers) {
    printf("Device Name:\n%s\n", deviceProperties.deviceName);
  }
  if (dev == VK_NULL_HANDLE) {
    fprintf(stderr, "Failed to find a suitable GPU!");
  }
}

void createLogicalDevice() {
  printf("finding queue families...\n");
  QueueFamilyIndices indices = findQueueFamilies(dev);
  printf("queueFamilyIndex: %u\n", indices.graphicsFamily);

  uint32_t uniqueFamilies[2];
  uint32_t uniqueCount = 0;

  uniqueFamilies[uniqueCount++] = indices.graphicsFamily;

  if (indices.presentFamily != indices.graphicsFamily) {
    uniqueFamilies[uniqueCount++] = indices.presentFamily;
  }

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfos[2];

  for (uint32_t i = 0; i < uniqueCount; i++) {
    // specifying the queues to be created
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.pNext = NULL;
    queueCreateInfo.flags = 0;
    queueCreateInfo.queueFamilyIndex = uniqueFamilies[i];
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos[i] = queueCreateInfo;
  }

  // specifying used device features
  VkPhysicalDeviceFeatures deviceFeatures = {0};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  // create the logical device
  VkDeviceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.pQueueCreateInfos = queueCreateInfos;
  createInfo.queueCreateInfoCount = uniqueCount;
  createInfo.pEnabledFeatures = &deviceFeatures;

  // Validation layer for device
  createInfo.enabledExtensionCount = NumberOfDeviceExtension;
  createInfo.ppEnabledExtensionNames = deviceExtensions;

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = NumberOfValidationLayer;
    createInfo.ppEnabledLayerNames = validationLayers;
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkResult result = vkCreateDevice(dev, &createInfo, NULL, &Ldevice);

  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!");
  }

  // Using vkGetDeviceQueue function to retrive queue handles for each queue
  // family

  vkGetDeviceQueue(Ldevice, indices.graphicsFamily, 0, &graphicsQueue);
  vkGetDeviceQueue(Ldevice, indices.presentFamily, 0, &presentQueue);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats,
                                           uint32_t formatCount) {
  for (uint32_t i = 0; i < formatCount; ++i) {
    if (availableFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB &&
        availableFormats[i].colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { // VK_COLORSPACE_SRGB_NONLINEAR_KHR
                                                 // for older version fo the
                                                 // spec else use
                                                 // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
      return availableFormats[i];
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR *availablePresentModes,
                                       uint32_t presentModeCount) {

  /*
  for (uint32_t i = 0; i < presentModeCount; ++i) {
    if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      return availablePresentModes[i];
  }
  */
  return VK_PRESENT_MODE_FIFO_KHR;
}
static uint32_t clamp_u32(uint32_t val, uint32_t min, uint32_t max) {
  if (val < min)
    return min;
  if (val > max)
    return max;
  return val;
}
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        clamp_u32((uint32_t)width, capabilities.minImageExtent.width,
                  capabilities.maxImageExtent.width),
        clamp_u32((uint32_t)height, capabilities.minImageExtent.height,
                  capabilities.maxImageExtent.height)};

    return actualExtent;
  }
}

void createSwapChain() {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
      swapChainSupport.formats, swapChainSupport.formatCount);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(
      swapChainSupport.presentModes, swapChainSupport.presentModeCount);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount +
                        1; // sticking to min means sometimes waiting for driver
                           // to complete internal operation before we acqire
                           // another image to render to. So +1

  // 0 here means no max
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {0};

  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(dev);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily,
                                   indices.presentFamily};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;  // optional
    createInfo.pQueueFamilyIndices = NULL; // optional
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult result =
      vkCreateSwapchainKHR(Ldevice, &createInfo, NULL, &swapChain);

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
  free(swapChainSupport.formats);
  free(swapChainSupport.presentModes);

  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swap chain");
  }
  printf("Created swap chain!\n");

  vkGetSwapchainImagesKHR(Ldevice, swapChain, &swapChainImageCount, NULL);
  swapChainImages = malloc(swapChainImageCount * sizeof(VkImage));
  if (!swapChainImages) {
    fprintf(stderr, "Failed to allocate swapChainImages");
  }
  vkGetSwapchainImagesKHR(Ldevice, swapChain, &swapChainImageCount,
                          swapChainImages);
}

VkImageView createImageView(VkImage image, VkFormat format,
                            VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo viewInfo = {0};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(Ldevice, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create texture image view!");
  }

  return imageView;
}

void createImageViews() {
  swapChainImageViews = malloc(swapChainImageCount * sizeof(VkImageView));

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    swapChainImageViews[i] = createImageView(
        swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
  }
  printf("Created image view successfully\n");
}

VkShaderModule createShaderModule(FileBuffer code) {
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.codeSize = code.size;
  createInfo.pCode = (const uint32_t *)code.data;

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(Ldevice, &createInfo, NULL, &shaderModule) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create shader module!\n");
  }
  return shaderModule;
}

VkFormat findSupportedFormat(VkFormat *candidates, uint32_t count,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features) {
  for (uint32_t i = 0; i < count; i++) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(dev, candidates[i], &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return candidates[i];
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return candidates[i];
    }
  }
  fprintf(stderr, "Failed to find supported formats!\n");
  return VK_FORMAT_UNDEFINED;
}

VkFormat findDepthFormat() {
  VkFormat checkFormat[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT};

  int formatSize = sizeof(checkFormat) / sizeof(checkFormat[0]);

  return findSupportedFormat(checkFormat, formatSize, VK_IMAGE_TILING_OPTIMAL,
                             VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.flags = 0;
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {0};
  depthAttachment.format = findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {0};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};

  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.flags = 0;
  renderPassInfo.attachmentCount = 2;
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(Ldevice, &renderPassInfo, NULL, &renderPass) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create render pass!\n");
  }
  printf("Created render pass successfully\n");
}

void createaDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = NULL;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = NULL;

  VkDescriptorSetLayoutBinding bindings[2] = {uboLayoutBinding,
                                              samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.pNext = NULL;
  layoutInfo.flags = 0;
  layoutInfo.bindingCount = 2;
  layoutInfo.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(Ldevice, &layoutInfo, NULL,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create descriptor set layout!\n");
  }
  printf("created descriptor set layout successfully\n");
}

void createGraphicsPipeline() {
  FileBuffer vertShaderCode = readFile("shaders/vert.spv");
  FileBuffer fragShaderCode = readFile("shaders/frag.spv");

  if (!vertShaderCode.data || !fragShaderCode.data) {
    fprintf(stderr, "Shader load failed\n");
    exit(1);
  }

  vertShaderModule = createShaderModule(vertShaderCode);
  fragShaderModule = createShaderModule(fragShaderCode);

  free(vertShaderCode.data);
  free(fragShaderCode.data);

  printf("Created Shader module successfully\n");

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.pNext = NULL;
  vertShaderStageInfo.flags = 0;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";
  vertShaderStageInfo.pSpecializationInfo = NULL;

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.pNext = NULL;
  fragShaderStageInfo.flags = 0;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";
  fragShaderStageInfo.pSpecializationInfo = NULL;

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  uint32_t dynamicstatesCount = 2; // needs to be updated everytime
  dynamicStates = malloc(dynamicstatesCount * sizeof(VkDynamicState));
  dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
  dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

  VkPipelineDynamicStateCreateInfo dynamicState = {0};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.pNext = NULL;
  dynamicState.flags = 0;
  dynamicState.dynamicStateCount = dynamicstatesCount;
  dynamicState.pDynamicStates = dynamicStates;

  VkVertexInputBindingDescription bindingDescription = getBindingDescription();
  VkVertexInputAttributeDescription *attributeDescription =
      getAttributeDescription();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pNext = NULL;
  vertexInputInfo.flags = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 3;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescription;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.pNext = NULL;
  inputAssembly.flags = 0;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = NULL;
  viewportState.flags = 0;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.pNext = NULL;
  rasterizer.flags = 0;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};

  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.pNext = NULL;
  multisampling.flags = 0;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = NULL;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.pNext = NULL;
  depthStencil.flags = 0;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f;
  depthStencil.maxDepthBounds = 1.0f;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = NULL;
  colorBlending.flags = 0;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pNext = NULL;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;

  if (vkCreatePipelineLayout(Ldevice, &pipelineLayoutInfo, NULL,
                             &pipelineLayout) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create pipeline layout!\n");
  }

  printf("Created pipeline layout\n");

  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = NULL;
  pipelineInfo.flags = 0;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pTessellationState = NULL;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(Ldevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
                                &graphicsPipeline) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create graphics pipeline!\n");
  }
  printf("Created graphics pipeline\n");

  free(attributeDescription);
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(dev, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }
  fprintf(stderr, "Failed to find suitable memory type!\n");
  return UINT32_MAX;
}

void createImage(uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlagBits properties, VkImage *image,
                 VkDeviceMemory *imageMemory) {
  VkImageCreateInfo imageInfo = {0};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.pNext = NULL;
  imageInfo.flags = 0;
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

  if (vkCreateImage(Ldevice, &imageInfo, NULL, image) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create image!\n");
  }

  VkMemoryRequirements memRequirement;
  vkGetImageMemoryRequirements(Ldevice, *image, &memRequirement);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = NULL;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirement.memoryTypeBits, properties);
  allocInfo.allocationSize = memRequirement.size;

  if (vkAllocateMemory(Ldevice, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate memory for image!\n");
  }

  vkBindImageMemory(Ldevice, *image, *imageMemory, 0);
}

int hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkCommandBuffer beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.pNext = NULL;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(Ldevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  beginInfo.pNext = NULL;
  beginInfo.pInheritanceInfo = NULL;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = NULL;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.signalSemaphoreCount = 0;
  submitInfo.pSignalSemaphores = NULL;
  submitInfo.waitSemaphoreCount = 0;
  submitInfo.pWaitSemaphores = NULL;
  submitInfo.pWaitDstStageMask = NULL;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(Ldevice, commandPool, 1, &commandBuffer);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyregion = {0};
  copyregion.srcOffset = 0;
  copyregion.dstOffset = 0;
  copyregion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyregion);

  endSingleTimeCommands(commandBuffer);
}

void transitionImageLayout(VkImage image, VkFormat format,
                           VkImageLayout oldlayout, VkImageLayout newlayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldlayout;
  barrier.newLayout = newlayout;
  barrier.pNext = NULL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  if (newlayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (oldlayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newlayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldlayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newlayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldlayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newlayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    fprintf(stderr, "ERROR: unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL,
                       0, NULL, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void createDepthResources() {
  VkFormat depthFormat = findDepthFormat();
  if (depthFormat == -1) {
    fprintf(stderr, "failed finding depth format");
    return;
  }

  createImage(
      swapChainExtent.width, swapChainExtent.height, depthFormat,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory);
  depthImageView =
      createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

  transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void createframebuffers() {
  swapChainFramebuffers = malloc(swapChainImageCount * sizeof(VkFramebuffer));

  for (uint32_t i = 0; i < swapChainImageCount; ++i) {
    VkImageView attachments[2] = {swapChainImageViews[i], depthImageView};

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = NULL;
    framebufferInfo.flags = 0;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(Ldevice, &framebufferInfo, NULL,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create framebuffer\n");
    }
  }

  printf("Created framebuffer successfully\n");
}

void cleanupSwpaChain() {
  vkDestroyImageView(Ldevice, depthImageView, NULL);
  vkDestroyImage(Ldevice, depthImage, NULL);
  vkFreeMemory(Ldevice, depthImageMemory, NULL);

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroyFramebuffer(Ldevice, swapChainFramebuffers[i], NULL);
  }
  free(swapChainFramebuffers);

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroyImageView(Ldevice, swapChainImageViews[i], NULL);
  }
  free(swapChainImageViews);
  free(swapChainImages);
  vkDestroySwapchainKHR(Ldevice, swapChain, NULL);
}

void recreateSwapChain() {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(Ldevice);
  cleanupSwpaChain();

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroySemaphore(Ldevice, renderFinishedSemaphores[i], NULL);
  }
  free(renderFinishedSemaphores);

  createSwapChain();
  createImageViews();
  createDepthResources();
  createframebuffers();
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  renderFinishedSemaphores = malloc(swapChainImageCount * sizeof(VkSemaphore));
  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkCreateSemaphore(Ldevice, &semaphoreInfo, NULL,
                      &renderFinishedSemaphores[i]);
  }
}

void createCommandPool() {
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(dev);

  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.pNext = NULL;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

  if (vkCreateCommandPool(Ldevice, &poolInfo, NULL, &commandPool) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create command pool!\n");
  }
  printf("Created command pool successfully\n");
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer *buffer,
                  VkDeviceMemory *bufferMemory) {
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.flags = 0;
  bufferInfo.pNext = NULL;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferInfo.queueFamilyIndexCount = 0;
  bufferInfo.pQueueFamilyIndices = NULL;

  if (vkCreateBuffer(Ldevice, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vertex buffer!\n");
  }
  printf("Created Vertex buffer successfully\n");

  VkMemoryRequirements memRequirement;
  vkGetBufferMemoryRequirements(Ldevice, *buffer, &memRequirement);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = NULL;
  allocInfo.allocationSize = memRequirement.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirement.memoryTypeBits, properties);

  if (vkAllocateMemory(Ldevice, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate buffer memory");
  }

  printf("Allocated buffer memory\n");

  vkBindBufferMemory(Ldevice, *buffer, *bufferMemory, 0);
  printf("Binded buffer memory\n");
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                       uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  VkOffset3D imageOffset = {0, 0, 0};
  VkExtent3D imageExtent = {width, height, 1};

  region.imageOffset = imageOffset;
  region.imageExtent = imageExtent;

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

void createTextureImage() {
  Image img = loadImage(TEXTURE_PATH);

  if (!img.pixels) {
    fprintf(stderr, "Failed to load image!\n");
    return;
  }

  VkDeviceSize imagesSize = img.size;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(imagesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(Ldevice, stagingBufferMemory, 0, imagesSize, 0, &data);
  memcpy(data, img.pixels, (size_t)imagesSize);
  vkUnmapMemory(Ldevice, stagingBufferMemory);

  createImage(
      img.width, img.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage, &textureImageMemory);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stagingBuffer, textureImage, img.width, img.height);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  freeImage(&img);
  vkDestroyBuffer(Ldevice, stagingBuffer, NULL);
  vkFreeMemory(Ldevice, stagingBufferMemory, NULL);
}

void createTextureImageView() {
  textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                                     VK_IMAGE_ASPECT_COLOR_BIT);
}

void createTextureSampler() {
  VkPhysicalDeviceProperties properties = {0};
  vkGetPhysicalDeviceProperties(dev, &properties);

  VkSamplerCreateInfo samplerInfo = {0};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (vkCreateSampler(Ldevice, &samplerInfo, NULL, &textureSampler) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create texture sampler!\n");
  }
  printf("craeted sampler successfully\n");
}

void loadModel() {
  ObjAttrib attrib;
  ObjShape shape;

  if (loadObj(&attrib, &shape, MODLE_PATH) != 0) {
    fprintf(stderr, "Failed to load model: %s\n", MODLE_PATH);
    return;
  }

  // temporary dynamic arrays for deduplication
  Vertex *uniqueVertices = NULL;
  size_t uniqueCount = 0;
  uint32_t *outIndices = NULL;
  size_t outIndexCount = 0;

  for (size_t m = 0; m < shape.meshCount; m++) {
    ObjMesh *mesh = &shape.meshes[m];

    for (size_t i = 0; i < mesh->indexCount; i++) {
      ObjIndex *idx = &mesh->indices[i];

      Vertex v = {0};

      // position
      if (idx->v >= 0) {
        v.pos[0] = attrib.vertices[idx->v * 3 + 0];
        v.pos[1] = attrib.vertices[idx->v * 3 + 1];
        v.pos[2] = attrib.vertices[idx->v * 3 + 2];
      }

      // texcoord (flip V for Vulkan)
      if (idx->vt >= 0) {
        v.texCoord[0] = attrib.texcoords[idx->vt * 2 + 0];
        v.texCoord[1] = 1.0f - attrib.texcoords[idx->vt * 2 + 1];
      }

      // color — white by default
      v.color[0] = 1.0f;
      v.color[1] = 1.0f;
      v.color[2] = 1.0f;

      // check if vertex already exists (simple linear search)
      int found = 0;
      for (size_t j = 0; j < uniqueCount; j++) {
        if (memcmp(&uniqueVertices[j], &v, sizeof(Vertex)) == 0) {
          outIndices =
              realloc(outIndices, (outIndexCount + 1) * sizeof(uint32_t));
          outIndices[outIndexCount++] = (uint32_t)j;
          found = 1;
          break;
        }
      }

      if (!found) {
        uniqueVertices =
            realloc(uniqueVertices, (uniqueCount + 1) * sizeof(Vertex));
        uniqueVertices[uniqueCount] = v;
        outIndices =
            realloc(outIndices, (outIndexCount + 1) * sizeof(uint32_t));
        outIndices[outIndexCount++] = (uint32_t)uniqueCount;
        uniqueCount++;
      }
    }
  }

  // store into globals (replace your hardcoded vertices/indices arrays)
  // adjust these global names to match yours
  free(vertices);
  free(indices);
  vertices = uniqueVertices;
  vertexCount = uniqueCount;
  indices = outIndices;
  indexCount = outIndexCount;

  printf("Loaded model: %zu vertices, %zu indices\n", uniqueCount,
         outIndexCount);

  freeObj(&attrib, &shape);
}

void createVertexBuffer() {

  VkDeviceSize bufferSize = vertexCount * sizeof(vertices[0]);

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(Ldevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices, bufferSize);
  vkUnmapMemory(Ldevice, stagingBufferMemory);
  printf("filled staging buffer successfully for vertex buffer\n");

  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &VertexBuffer, &vertexBufferMemory);

  printf("Created vertex buffer\n");

  copyBuffer(stagingBuffer, VertexBuffer, bufferSize);
  printf("Copied data from staging buffer to vertex buffer\n");

  vkDestroyBuffer(Ldevice, stagingBuffer, NULL);
  vkFreeMemory(Ldevice, stagingBufferMemory, NULL);
}

void createIndexBuffer() {
  VkDeviceSize bufferSize = indexCount * sizeof(indices[0]);

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(Ldevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices, bufferSize);
  vkUnmapMemory(Ldevice, stagingBufferMemory);
  printf("filled staging buffer successfully for index buffer\n");

  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

  printf("Created index buffer\n");

  copyBuffer(stagingBuffer, indexBuffer, bufferSize);
  printf("Copied data from staging buffer to index buffer\n");

  vkDestroyBuffer(Ldevice, stagingBuffer, NULL);
  vkFreeMemory(Ldevice, stagingBufferMemory, NULL);
}

void createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers = malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkBuffer));
  if (!uniformBuffers) {
    fprintf(stderr, "Failed to allocate uniformBuffers array!\n");
  }

  uniformBuffersMemory = malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkDeviceMemory));
  if (!uniformBuffers) {
    fprintf(stderr, "Failed to allocate uniformBuffersMemory array!\n");
  }

  uniformBuffersMapped = malloc(MAX_FRAME_IN_FLIGHT * sizeof(void *));
  if (!uniformBuffers) {
    fprintf(stderr, "Failed to allocate uniformBuffersMapped array!\n");
  }

  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 &uniformBuffers[i], &uniformBuffersMemory[i]);

    vkMapMemory(Ldevice, uniformBuffersMemory[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void createDescriptorPool() {
  VkDescriptorPoolSize poolSize[2] = {0};
  poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize[0].descriptorCount = (uint32_t)MAX_FRAME_IN_FLIGHT;
  poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize[1].descriptorCount = (uint32_t)MAX_FRAME_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.pNext = NULL;
  poolInfo.flags = 0;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes = poolSize;
  poolInfo.maxSets = (uint32_t)MAX_FRAME_IN_FLIGHT;

  if (vkCreateDescriptorPool(Ldevice, &poolInfo, NULL, &descriptorPool) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create descriptor pool\n");
  }
  printf("Created descriptor pool successfully\n");
}

void createDescriptorSet() {
  VkDescriptorSetLayout *layouts =
      malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkDescriptorSetLayout));

  if (!layouts) {
    fprintf(stderr, "Failed to allocate layout!\n");
  }

  for (uint32_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    layouts[i] = descriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext = NULL;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = (uint32_t)MAX_FRAME_IN_FLIGHT;
  allocInfo.pSetLayouts = layouts;

  descriptorSets = malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkDescriptorSet));

  if (!descriptorSets) {
    fprintf(stderr, "Failed to allocate descriptorSets\n");
  }

  if (vkAllocateDescriptorSets(Ldevice, &allocInfo, descriptorSets) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate descriptor sets!\n");
  }
  printf("Allocated descriptor sets successfully\n");
  free(layouts);

  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    VkWriteDescriptorSet descriptorWrite[2] = {0};
    descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].pNext = NULL;
    descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite[0].descriptorCount = 1;
    descriptorWrite[0].pBufferInfo = &bufferInfo;
    descriptorWrite[0].pImageInfo = NULL;
    descriptorWrite[0].pTexelBufferView = NULL;
    descriptorWrite[0].dstArrayElement = 0;
    descriptorWrite[0].dstBinding = 0;
    descriptorWrite[0].dstSet = descriptorSets[i];

    descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[1].pNext = NULL;
    descriptorWrite[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[1].descriptorCount = 1;
    descriptorWrite[1].pBufferInfo = NULL;
    descriptorWrite[1].pImageInfo = &imageInfo;
    descriptorWrite[1].pTexelBufferView = NULL;
    descriptorWrite[1].dstArrayElement = 0;
    descriptorWrite[1].dstBinding = 1;
    descriptorWrite[1].dstSet = descriptorSets[i];

    vkUpdateDescriptorSets(Ldevice,
                           sizeof(descriptorWrite) / sizeof(descriptorWrite[0]),
                           descriptorWrite, 0, NULL);
  }
}

void createCommandBuffer() {
  commandBuffers = malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkCommandBuffer));
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.pNext = NULL;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAME_IN_FLIGHT;

  if (vkAllocateCommandBuffers(Ldevice, &allocInfo, commandBuffers) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate command buffer\n");
  }
  printf("Allocated command buffers successfully\n");
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = NULL;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = NULL;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "Failed to begin recording command buffer!\n");
  }

  VkRenderPassBeginInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset.x = 0;
  renderPassInfo.renderArea.offset.y = 0;
  renderPassInfo.renderArea.extent = swapChainExtent;

  VkClearValue clearValues[2] = {{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                                 {.depthStencil = {1.0f, 0.0f}}};
  renderPassInfo.clearValueCount = 2;
  renderPassInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkBuffer VertexBuffers[] = {VertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, VertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSets[currentFrame],
                          0, NULL);

  vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    fprintf(stderr, "Failed to record command buffer!\n");
  }
}

void createSyncObjects() {
  imageAvailableSemaphores = malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkSemaphore));
  renderFinishedSemaphores = malloc(swapChainImageCount * sizeof(VkSemaphore));
  inFlightFence = malloc(MAX_FRAME_IN_FLIGHT * sizeof(VkFence));

  if (!imageAvailableSemaphores || !renderFinishedSemaphores ||
      !inFlightFence) {
    fprintf(stderr, "Failed to allocate sync objects arrays\n");
    return;
  }

  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.pNext = NULL;
  semaphoreInfo.flags = 0;

  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.pNext = NULL;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(Ldevice, &semaphoreInfo, NULL,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create imageAvailableSemaphore[%u]\n", i);
    }
    if (vkCreateFence(Ldevice, &fenceInfo, NULL, &inFlightFence[i]) !=
        VK_SUCCESS) {
      fprintf(stderr, "Failed to create fence\n");
    }
  }

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    if (vkCreateSemaphore(Ldevice, &semaphoreInfo, NULL,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create renderFinishedSemaphore[%u]\n", i);
    }
  }

  printf("Created sync objects successfully\n");
}

void initvulkan() {
  VkApplicationInfo appInfo = createInstance();

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &appInfo;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = NumberOfValidationLayer;
    createInfo.ppEnabledLayerNames = validationLayers;
  } else {
    createInfo.enabledLayerCount = 0;
  }

  // checking for possible error code VK_ERROR_EXTENSION_NOT_PRESENT (-7)
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

  VkExtensionProperties extensions[extensionCount];

  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
  //* prints all extensions name and version
  // iterating over array
  printf("Available extensions: \n\n");
  printf("Extension Name\t\t\t\tversion\n");

  for (uint32_t i = 0; i < extensionCount; ++i) {
    printf("%s---------->%d\n", extensions[i].extensionName,
           extensions[i].specVersion);
  }
  printf("\n");

  VkResult result = vkCreateInstance(&createInfo, NULL, &instance);

  if (result != VK_SUCCESS) {
    printf("Error Creating Instance\n");
    printf("%d\n", result);
    return;
  }

  if (enableValidationLayers) {
    printf("successfully created vulkan instance\n\n");
  }

  createSurface();

  pickPhysicalDevice();

  createLogicalDevice();

  createSwapChain();

  createImageViews();

  createRenderPass();

  createaDescriptorSetLayout();

  createGraphicsPipeline();

  createCommandPool();

  createDepthResources();

  createframebuffers();

  createTextureImage();

  createTextureImageView();

  createTextureSampler();

  loadModel();

  createVertexBuffer();

  createIndexBuffer();

  createUniformBuffers();

  createDescriptorPool();

  createDescriptorSet();

  createCommandBuffer();

  createSyncObjects();

  return;
}

void initwindow() {

  if (!glfwInit()) {
    printf("Problem in glfwInit\n");
    return;
  }

  if (enableValidationLayers) {
    printf("\nGLFW initialized successfully\n\n");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Window", NULL, NULL);

  if (!window) {
    printf("Failed to create Window");
    return;
  }

  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void updateUniformbuffers(uint32_t currentImage) {
  static struct timespec startTime;
  static int initialized = 0;

  if (!initialized) {
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    initialized = 1;
  }

  struct timespec currentTime;
  clock_gettime(CLOCK_MONOTONIC, &currentTime);
  float time = (currentTime.tv_sec - startTime.tv_sec) +
               (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;

  mat4 model;
  glm_mat4_identity(model);

  vec3 axis = {0.0f, 1.0f, 0.0f};
  UniformBufferObject ubo = {0};
  glm_rotate(model, time * glm_rad(90.0f), axis);
  glm_mat4_copy(model, ubo.model);

  vec3 eye = {3.0f, 3.0f, 3.0f};
  vec3 center = {0.0f, 1.0f, 0.0f};
  vec3 up = {0.0f, 1.0f, 0.0f};
  glm_lookat(eye, center, up, ubo.view);

  glm_perspective(glm_rad(45.0f),
                  swapChainExtent.width / (float)swapChainExtent.height, 0.1f,
                  10.0f, ubo.proj);
  ubo.proj[1][1] *= -1;

  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void drawFrame() {
  vkWaitForFences(Ldevice, 1, &inFlightFence[currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;

  VkResult acquireResult = vkAcquireNextImageKHR(
      Ldevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (acquireResult != VK_SUCCESS &&
             acquireResult != VK_SUBOPTIMAL_KHR) {
    fprintf(stderr, "Failed to acquire swapchain image: %d\n", acquireResult);
  }

  vkResetFences(Ldevice, 1, &inFlightFence[currentFrame]);

  updateUniformbuffers(currentFrame);
  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = NULL;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkResult submitResult =
      vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence[currentFrame]);
  if (submitResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to submit draw command buffer: %d\n", submitResult);
  }

  VkSwapchainKHR swapChains[] = {swapChain};
  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL;

  VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
      presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (presentResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to present: %d\n", presentResult);
  }

  // advance to next semaphore slot, wrapping around
  currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

void mainloop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
}

void cleanup() {
  vkDeviceWaitIdle(Ldevice);
  cleanupSwpaChain();

  vkDestroySampler(Ldevice, textureSampler, NULL);
  vkDestroyImageView(Ldevice, textureImageView, NULL);

  vkDestroyImage(Ldevice, textureImage, NULL);
  vkFreeMemory(Ldevice, textureImageMemory, NULL);

  for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    vkDestroyBuffer(Ldevice, uniformBuffers[i], NULL);
    vkFreeMemory(Ldevice, uniformBuffersMemory[i], NULL);
  }

  free(uniformBuffers);
  free(uniformBuffersMemory);
  free(uniformBuffersMapped);
  vkDestroyDescriptorPool(Ldevice, descriptorPool, NULL);
  free(descriptorSets);
  vkDestroyDescriptorSetLayout(Ldevice, descriptorSetLayout, NULL);

  vkDestroyBuffer(Ldevice, indexBuffer, NULL);
  vkFreeMemory(Ldevice, indexBufferMemory, NULL);

  vkDestroyBuffer(Ldevice, VertexBuffer, NULL);
  vkFreeMemory(Ldevice, vertexBufferMemory, NULL);

  for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
    vkDestroySemaphore(Ldevice, imageAvailableSemaphores[i], NULL);
    vkDestroyFence(Ldevice, inFlightFence[i], NULL);
  }
  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroySemaphore(Ldevice, renderFinishedSemaphores[i], NULL);
  }
  free(imageAvailableSemaphores);
  free(renderFinishedSemaphores);
  free(inFlightFence);
  free(commandBuffers);

  vkDestroyCommandPool(Ldevice, commandPool, NULL);

  vkDestroyPipeline(Ldevice, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(Ldevice, pipelineLayout, NULL);
  vkDestroyRenderPass(Ldevice, renderPass, NULL);

  free(dynamicStates);

  vkDestroyShaderModule(Ldevice, fragShaderModule, NULL);
  vkDestroyShaderModule(Ldevice, vertShaderModule, NULL);

  vkDestroyDevice(Ldevice, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);

  glfwDestroyWindow(window);
  glfwTerminate();
}

int main() {

  initwindow();

  initvulkan();

  mainloop();

  cleanup();

  return 0;
}
