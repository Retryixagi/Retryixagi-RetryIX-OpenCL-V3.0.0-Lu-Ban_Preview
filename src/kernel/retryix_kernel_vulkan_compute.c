// RetryIX 3.0.0 "魯班" - Vulkan Compute 執行引擎
// 全新實做: 直接用 Vulkan compute shader 執行 kernel,不依賴外部 OpenCL
// Version: 3.0.0 Codename: 魯班 (Lu Ban)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// === Vulkan 動態函數加載 ===
static HMODULE g_vulkan_lib = NULL;

// Instance functions  
static PFN_vkCreateInstance vkCreateInstance_dyn = NULL;
static PFN_vkDestroyInstance vkDestroyInstance_dyn = NULL;
static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices_dyn = NULL;
static PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties_dyn = NULL;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties_dyn = NULL;
static PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties_dyn = NULL;
static PFN_vkCreateDevice vkCreateDevice_dyn = NULL;
static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr_dyn = NULL;

// Device functions (will be loaded per-device)
static PFN_vkDestroyDevice vkDestroyDevice_dyn = NULL;
static PFN_vkGetDeviceQueue vkGetDeviceQueue_dyn = NULL;
static PFN_vkCreateCommandPool vkCreateCommandPool_dyn = NULL;
static PFN_vkDestroyCommandPool vkDestroyCommandPool_dyn = NULL;
static PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers_dyn = NULL;
static PFN_vkBeginCommandBuffer vkBeginCommandBuffer_dyn = NULL;
static PFN_vkEndCommandBuffer vkEndCommandBuffer_dyn = NULL;
static PFN_vkQueueSubmit vkQueueSubmit_dyn = NULL;
static PFN_vkQueueWaitIdle vkQueueWaitIdle_dyn = NULL;
static PFN_vkCreateBuffer vkCreateBuffer_dyn = NULL;
static PFN_vkDestroyBuffer vkDestroyBuffer_dyn = NULL;
static PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements_dyn = NULL;
static PFN_vkAllocateMemory vkAllocateMemory_dyn = NULL;
static PFN_vkFreeMemory vkFreeMemory_dyn = NULL;
static PFN_vkBindBufferMemory vkBindBufferMemory_dyn = NULL;
static PFN_vkMapMemory vkMapMemory_dyn = NULL;
static PFN_vkUnmapMemory vkUnmapMemory_dyn = NULL;
static PFN_vkCmdCopyBuffer vkCmdCopyBuffer_dyn = NULL;
static PFN_vkCmdBindPipeline vkCmdBindPipeline_dyn = NULL;
static PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets_dyn = NULL;
static PFN_vkCmdDispatch vkCmdDispatch_dyn = NULL;
static PFN_vkCmdPushConstants vkCmdPushConstants_dyn = NULL;
static PFN_vkCreateShaderModule vkCreateShaderModule_dyn = NULL;
static PFN_vkDestroyShaderModule vkDestroyShaderModule_dyn = NULL;
static PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout_dyn = NULL;
static PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout_dyn = NULL;
static PFN_vkCreatePipelineLayout vkCreatePipelineLayout_dyn = NULL;
static PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout_dyn = NULL;
static PFN_vkCreateComputePipelines vkCreateComputePipelines_dyn = NULL;
static PFN_vkDestroyPipeline vkDestroyPipeline_dyn = NULL;
static PFN_vkCreateDescriptorPool vkCreateDescriptorPool_dyn = NULL;
static PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool_dyn = NULL;
static PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets_dyn = NULL;
static PFN_vkFreeDescriptorSets vkFreeDescriptorSets_dyn = NULL;
static PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets_dyn = NULL;

// === Vulkan GPU 上下文 ===
typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue compute_queue;
    VkCommandPool command_pool;
    uint32_t compute_queue_family;
    VkPhysicalDeviceMemoryProperties mem_properties;
    int initialized;
    
    // 緩存的 pipeline 資源 (避免每次重建)
    VkShaderModule cached_shader;
    VkDescriptorSetLayout cached_dsl;
    VkPipelineLayout cached_pl;
    VkPipeline cached_pipeline;
    VkDescriptorPool cached_dpool;
    int pipeline_ready;
} vulkan_compute_context_t;

