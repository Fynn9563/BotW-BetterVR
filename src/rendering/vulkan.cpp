#include "vulkan.h"
#include "hooking/layer.h"
#include "instance.h"

RND_Vulkan::RND_Vulkan(VkInstance vkInstance, VkPhysicalDevice vkPhysDevice, VkDevice vkDevice): m_instance(vkInstance), m_physicalDevice(vkPhysDevice), m_device(vkDevice) {
    m_instanceDispatch = vkroots::tables::LookupInstanceDispatch(vkInstance);
    m_physicalDeviceDispatch = vkroots::tables::LookupPhysicalDeviceDispatch(vkPhysDevice);
    m_deviceDispatch = vkroots::tables::LookupDeviceDispatch(vkDevice);

    m_physicalDeviceDispatch->GetPhysicalDeviceMemoryProperties2KHR(vkPhysDevice, &m_memoryProperties);

    VkPhysicalDeviceProperties props{};
    m_instanceDispatch->GetPhysicalDeviceProperties(vkPhysDevice, &props);

    uint64_t localVramBytes = 0;
    for (uint32_t i = 0; i < m_memoryProperties.memoryProperties.memoryHeapCount; ++i) {
        if ((m_memoryProperties.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) {
            localVramBytes += m_memoryProperties.memoryProperties.memoryHeaps[i].size;
        }
    }

    Log::print<INFO>("GPU: {} (vendor={:#06x}, device={:#06x}, driver={})", props.deviceName, props.vendorID, props.deviceID, props.driverVersion);
    if (localVramBytes > 0) {
        Log::print<INFO>("GPU VRAM (device local): {:.2f} GiB", double(localVramBytes) / (1024.0 * 1024.0 * 1024.0));
    }
}

RND_Vulkan::~RND_Vulkan() {
}

uint32_t RND_Vulkan::FindMemoryType(uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requirementsMask) {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        const uint32_t memoryTypeBits = (1 << i);
        const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;
        const bool satisfiesFlags = (m_memoryProperties.memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask;

        if (isRequiredMemoryType && satisfiesFlags) {
            return i;
        }
    }
    checkAssert(false, "Failed to find suitable memory type");
    return 0;
}


VkResult VRLayer::VkDeviceOverrides::CreateSwapchainKHR(const vkroots::VkDeviceDispatch* pDispatch, VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
    return pDispatch->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}