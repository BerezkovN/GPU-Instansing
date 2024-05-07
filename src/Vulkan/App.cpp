#include "App.hpp"

#include "pch.hpp"
#include "helpers/VkHelper.hpp"
#include "renderers/MainRenderPass.hpp"
#include "renderers/MainRenderPipeline.hpp"

namespace {
	void glfwWindowResizeCallback(GLFWwindow* window, int width, int height) {
		const auto app = static_cast<App*>(glfwGetWindowUserPointer(window));
	    app->HintWindowResize();
	}
}

App::App() {

    m_config = std::make_unique<Config>();
    spdlog::set_pattern("%^[%H:%M] [%l]%$ %v");

    this->InitializeWindow();
    this->InitializeVulkan();

    this->ChooseMainDevice();

    m_surface = std::make_unique<Surface>(this, m_window);
    m_mainDevice = Device::FindDevice(this, m_mainDeviceID);

    if (!m_mainDevice->DoesSupportRendering(m_surface.get())) {
        throw std::runtime_error("[App] Main device doesn't support rendering");
    }

    const auto graphicsQueue = m_mainDevice->AddQueue(DeviceQueue::Type::Graphics, 1.0f, m_surface.get());
    if (!graphicsQueue.has_value()) {
        throw std::runtime_error("[App] Could not create graphics queue");
    }
    m_graphicsQueue = graphicsQueue.value();
    spdlog::info("[App] Graphics queue: {} family, {} queue", m_graphicsQueue->GetFamilyIndex(), m_graphicsQueue->GetQueueIndex());

    m_transferQueue = m_mainDevice->AddQueue(DeviceQueue::Type::Transfer, 1.0f);
    if (!m_transferQueue.has_value()) {
        spdlog::warn("[App] Could not create transfer queue");
    }
    else {
        spdlog::info("[App] Transfer queue: {} family, {} queue", m_transferQueue.value()->GetFamilyIndex(), m_transferQueue.value()->GetQueueIndex());
    }

    m_mainDevice->Initialize();

    m_swapchain = std::make_unique<Swapchain>(this, m_surface.get(), m_mainDevice.get());

    m_renderPass = std::make_unique<MainRenderPass>(m_mainDevice.get(), m_swapchain.get());


    m_shaderManager = std::make_unique<ShaderManager>(m_mainDevice.get());
    const VkShaderModule fragModule = m_shaderManager->LoadShader("shaders/triangle.frag.spv");
    const VkShaderModule vertModule = m_shaderManager->LoadShader("shaders/triangle.vert.spv");

    std::optional<const DeviceQueue*> transferQueue = std::nullopt;
    if (m_transferQueue.has_value()) {
        transferQueue = m_transferQueue.value().get();
    }

    MainRenderPipeline::CreateDesc pipelineDesc = {
        .app = this,
        .framesInFlight = m_config->framesInFlight,
        .device = m_mainDevice.get(),
        .renderPass = m_renderPass.get(),
        .graphicsQueue = m_graphicsQueue.get(),
        .transferQueue = transferQueue,
        .fragmentShader = fragModule,
        .vertexShader = vertModule
    };
    m_renderPipeline = std::make_unique<MainRenderPipeline>(pipelineDesc);

    this->CreateFramebuffers();
    this->CreateSyncObjects();
}

void App::Run() {

    while (!glfwWindowShouldClose(m_window)) {
    	glfwPollEvents();

    	this->Update();

        // TODO: Make sure this is correct
        m_mainDevice->WaitIdle();
    }

}

void App::Destroy() {

    this->DestroySyncObjects();

    m_renderPipeline->Destroy();
    m_renderPass->Destroy();

    m_shaderManager->DestroyAllShaders();

    this->DestroyFramebuffers();
    m_swapchain->Destroy();
    m_surface->Destroy();
    m_mainDevice->Destroy();

    this->DestroyVulkan();
    this->DestroyWindow();
}


void App::HintWindowResize() {

    m_mustResize = true;
}

void App::InitializeWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(800, 600, "GPUInstancing", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, glfwWindowResizeCallback);
}