static vulkan_compute_context_t g_vk_ctx = {0};

// === 初始化 Vulkan compute 上下文 ===
int retryix_vulkan_compute_init() {
    if (g_vk_ctx.initialized) {
        return 1; // Already initialized
    }
    
    printf("[Vulkan Compute] Initializing RetryIX Vulkan compute engine...\n");
    
#ifdef _WIN32
    g_vulkan_lib = LoadLibraryA("vulkan-1.dll");
    if (!g_vulkan_lib) {
        printf("[Vulkan Compute] ERROR: Failed to load vulkan-1.dll\n");
        return 0;
    }
    
    // Load instance functions
    vkCreateInstance_dyn = (PFN_vkCreateInstance)GetProcAddress(g_vulkan_lib, "vkCreateInstance");
    vkDestroyInstance_dyn = (PFN_vkDestroyInstance)GetProcAddress(g_vulkan_lib, "vkDestroyInstance");
    vkEnumeratePhysicalDevices_dyn = (PFN_vkEnumeratePhysicalDevices)GetProcAddress(g_vulkan_lib, "vkEnumeratePhysicalDevices");
    vkGetPhysicalDeviceProperties_dyn = (PFN_vkGetPhysicalDeviceProperties)GetProcAddress(g_vulkan_lib, "vkGetPhysicalDeviceProperties");
    vkGetPhysicalDeviceQueueFamilyProperties_dyn = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)GetProcAddress(g_vulkan_lib, "vkGetPhysicalDeviceQueueFamilyProperties");
    vkGetPhysicalDeviceMemoryProperties_dyn = (PFN_vkGetPhysicalDeviceMemoryProperties)GetProcAddress(g_vulkan_lib, "vkGetPhysicalDeviceMemoryProperties");
    vkCreateDevice_dyn = (PFN_vkCreateDevice)GetProcAddress(g_vulkan_lib, "vkCreateDevice");
    vkGetDeviceProcAddr_dyn = (PFN_vkGetDeviceProcAddr)GetProcAddress(g_vulkan_lib, "vkGetDeviceProcAddr");
    
    if (!vkCreateInstance_dyn || !vkEnumeratePhysicalDevices_dyn || !vkCreateDevice_dyn) {
        printf("[Vulkan Compute] ERROR: Failed to load required Vulkan functions\n");
        FreeLibrary(g_vulkan_lib);
        return 0;
    }
