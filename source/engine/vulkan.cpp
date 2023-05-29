#include "vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace engine {

Vulkan::Vulkan(bool enableValidation)
    : m_validation(enableValidation)
{
    // dynamic dispatch loader
    auto vkGetInstanceProcAddr = m_dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // sdl2
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    m_window = SDL_CreateWindow(
        "palace", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, s_windowWidth, s_windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    unsigned int extensionCount;
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, nullptr);
    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, extensionNames.data());

    // molten vk
    vk::InstanceCreateFlagBits flags {};
#ifdef __APPLE__
    extensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    extensionNames.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    // application
    vk::ApplicationInfo appInfo { .pApplicationName = "viewer", .pEngineName = "palace", .apiVersion = VK_API_VERSION_1_0 };

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo {};
    std::vector<const char*> validationLayers;

    // validation
    if (m_validation) {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        debugInfo = vk_::debug::createInfo();
    }

    // instance
    vk::InstanceCreateInfo instanceInfo { .pNext = &debugInfo,
        .flags = flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data() };
    m_uniqueInstance = vk::createInstanceUnique(instanceInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_uniqueInstance.get());

    // surface
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, m_uniqueInstance.get(), &surface);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(m_uniqueInstance.get());
    m_uniqueSurface = vk::UniqueSurfaceKHR(surface, deleter);

    // device
    m_device = vk_::Device(m_uniqueInstance.get(), m_uniqueSurface.get());

    // swapchain
    m_swapchain = vk_::Swapchain(m_window, m_uniqueSurface.get(), m_device.physicalDevice(), m_device.device());

    m_initialized = true;
}

Vulkan::~Vulkan()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Vulkan::run() const
{
    if (!m_initialized) {
        return;
    }
    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
        }
    }
}

} // namespace engine