void App::DestroyWindow() {

    if (m_window == nullptr) {
        return;
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();

    m_window = nullptr;
}

void App::InitializeVulkan() {

    VkResult result = volkInitialize();

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Device] Unable to find vulkan loader: " + std::to_string(result));
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "GPUInstancing",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "None",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t extensionCount;
    uint32_t layerCount;

    const auto extensions = this->GetVulkanInstanceExtensions(extensionCount);
    const auto layers = this->GetVulkanValidationLayers(layerCount);

    const VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = layerCount,
            .ppEnabledLayerNames = layers,
            .enabledExtensionCount = extensionCount,
            .ppEnabledExtensionNames = extensions,
    };

    result = vkCreateInstance(&createInfo, nullptr, &m_instance);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Device] Could not create Vulkan instance: " + std::to_string(result));
    }

    volkLoadInstance(m_instance);
}

void App::DestroyVulkan() {
    vkDestroyInstance(m_instance, nullptr);
    volkFinalize();

    m_instance = VK_NULL_HANDLE;
}


void App::ChooseMainDevice() {

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("[App] No GPUs were found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    std::vector <DeviceID> deviceIDs;

    spdlog::info("[App] Detected GPUs on the current machine:");


    int index = 0;
    for (const auto& device : devices) {

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        auto deviceID = Device::CalculateID(deviceProperties);
        deviceIDs.push_back(deviceID);

        auto deviceName = vk_to_string(deviceProperties.deviceType);
        spdlog::info("\t[{}] {}: {}. ID: {}",
            index, deviceName,
            deviceProperties.deviceName, deviceID);

        index += 1;

    }

    if (m_config->mainDeviceID.has_value()) {
        m_mainDeviceID = m_config->mainDeviceID.value();
        spdlog::info("[App] Config queried a device with ID: {}", m_config->mainDeviceID.value());
        return;
    }

    spdlog::info("[App] Choose device: (device index)");

    int specifiedDeviceIndex;
    std::cin >> specifiedDeviceIndex;

    m_mainDeviceID = deviceIDs[specifiedDeviceIndex];
}



void App::Update() {

    vkWaitForFences(m_mainDevice->GetVkDevice(), 1, &m_submitFrameFences[m_currentFrameInFlight], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    if (m_mustResize) {

        this->ResizeImmediately();
        m_mustResize = false;

        return;
    }
    VkResult result = vkAcquireNextImageKHR(m_mainDevice->GetVkDevice(), m_swapchain->GetVkSwapchain(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrameInFlight], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_mustResize = true;
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("[App] Error acquiring next swap chain: " + std::to_string(result));
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        std::cout << "[App] The current swap chain is suboptimal\n";
    }

    vkResetFences(m_mainDevice->GetVkDevice(), 1, &m_submitFrameFences[m_currentFrameInFlight]);


    VkExtent2D swapchainExtent = m_swapchain->GetExtent();
    MainRenderPipeline::RecordDesc recordDesc = {
        .frameIndex = m_currentFrameInFlight,
        .renderArea = {
            .offset = {0, 0},
            .extent = swapchainExtent
        },
        .framebuffer = m_framebuffers[imageIndex],
        .imageAvailableSemaphore = m_imageAvailableSemaphores[m_currentFrameInFlight],
        .renderFinishedSemaphore = m_renderFinishedSemaphores[m_currentFrameInFlight],
        .submitFrameFence = m_submitFrameFences[m_currentFrameInFlight]
    };
    m_renderPipeline->RecordAndSubmit(recordDesc);

    VkSwapchainKHR swapchain = m_swapchain->GetVkSwapchain();
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrameInFlight],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(m_graphicsQueue->GetVkQueue(), &presentInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Error presenting graphics queue: " + std::to_string(result));
    }

    m_currentFrameInFlight = (m_currentFrameInFlight + 1) % m_config->framesInFlight;
}


void App::ResizeImmediately() {

    int width = 0, height = 0;
    this->GetScreenSize(width, height);

    // Do nothing when window gets minimized.
    while (width == 0 || height == 0) {
        this->GetScreenSize(width, height);
        glfwWaitEvents();
    }

    // TODO: Implement async swap chain recreation.
    m_mainDevice->WaitIdle();

    m_swapchain->Resize();

    this->DestroyFramebuffers();
    this->CreateFramebuffers();
}

void App::CreateFramebuffers() {

    m_framebuffers.resize(m_swapchain->GetImageCount());

    for (size_t ind = 0; ind < m_framebuffers.size(); ind++)
    {
        // Currently, supports only color attachment.
        VkImageView framebufferAttachments[]{ m_swapchain->GetImage(ind) };

        VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass->GetVkRenderPass(),
            .attachmentCount = 1,
            .pAttachments = framebufferAttachments,
            .width = m_swapchain->GetExtent().width,
            .height = m_swapchain->GetExtent().height,
            .layers = 1
        };

        const VkResult result = vkCreateFramebuffer(m_mainDevice->GetVkDevice(), &framebufferCreateInfo, nullptr, &m_framebuffers[ind]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[App] Could not create framebuffer " + std::to_string(ind) + ": " + std::to_string(result));
        }
    }
}