#endif
    
    // Create Vulkan instance
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "RetryIX Compute";
    app_info.applicationVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.pEngineName = "LuBan";
    app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    VkResult result = vkCreateInstance_dyn(&create_info, NULL, &g_vk_ctx.instance);
    if (result != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: Failed to create Vulkan instance (error %d)\n", result);
        return 0;
    }
    
    printf("[Vulkan Compute] Vulkan instance created\n");
    
    // Enumerate physical devices
    uint32_t device_count = 0;
    result = vkEnumeratePhysicalDevices_dyn(g_vk_ctx.instance, &device_count, NULL);
    if (result != VK_SUCCESS || device_count == 0) {
        printf("[Vulkan Compute] ERROR: No Vulkan devices found\n");
        vkDestroyInstance_dyn(g_vk_ctx.instance, NULL);
        return 0;
    }
    
    VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * device_count);
    result = vkEnumeratePhysicalDevices_dyn(g_vk_ctx.instance, &device_count, physical_devices);
    
    // Select first discrete GPU (or any GPU)
    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties_dyn(physical_devices[i], &props);
        
        printf("[Vulkan Compute] Device %d: %s\n", i, props.deviceName);
        
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || 
            props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            g_vk_ctx.physical_device = physical_devices[i];
            printf("[Vulkan Compute] Selected GPU: %s\n", props.deviceName);
            break;
        }
    }
    
    free(physical_devices);
    
    if (g_vk_ctx.physical_device == VK_NULL_HANDLE) {
        printf("[Vulkan Compute] ERROR: No suitable GPU found\n");
        vkDestroyInstance_dyn(g_vk_ctx.instance, NULL);
        return 0;
    }
    
    // Get memory properties
    vkGetPhysicalDeviceMemoryProperties_dyn(g_vk_ctx.physical_device, &g_vk_ctx.mem_properties);
    
    // Find compute queue family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties_dyn(g_vk_ctx.physical_device, &queue_family_count, NULL);
    
    VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties_dyn(g_vk_ctx.physical_device, &queue_family_count, queue_families);
    
    g_vk_ctx.compute_queue_family = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            g_vk_ctx.compute_queue_family = i;
            printf("[Vulkan Compute] Compute queue family: %d\n", i);
            break;
        }
    }
    
    free(queue_families);
    
    if (g_vk_ctx.compute_queue_family == UINT32_MAX) {
        printf("[Vulkan Compute] ERROR: No compute queue family found\n");
        vkDestroyInstance_dyn(g_vk_ctx.instance, NULL);
        return 0;
    }
    
    // Create logical device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = g_vk_ctx.compute_queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    
    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    
    result = vkCreateDevice_dyn(g_vk_ctx.physical_device, &device_create_info, NULL, &g_vk_ctx.device);
    if (result != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: Failed to create logical device (error %d)\n", result);
        vkDestroyInstance_dyn(g_vk_ctx.instance, NULL);
        return 0;
    }
    
    printf("[Vulkan Compute] Logical device created\n");
    
    // Load device-specific functions
    vkDestroyDevice_dyn = (PFN_vkDestroyDevice)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyDevice");
    vkGetDeviceQueue_dyn = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkGetDeviceQueue");
    vkCreateCommandPool_dyn = (PFN_vkCreateCommandPool)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreateCommandPool");
    vkDestroyCommandPool_dyn = (PFN_vkDestroyCommandPool)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyCommandPool");
    vkAllocateCommandBuffers_dyn = (PFN_vkAllocateCommandBuffers)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkAllocateCommandBuffers");
    vkBeginCommandBuffer_dyn = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkBeginCommandBuffer");
    vkEndCommandBuffer_dyn = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkEndCommandBuffer");
    vkQueueSubmit_dyn = (PFN_vkQueueSubmit)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkQueueSubmit");
    vkQueueWaitIdle_dyn = (PFN_vkQueueWaitIdle)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkQueueWaitIdle");
    vkCreateBuffer_dyn = (PFN_vkCreateBuffer)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreateBuffer");
    vkDestroyBuffer_dyn = (PFN_vkDestroyBuffer)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyBuffer");
    vkGetBufferMemoryRequirements_dyn = (PFN_vkGetBufferMemoryRequirements)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkGetBufferMemoryRequirements");
    vkAllocateMemory_dyn = (PFN_vkAllocateMemory)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkAllocateMemory");
    vkFreeMemory_dyn = (PFN_vkFreeMemory)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkFreeMemory");
    vkBindBufferMemory_dyn = (PFN_vkBindBufferMemory)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkBindBufferMemory");
    vkMapMemory_dyn = (PFN_vkMapMemory)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkMapMemory");
    vkUnmapMemory_dyn = (PFN_vkUnmapMemory)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkUnmapMemory");
    vkCmdCopyBuffer_dyn = (PFN_vkCmdCopyBuffer)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCmdCopyBuffer");
    vkCmdBindPipeline_dyn = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCmdBindPipeline");
    vkCmdBindDescriptorSets_dyn = (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCmdBindDescriptorSets");
    vkCmdDispatch_dyn = (PFN_vkCmdDispatch)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCmdDispatch");
    vkCmdPushConstants_dyn = (PFN_vkCmdPushConstants)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCmdPushConstants");
    vkCreateShaderModule_dyn = (PFN_vkCreateShaderModule)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreateShaderModule");
    vkDestroyShaderModule_dyn = (PFN_vkDestroyShaderModule)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyShaderModule");
    vkCreateDescriptorSetLayout_dyn = (PFN_vkCreateDescriptorSetLayout)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreateDescriptorSetLayout");
    vkDestroyDescriptorSetLayout_dyn = (PFN_vkDestroyDescriptorSetLayout)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyDescriptorSetLayout");
    vkCreatePipelineLayout_dyn = (PFN_vkCreatePipelineLayout)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreatePipelineLayout");
    vkDestroyPipelineLayout_dyn = (PFN_vkDestroyPipelineLayout)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyPipelineLayout");
    vkCreateComputePipelines_dyn = (PFN_vkCreateComputePipelines)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreateComputePipelines");
    vkDestroyPipeline_dyn = (PFN_vkDestroyPipeline)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyPipeline");
    vkCreateDescriptorPool_dyn = (PFN_vkCreateDescriptorPool)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkCreateDescriptorPool");
    vkDestroyDescriptorPool_dyn = (PFN_vkDestroyDescriptorPool)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkDestroyDescriptorPool");
    vkAllocateDescriptorSets_dyn = (PFN_vkAllocateDescriptorSets)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkAllocateDescriptorSets");
    vkFreeDescriptorSets_dyn = (PFN_vkFreeDescriptorSets)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkFreeDescriptorSets");
    vkUpdateDescriptorSets_dyn = (PFN_vkUpdateDescriptorSets)vkGetDeviceProcAddr_dyn(g_vk_ctx.device, "vkUpdateDescriptorSets");
    
    // Get compute queue
    vkGetDeviceQueue_dyn(g_vk_ctx.device, g_vk_ctx.compute_queue_family, 0, &g_vk_ctx.compute_queue);
    
    // Create command pool
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = g_vk_ctx.compute_queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    result = vkCreateCommandPool_dyn(g_vk_ctx.device, &pool_info, NULL, &g_vk_ctx.command_pool);
    if (result != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: Failed to create command pool (error %d)\n", result);
        vkDestroyDevice_dyn(g_vk_ctx.device, NULL);
        vkDestroyInstance_dyn(g_vk_ctx.instance, NULL);
        return 0;
    }
    
    g_vk_ctx.initialized = 1;
    printf("[Vulkan Compute] ✓ Initialization complete - Ready for GPU compute!\n");
    
    return 1;
}

