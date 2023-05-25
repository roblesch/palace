#include "vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace graphics {

Vulkan::Vulkan(bool enableValidation)
{
    m_validation = enableValidation;

    // sdl2 initialization
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    m_window = SDL_CreateWindow("palace",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    // setup dynamic dispatcher
    auto vkGetInstanceProcAddr = m_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // sdl2 extensions
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
    vk::ApplicationInfo appInfo {
        .pApplicationName = "viewer",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "palace",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_0
    };

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo {};
    std::vector<const char*> validationLayers;

    // validation
    if (m_validation) {
        extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        debugInfo = vk_::debug::createInfo();
    }

    // instance
    vk::InstanceCreateInfo instanceInfo {
        .pNext = &debugInfo,
        .flags = flags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = uint32_t(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = uint32_t(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data()
    };
    m_instance = vk::createInstanceUnique(instanceInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance.get());

    // create surface
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(m_window, m_instance.get(), &surface);
    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(m_instance.get());
    m_surface = vk::UniqueSurfaceKHR(surface, deleter);

    // create device
    m_device = vk_::Device(m_instance.get(), m_surface.get());
}

Vulkan::~Vulkan()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Vulkan::run()
{
    while (true) {
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
        }
    }
}

}