void App::DestroyFramebuffers() {
    for (const auto& framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_mainDevice->GetVkDevice(), framebuffer, nullptr);
    }

    m_framebuffers.clear();
}

void App::CreateSyncObjects() {

	constexpr VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	constexpr VkFenceCreateInfo fenceCreateInfo = {
    	.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    m_imageAvailableSemaphores.resize(m_config->framesInFlight);
    m_renderFinishedSemaphores.resize(m_config->framesInFlight);
    m_submitFrameFences.resize(m_config->framesInFlight);

    for (size_t ind = 0; ind < m_config->framesInFlight; ind++) {
		    
	    if (vkCreateSemaphore(m_mainDevice->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[ind]) != VK_SUCCESS ||
            vkCreateSemaphore(m_mainDevice->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[ind]) != VK_SUCCESS ||
            vkCreateFence(m_mainDevice->GetVkDevice(), &fenceCreateInfo, nullptr, &m_submitFrameFences[ind]) != VK_SUCCESS)
	    {
	        throw std::runtime_error("[App] Could not create sync objects");
	    }
    }

}

void App::DestroySyncObjects() {
    for (size_t ind = 0; ind < m_config->framesInFlight; ind++) {

        vkDestroySemaphore(m_mainDevice->GetVkDevice(), m_imageAvailableSemaphores[ind], nullptr);
        vkDestroySemaphore(m_mainDevice->GetVkDevice(), m_renderFinishedSemaphores[ind], nullptr);
        vkDestroyFence(m_mainDevice->GetVkDevice(), m_submitFrameFences[ind], nullptr);
    }

    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_submitFrameFences.clear();
}


const char* const* App::GetVulkanValidationLayers(uint32_t& layerCount) const {

#if !NDEBUG
    layerCount = static_cast<uint32_t>(m_config->vkValidationLayers.size());

    uint32_t supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    std::string missingLayer;
    const bool support = does_contain_names<VkLayerProperties>(m_config->vkValidationLayers, supportedLayers,
        [](auto& layerProperty) { return layerProperty.layerName; },
        missingLayer);

    if (!support) {
        throw std::runtime_error("[App] Instance does not support validation layer: " + missingLayer);
    }

    return m_config->vkValidationLayers.data();
#else
    layerCount = 0;
    return nullptr;
#endif
}

const char* const* App::GetVulkanInstanceExtensions(uint32_t& extensionCount) const {
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    spdlog::info("[App] Instance extensions:");
    for (int ind = 0; ind < extensionCount; ind++) {
        spdlog::info("\t{}", glfwExtensions[ind]);
    }
    // Add extra extensions if needed.

    return glfwExtensions;
}

void App::GetScreenSize(int &width, int &height) const {
    glfwGetFramebufferSize(m_window, &width, &height);
}

void App::SetSwapchainImageCount(uint32_t count) const {
    m_config->swapChainImageCount = count;
}

uint32_t App::GetSwapchainImageCount() const {
    return m_config->swapChainImageCount;
}

const Config* App::GetConfig() const {
    return m_config.get();
}

VkInstance App::GetVkInstance() const {
    return m_instance;
}