// === 清理 Vulkan compute 上下文 ===
void retryix_vulkan_compute_cleanup() {
    if (!g_vk_ctx.initialized) return;
    
    if (g_vk_ctx.command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool_dyn(g_vk_ctx.device, g_vk_ctx.command_pool, NULL);
    }
    if (g_vk_ctx.device != VK_NULL_HANDLE) {
        vkDestroyDevice_dyn(g_vk_ctx.device, NULL);
    }
    if (g_vk_ctx.instance != VK_NULL_HANDLE) {
        vkDestroyInstance_dyn(g_vk_ctx.instance, NULL);
    }
    
#ifdef _WIN32
    if (g_vulkan_lib) {
        FreeLibrary(g_vulkan_lib);
    }
#endif
    
    g_vk_ctx.initialized = 0;
    printf("[Vulkan Compute] Cleanup complete\n");
}

// === 輔助函數: 尋找合適的記憶體類型 ===
static uint32_t find_memory_type(const VkPhysicalDeviceMemoryProperties* memProps, 
                                  uint32_t typeBits, VkMemoryPropertyFlags props) {
    for (uint32_t i = 0; i < memProps->memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) && 
            (memProps->memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return UINT32_MAX;
}

// === 讀取 SPIR-V shader 文件 ===
static char* read_spv_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, sz, f) != (size_t)sz) { 
        free(buf); 
        fclose(f); 
        return NULL; 
    }
    fclose(f);
    *out_size = sz;
    return buf;
}

