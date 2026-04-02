#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkFramebuffer *swapChainFramebuffers;
VkCommandPool commandPool;
VkCommandBuffer commandBuffer;
VkSemaphore *imageAvailableSemaphores;
VkSemaphore *renderFinishedSemaphores;
VkFence inFlightFence;

VkDynamicState *dynamicStates;

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
      printf("Extension not found: %s\n",
             deviceExtensions[i]); // ✅ show which one
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
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);

  QueueFamilyIndices indices = findQueueFamilies(device);

  if (indices.hasGraphicsFamily && indices.hasPresentFamily &&
      checkDeviceExtensionSupport(device)) {
    // check for swapchain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    if (swapChainSupport.formatCount && swapChainSupport.presentModeCount) {
      free(swapChainSupport.presentModes);
      free(swapChainSupport.formats);
      return true;
    }
    free(swapChainSupport.presentModes);
    free(swapChainSupport.formats);
  }

  return false;
}

void pickPhysicalDevice() {

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
  VkPhysicalDeviceFeatures deviceFeatires = {0};

  // create the logical device
  VkDeviceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = NULL;
  createInfo.flags = 0;
  createInfo.pQueueCreateInfos = queueCreateInfos;
  createInfo.queueCreateInfoCount = uniqueCount;
  createInfo.pEnabledFeatures = &deviceFeatires;

  // Validation layer for device
  createInfo.enabledExtensionCount = NumberOfDeviceExtension;
  createInfo.ppEnabledExtensionNames = deviceExtensions;

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = NumberOfValidationLayer;
    createInfo.ppEnabledLayerNames = validationLayers;
  } else {
    createInfo.enabledLayerCount = 0;
  }

  printf("calling vkCreateDevice...\n");
  VkResult result = vkCreateDevice(dev, &createInfo, NULL, &Ldevice);
  printf("vkCreateDevice returned: %d\n", result);

  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!");
  }

  // Using vkGetDeviceQueue function to retrive queue handles for each queue
  // family

  printf("getting device queue...\n");
  vkGetDeviceQueue(Ldevice, indices.graphicsFamily, 0, &graphicsQueue);
  vkGetDeviceQueue(Ldevice, indices.presentFamily, 0, &presentQueue);
  printf("createLogicalDevice done\n");
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats,
                                           uint32_t formatCount) {
  for (uint32_t i = 0; i < formatCount; ++i) {
    if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormats[i].colorSpace ==
            VK_COLORSPACE_SRGB_NONLINEAR_KHR) { // VK_COLORSPACE_SRGB_NONLINEAR_KHR
                                                // for older version fo the spec
                                                // else use
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

void createImageView() {
  swapChainImageViews = malloc(swapChainImageCount * sizeof(VkImageView));

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    VkImageViewCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(Ldevice, &createInfo, NULL,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image view\n");
      return;
    }
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

  VkSubpassDescription subpass = {0};
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = NULL;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  VkSubpassDependency dependencies[2] = {0};
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = 0;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].dstAccessMask = 0;
  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.flags = 0;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = dependencies;

  if (vkCreateRenderPass(Ldevice, &renderPassInfo, NULL, &renderPass) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create render pass!\n");
  }
  printf("Created render pass successfully\n");
}

void createGraphicsPipeline() {
  FileBuffer vertShaderCode = readFile("shaders/vert.spv");
  FileBuffer fragShaderCode = readFile("shaders/frag.spv");

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

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pNext = NULL;
  vertexInputInfo.flags = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;

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
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = NULL;
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
  pipelineInfo.pDepthStencilState = NULL;
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
}

void createframebuffers() {
  swapChainFramebuffers = malloc(swapChainImageCount * sizeof(VkFramebuffer));

  for (uint32_t i = 0; i < swapChainImageCount; ++i) {

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = NULL;
    framebufferInfo.flags = 0;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &swapChainImageViews[i];
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

void createCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.pNext = NULL;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(Ldevice, &allocInfo, &commandBuffer) !=
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
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

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

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    fprintf(stderr, "Failed to record command buffer!\n");
  }
}

void createSyncObjects() {
  imageAvailableSemaphores = malloc(swapChainImageCount * sizeof(VkSemaphore));
  renderFinishedSemaphores = malloc(swapChainImageCount * sizeof(VkSemaphore));

  if (!imageAvailableSemaphores || !renderFinishedSemaphores) {
    fprintf(stderr, "Failed to allocate semaphore arrays\n");
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

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    if (vkCreateSemaphore(Ldevice, &semaphoreInfo, NULL,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create imageAvailableSemaphore[%u]\n", i);
    }
    if (vkCreateSemaphore(Ldevice, &semaphoreInfo, NULL,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create renderFinishedSemaphore[%u]\n", i);
    }
  }

  if (vkCreateFence(Ldevice, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create fence\n");
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

  createImageView();

  createRenderPass();

  createGraphicsPipeline();

  createframebuffers();

  createCommandPool();

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
}

void drawFrame() {
  vkWaitForFences(Ldevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(Ldevice, 1, &inFlightFence);

  uint32_t imageIndex;
  static uint32_t semaphoreIndex = 0;

  VkResult acquireResult = vkAcquireNextImageKHR(
      Ldevice, swapChain, UINT64_MAX, imageAvailableSemaphores[semaphoreIndex],
      VK_NULL_HANDLE, &imageIndex);
  if (acquireResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to acquire swapchain image: %d\n", acquireResult);
  }

  vkResetCommandBuffer(commandBuffer, 0);
  recordCommandBuffer(commandBuffer, imageIndex);

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[semaphoreIndex]};
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
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkResult submitResult =
      vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
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
  if (presentResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to present: %d\n", presentResult);
  }

  // advance to next semaphore slot, wrapping around
  semaphoreIndex = (semaphoreIndex + 1) % swapChainImageCount;
}

void mainloop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
}

void cleanup() {
  vkDeviceWaitIdle(Ldevice);

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroySemaphore(Ldevice, imageAvailableSemaphores[i], NULL);
    vkDestroySemaphore(Ldevice, renderFinishedSemaphores[i], NULL);
  }
  free(imageAvailableSemaphores);
  free(renderFinishedSemaphores);

  vkDestroyFence(Ldevice, inFlightFence, NULL);

  vkDestroyCommandPool(Ldevice, commandPool, NULL);

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroyFramebuffer(Ldevice, swapChainFramebuffers[i], NULL);
  }
  free(swapChainFramebuffers);

  vkDestroyPipeline(Ldevice, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(Ldevice, pipelineLayout, NULL);
  vkDestroyRenderPass(Ldevice, renderPass, NULL);

  free(dynamicStates);

  vkDestroyShaderModule(Ldevice, fragShaderModule, NULL);
  vkDestroyShaderModule(Ldevice, vertShaderModule, NULL);

  for (uint32_t i = 0; i < swapChainImageCount; i++) {
    vkDestroyImageView(Ldevice, swapChainImageViews[i], NULL);
  }
  free(swapChainImageViews);
  free(swapChainImages);

  vkDestroySwapchainKHR(Ldevice, swapChain, NULL);
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
