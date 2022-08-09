#include "layer.h"

// #define ENABLE_FUNC_LOGGING

std::vector<VkInstance> steamInstances;
std::vector<VkDevice> steamDevices;
HMODULE vulkanModule = NULL;

// Original functions
PFN_vkCreateInstance top_origCreateInstance = nullptr;
PFN_vkCreateDevice top_origCreateDevice = nullptr;

// External variables
extern PFN_vkGetInstanceProcAddr saved_GetInstanceProcAddr;
extern PFN_vkGetDeviceProcAddr saved_GetDeviceProcAddr;

// Resolve macros
#define FUNC_LOGGING_LEVEL 2

#if FUNC_LOGGING_LEVEL == 2
#define LOG_FUNC_RESOLVE(resolveFuncType, resolveFunc, object, name) \
logPrint(std::format("[GetProcAddr] Using {}: vkGet*ProcAddr(object={}, name={}) returned {}", resolveFuncType, (void*)object, name, (void*)resolveFunc(object, name));\
resolveFunc(object, name);
#define logDebugPrintAddr(func) logPrint(func);
#else
#define LOG_FUNC_RESOLVE(func, funcName) resolveFunc(object, name);
#define logDebugPrintAddr(func) {}
#endif

void SteamVRHook_initialize() {
	SetEnvironmentVariableA("VK_INSTANCE_LAYERS", NULL);

	vulkanModule = LoadLibraryA("vulkan-1.dll");
}

VkResult VKAPI_CALL SteamVRHook_CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
	VkResult result = top_origCreateInstance(pCreateInfo, pAllocator, pInstance);
	steamInstances.emplace_back(*pInstance);
	logPrint(std::format("Created new NESTED instance {} with these instance extensions:", (void*)*pInstance));
	for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
		logPrint(std::format(" - {}", pCreateInfo->ppEnabledExtensionNames[i]));
	}
	return result;
}

VkResult VKAPI_CALL SteamVRHook_CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
	VkResult result = top_origCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
	steamDevices.emplace_back(*pDevice);
	logPrint(std::format("Created new NESTED device {} with these device extensions:", (void*)*pDevice));
	for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
		logPrint(std::format(" - {}", pCreateInfo->ppEnabledExtensionNames[i]));
	}
	return result;
}

PFN_vkVoidFunction VKAPI_CALL SteamVRHook_GetInstanceProcAddr(VkInstance instance, const char* pName) {
	PFN_vkGetInstanceProcAddr top_InstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(reinterpret_cast<HMODULE>(vulkanModule), "vkGetInstanceProcAddr"));

	// For Swapchain
	if (vkSharedInstance == nullptr || vkSharedDevice == nullptr) {
	}
	else {
		if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
			logDebugPrintAddr(std::format("Hooked vkGetDeviceProcAddr: {} {}", pName, (void*)instance))
			return (PFN_vkVoidFunction)saved_GetDeviceProcAddr;
		}

		PFN_vkVoidFunction funcRet = saved_GetInstanceProcAddr(vkSharedInstance, pName);
		if (funcRet == nullptr) {
			funcRet = saved_GetDeviceProcAddr(vkSharedDevice, pName);
			if (funcRet == nullptr) {
				//funcRet = top_InstanceProcAddr(vkSharedInstance, pName);
				logDebugPrintAddr(std::format("Wasn't able to get address from top: {} {} {}", pName, (void*)vkSharedInstance, (void*)funcRet));
			}
			else {
				//logPrint(std::format("Got address from top: {} {} {}", pName, (void*)vkSharedInstance, (void*)funcRet));
			}
		}
		else {
#ifdef ENABLE_FUNC_LOGGING
			logPrint(std::format("Got address from saved instance: {} {} (shared = {}) {}", pName, (void*)vkSharedInstance, (void*)vkSharedInstance, (void*)funcRet));
#endif
		}
		return funcRet;
	}


	if (strcmp(pName, "vkCreateInstance") == 0) {
		top_origCreateInstance = (PFN_vkCreateInstance)saved_GetInstanceProcAddr(instance, pName);
		return (PFN_vkVoidFunction)SteamVRHook_CreateInstance;
	}

	if (strcmp(pName, "vkCreateDevice") == 0) {
		top_origCreateDevice = (PFN_vkCreateDevice)saved_GetInstanceProcAddr(steamInstances[0], pName);
		return (PFN_vkVoidFunction)SteamVRHook_CreateDevice;
	}

	//if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
	//	return (PFN_vkVoidFunction)Layer_NESTED_TOP_GetDeviceProcAddr;
	//}

	// Required to self-intercept for compatibility
	if (strcmp(pName, "vkGetInstanceProcAddr") == 0)
		return (PFN_vkVoidFunction)SteamVRHook_GetInstanceProcAddr;

	PFN_vkVoidFunction funcRet = saved_GetInstanceProcAddr(vkSharedInstance, pName);
	if (funcRet == nullptr) {
		if (!steamInstances.empty()) funcRet = saved_GetInstanceProcAddr(steamInstances[0], pName);
		else funcRet = top_InstanceProcAddr(instance, pName);
#ifdef ENABLE_FUNC_LOGGING
		logPrint(std::format("Couldn't resolve using GetInstanceProcAddr, used top-level hook: {} {} {}", pName, (void*)instance, (void*)funcRet));
#endif
	}
	else {
#ifdef ENABLE_FUNC_LOGGING
		logPrint(std::format("Could resolve using GetInstanceProcAddr: {} {} {}", pName, (void*)instance, (void*)funcRet));
#endif
	}
	return funcRet;
}

PFN_vkVoidFunction VKAPI_CALL SteamVRHook_GetDeviceProcAddr(VkDevice device, const char* pName) {
	PFN_vkGetDeviceProcAddr top_DeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(GetProcAddress(reinterpret_cast<HMODULE>(vulkanModule), "vkGetDeviceProcAddr"));

	PFN_vkVoidFunction funcRet = saved_GetDeviceProcAddr(steamDevices[0], pName);
#ifdef ENABLE_FUNC_LOGGING
	logPrint(std::format("Intercepted NESTED GetDeviceProcAddr load: {} {} {}", pName, (void*)device, (void*)funcRet));
#endif
	// Required to self-intercept for compatibility
	if (strcmp(pName, "vkGetDeviceProcAddr") == 0)
		return (PFN_vkVoidFunction)SteamVRHook_GetDeviceProcAddr;

	return funcRet;
}