// === 初始化 Pipeline (只在第一次調用時執行) ===
static int ensure_pipeline_ready() {
    if (g_vk_ctx.pipeline_ready) {
        return 1; // Already ready
    }
    
    VkResult r;
    VkDevice dev = g_vk_ctx.device;
    
    printf("[Vulkan Compute] Creating compute pipeline (one-time setup)...\n");
    
    // === 1. 讀取 SPIR-V shader ===
    const char* spv_paths[] = {
        "shaders/vector_add.spv",
        "../shaders/vector_add.spv",
        "../../shaders/vector_add.spv",
        "vector_add.spv"
    };
    
    size_t spv_size = 0;
    char* spv_code = NULL;
    for (int i = 0; i < 4; i++) {
        spv_code = read_spv_file(spv_paths[i], &spv_size);
        if (spv_code) {
            printf("[Vulkan Compute] Loaded SPIR-V: %s (%zu bytes)\n", spv_paths[i], spv_size);
            break;
        }
    }
    
    if (!spv_code) {
        printf("[Vulkan Compute] ERROR: SPIR-V shader not found\n");
        return 0;
    }
    
    // === 2. 創建 Shader Module ===
    VkShaderModuleCreateInfo smci = {0};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize = spv_size;
    smci.pCode = (const uint32_t*)spv_code;
    
    r = vkCreateShaderModule_dyn(dev, &smci, NULL, &g_vk_ctx.cached_shader);
    free(spv_code);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkCreateShaderModule failed %d\n", r);
        return 0;
    }
    
    // === 3. 創建 Descriptor Set Layout (3個 storage buffers) ===
    VkDescriptorSetLayoutBinding bindings[3];
    for (int i = 0; i < 3; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = NULL;
    }
    
    VkDescriptorSetLayoutCreateInfo dslci = {0};
    dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslci.bindingCount = 3;
    dslci.pBindings = bindings;
    
    r = vkCreateDescriptorSetLayout_dyn(dev, &dslci, NULL, &g_vk_ctx.cached_dsl);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkCreateDescriptorSetLayout failed %d\n", r);
        vkDestroyShaderModule_dyn(dev, g_vk_ctx.cached_shader, NULL);
        return 0;
    }
    
    // === 4. 創建 Pipeline Layout (含 push constants) ===
    VkPushConstantRange pcr = {0};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset = 0;
    pcr.size = 4;  // uint32_t n
    
    VkPipelineLayoutCreateInfo plci = {0};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &g_vk_ctx.cached_dsl;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;
    
    r = vkCreatePipelineLayout_dyn(dev, &plci, NULL, &g_vk_ctx.cached_pl);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkCreatePipelineLayout failed %d\n", r);
        vkDestroyDescriptorSetLayout_dyn(dev, g_vk_ctx.cached_dsl, NULL);
        vkDestroyShaderModule_dyn(dev, g_vk_ctx.cached_shader, NULL);
        return 0;
    }
    
    // === 5. 創建 Compute Pipeline ===
    VkPipelineShaderStageCreateInfo pssci = {0};
    pssci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pssci.module = g_vk_ctx.cached_shader;
    pssci.pName = "main";
    
    VkComputePipelineCreateInfo cpci = {0};
    cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpci.stage = pssci;
    cpci.layout = g_vk_ctx.cached_pl;
    
    r = vkCreateComputePipelines_dyn(dev, VK_NULL_HANDLE, 1, &cpci, NULL, &g_vk_ctx.cached_pipeline);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkCreateComputePipelines failed %d\n", r);
        vkDestroyPipelineLayout_dyn(dev, g_vk_ctx.cached_pl, NULL);
        vkDestroyDescriptorSetLayout_dyn(dev, g_vk_ctx.cached_dsl, NULL);
        vkDestroyShaderModule_dyn(dev, g_vk_ctx.cached_shader, NULL);
        return 0;
    }
    
    // === 6. 創建 Descriptor Pool (支持多次分配) ===
    VkDescriptorPoolSize dps = {0};
    dps.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dps.descriptorCount = 300;  // 支持 100 次並發執行
    
    VkDescriptorPoolCreateInfo dpci = {0};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets = 100;
    dpci.poolSizeCount = 1;
    dpci.pPoolSizes = &dps;
    dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;  // 允許單獨釋放
    
    r = vkCreateDescriptorPool_dyn(dev, &dpci, NULL, &g_vk_ctx.cached_dpool);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkCreateDescriptorPool failed %d\n", r);
        vkDestroyPipeline_dyn(dev, g_vk_ctx.cached_pipeline, NULL);
        vkDestroyPipelineLayout_dyn(dev, g_vk_ctx.cached_pl, NULL);
        vkDestroyDescriptorSetLayout_dyn(dev, g_vk_ctx.cached_dsl, NULL);
        vkDestroyShaderModule_dyn(dev, g_vk_ctx.cached_shader, NULL);
        return 0;
    }
    
    g_vk_ctx.pipeline_ready = 1;
    printf("[Vulkan Compute] ✓ Pipeline ready for reuse!\n");
    return 1;
}

// === Vulkan GPU 向量加法 - 優化版 (重用 pipeline) ===
int retryix_vulkan_vector_add(float* a, float* b, float* c, int n) {
    if (!g_vk_ctx.initialized) {
        if (!retryix_vulkan_compute_init()) {
            return 0;
        }
    }
    
    // 確保 pipeline 已準備好 (只在第一次執行)
    if (!g_vk_ctx.pipeline_ready) {
        if (!ensure_pipeline_ready()) {
            return 0;
        }
    }
    
    VkResult r;
    VkDevice dev = g_vk_ctx.device;
    
    // === 1. 分配 Descriptor Set (從緩存的 pool) ===
    VkDescriptorSetAllocateInfo dsai = {0};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = g_vk_ctx.cached_dpool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &g_vk_ctx.cached_dsl;
    
    VkDescriptorSet dset;
    r = vkAllocateDescriptorSets_dyn(dev, &dsai, &dset);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkAllocateDescriptorSets failed %d\n", r);
        return 0;
    }
    
    // === 2. 創建 3 個 GPU buffers ===
    size_t buf_size = n * sizeof(float);
    VkBuffer buffers[3];
    VkDeviceMemory mems[3];
    
    for (int i = 0; i < 3; i++) {
        VkBufferCreateInfo bci = {0};
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size = buf_size;
        bci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        r = vkCreateBuffer_dyn(dev, &bci, NULL, &buffers[i]);
        if (r != VK_SUCCESS) {
            printf("[Vulkan Compute] ERROR: vkCreateBuffer[%d] failed %d\n", i, r);
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer_dyn(dev, buffers[j], NULL);
                vkFreeMemory_dyn(dev, mems[j], NULL);
            }
            return 0;
        }
        
        VkMemoryRequirements mr;
        vkGetBufferMemoryRequirements_dyn(dev, buffers[i], &mr);
        
        uint32_t memIndex = find_memory_type(&g_vk_ctx.mem_properties, 
                                            (uint32_t)mr.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (memIndex == UINT32_MAX) {
            printf("[Vulkan Compute] ERROR: No host visible memory\n");
            for (int j = 0; j <= i; j++) {
                vkDestroyBuffer_dyn(dev, buffers[j], NULL);
            }
            return 0;
        }
        
        VkMemoryAllocateInfo mai = {0};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = memIndex;
        
        r = vkAllocateMemory_dyn(dev, &mai, NULL, &mems[i]);
        if (r != VK_SUCCESS) {
            printf("[Vulkan Compute] ERROR: vkAllocateMemory[%d] failed %d\n", i, r);
            vkDestroyBuffer_dyn(dev, buffers[i], NULL);
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer_dyn(dev, buffers[j], NULL);
                vkFreeMemory_dyn(dev, mems[j], NULL);
            }
            return 0;
        }
        
        r = vkBindBufferMemory_dyn(dev, buffers[i], mems[i], 0);
        if (r != VK_SUCCESS) {
            printf("[Vulkan Compute] ERROR: vkBindBufferMemory[%d] failed %d\n", i, r);
            vkFreeMemory_dyn(dev, mems[i], NULL);
            vkDestroyBuffer_dyn(dev, buffers[i], NULL);
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer_dyn(dev, buffers[j], NULL);
                vkFreeMemory_dyn(dev, mems[j], NULL);
            }
            return 0;
        }
    }
    
    printf("[Vulkan Compute] ✓ GPU buffers created (%zu bytes each)\n", buf_size);
    
    // === 9. 上傳數據到 GPU ===
    void* mapped;
    vkMapMemory_dyn(dev, mems[0], 0, buf_size, 0, &mapped);
    memcpy(mapped, a, buf_size);
    vkUnmapMemory_dyn(dev, mems[0]);
    
    vkMapMemory_dyn(dev, mems[1], 0, buf_size, 0, &mapped);
    memcpy(mapped, b, buf_size);
    vkUnmapMemory_dyn(dev, mems[1]);
    
    printf("[Vulkan Compute] ✓ Data uploaded to GPU\n");
    
    // === 10. 更新 Descriptor Sets ===
    VkDescriptorBufferInfo dbi[3];
    VkWriteDescriptorSet wds[3];
    
    for (int i = 0; i < 3; i++) {
        dbi[i].buffer = buffers[i];
        dbi[i].offset = 0;
        dbi[i].range = buf_size;
        
        wds[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[i].pNext = NULL;
        wds[i].dstSet = dset;
        wds[i].dstBinding = i;
        wds[i].dstArrayElement = 0;
        wds[i].descriptorCount = 1;
        wds[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wds[i].pImageInfo = NULL;
        wds[i].pBufferInfo = &dbi[i];
        wds[i].pTexelBufferView = NULL;
    }
    
    vkUpdateDescriptorSets_dyn(dev, 3, wds, 0, NULL);
    
    // === 11. 分配 Command Buffer ===
    VkCommandBufferAllocateInfo cbai = {0};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = g_vk_ctx.command_pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    
    VkCommandBuffer cmd;
    r = vkAllocateCommandBuffers_dyn(dev, &cbai, &cmd);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkAllocateCommandBuffers failed %d\n", r);
        goto cleanup_buffers;
    }
    
    // === 12. 記錄 Command Buffer ===
    VkCommandBufferBeginInfo cbbi = {0};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    r = vkBeginCommandBuffer_dyn(cmd, &cbbi);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkBeginCommandBuffer failed %d\n", r);
        goto cleanup_buffers;
    }
    
    vkCmdBindPipeline_dyn(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_vk_ctx.cached_pipeline);
    vkCmdBindDescriptorSets_dyn(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_vk_ctx.cached_pl, 0, 1, &dset, 0, NULL);
    
    // Push constant: n
    uint32_t n_uint = (uint32_t)n;
    vkCmdPushConstants_dyn(cmd, g_vk_ctx.cached_pl, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &n_uint);
    
    // Dispatch: (n + 255) / 256 workgroups, each with 256 threads
    uint32_t groupCount = (n + 255) / 256;
    vkCmdDispatch_dyn(cmd, groupCount, 1, 1);
    
    r = vkEndCommandBuffer_dyn(cmd);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkEndCommandBuffer failed %d\n", r);
        goto cleanup_buffers;
    }
    
    // === 3. 提交並執行 ===
    VkSubmitInfo si = {0};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    
    r = vkQueueSubmit_dyn(g_vk_ctx.compute_queue, 1, &si, VK_NULL_HANDLE);
    if (r != VK_SUCCESS) {
        printf("[Vulkan Compute] ERROR: vkQueueSubmit failed %d\n", r);
        goto cleanup_buffers;
    }
    
    vkQueueWaitIdle_dyn(g_vk_ctx.compute_queue);
    
    // === 4. 下載結果 ===
    vkMapMemory_dyn(dev, mems[2], 0, buf_size, 0, &mapped);
    memcpy(c, mapped, buf_size);
    vkUnmapMemory_dyn(dev, mems[2]);
    
    // === 5. 清理 (buffers 和 descriptor set) ===
cleanup_buffers:
    for (int i = 0; i < 3; i++) {
        if (buffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer_dyn(dev, buffers[i], NULL);
        }
        if (mems[i] != VK_NULL_HANDLE) {
            vkFreeMemory_dyn(dev, mems[i], NULL);
        }
    }
    
    // 釋放 descriptor set 回 pool (供下次重用)
    if (dset != VK_NULL_HANDLE) {
        vkFreeDescriptorSets_dyn = (PFN_vkFreeDescriptorSets)vkGetDeviceProcAddr_dyn(dev, "vkFreeDescriptorSets");
        if (vkFreeDescriptorSets_dyn) {
            vkFreeDescriptorSets_dyn(dev, g_vk_ctx.cached_dpool, 1, &dset);
        }
    }
    
    return (r == VK_SUCCESS) ? 1 : 0;
}
