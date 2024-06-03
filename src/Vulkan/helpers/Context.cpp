#include "Context.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include <tracy/Tracy.hpp>

#include "IRenderPass.hpp"
#include "../pch.hpp"

#include "Surface.hpp"
#include "Swapchain.hpp"
#include "VkHelper.hpp"

namespace {
	void glfwWindowResizeCallback(GLFWwindow* window, int width, int height) {
		const auto context = static_cast<Context*>(glfwGetWindowUserPointer(window));
		context->HintWindowResize();
	}
}

Context::Context(const Context::CreateDesc& desc) {

    m_config = desc.config;
    m_renderPass = desc.renderPass;

	this->InitializeWindow();
	this->InitializeVulkan();

    m_surface = std::make_unique<Surface>(this, m_window);

    this->ChooseMainDevice();
	m_mainDevice = Device::FindDevice(this, m_mainDeviceID);

    if (!m_mainDevice->DoesSupportRendering(m_surface.get())) {
        throw std::runtime_error("[Context] Main device doesn't support rendering");
    }

    this->CreateQueues();
    m_mainDevice->Initialize();

    m_swapchain = std::make_unique<Swapchain>(this, m_surface.get(), m_mainDevice.get());

    m_renderPass->Initialize(m_mainDevice.get(), m_swapchain.get());
    this->CreateFramebuffers();
    this->CreateSyncObjects();

    this->CreateCommandPools();
    this->CreateCommandBuffers();

    this->InitializeImGui();
}

void Context::Destroy() {

    this->DestroyImGui();

    this->DestroyCommandBuffers();
    this->DestroyCommandPools();

    this->DestroySyncObjects();

    this->DestroyFramebuffers();
    m_swapchain->Destroy();
    m_surface->Destroy();
    m_mainDevice->Destroy();

    this->DestroyVulkan();
    this->DestroyWindow();
}

void Context::Run(const std::function<void(const Context::RenderDesc&)>& rendererCallback) {


    while (!glfwWindowShouldClose(m_window)) {

		ZoneScoped;
        glfwPollEvents();

        this->Update(rendererCallback);

        ZoneNamedN(waitIdleZone, "Wait Idle", true);
        // TODO: Make sure this is correct
        // TODO: Yeah, I don't remember why we need this.
        m_mainDevice->WaitIdle();

        // I think that it's logical to mark the frame after synchronization.
        tracy::Profiler::SendFrameMark(nullptr);
    }
}

void Context::HintWindowResize() {

    m_mustResize = true;
}

void Context::Update(const std::function<void(const Context::RenderDesc&)>& rendererCallback) {

    ZoneScoped;

    vkWaitForFences(m_mainDevice->GetVkDevice(), 1, &m_submitFrameFence, VK_TRUE, UINT64_MAX);


    uint32_t imageIndex;
    if (m_mustResize) {

        this->ResizeImmediately();
        m_mustResize = false;

        return;
    }

    VkResult result;
    {
        ZoneScopedN("Acquire image");
        result = vkAcquireNextImageKHR(m_mainDevice->GetVkDevice(), m_swapchain->GetVkSwapchain(), UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_mustResize = true;
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("[Context] Error acquiring next swap chain: " + std::to_string(result));
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        std::cout << "[Context] The current swap chain is suboptimal\n";
    }

    vkResetFences(m_mainDevice->GetVkDevice(), 1, &m_submitFrameFence);

    this->Render(rendererCallback, imageIndex);

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphore[] = { m_renderFinishedSemaphore };

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_graphicsCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphore
    };


    {
        ZoneScopedN("Graphics queue submit");
        result = vkQueueSubmit(m_graphicsQueue->GetVkQueue(), 1, &submitInfo, m_submitFrameFence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[MainRenderPipeline] Error submitting a graphics queue: " + std::to_string(result));
        }
    }

    VkSwapchainKHR swapchain = m_swapchain->GetVkSwapchain();
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    {
        ZoneScopedN("Graphics queue present");
        result = vkQueuePresentKHR(m_graphicsQueue->GetVkQueue(), &presentInfo);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[Context] Error presenting graphics queue: " + std::to_string(result));
        }
    }
}

void Context::Render(const std::function<void(const Context::RenderDesc&)>& rendererCallback, uint32_t imageIndex) {

	vkResetCommandBuffer(m_graphicsCommandBuffer, 0);

    constexpr VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(m_graphicsCommandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not begin graphics command buffer: " + std::to_string(result));
    }

    // User code here
    rendererCallback(RenderDesc{
        .commandBuffer = m_graphicsCommandBuffer,
        .framebuffer = m_framebuffers[imageIndex],
        .renderPass = m_renderPass
    });

    result = vkEndCommandBuffer(m_graphicsCommandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not end graphics command buffer: " + std::to_string(result));
    }
}


void Context::InitializeWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(800, 600, "GPUInstancing", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, glfwWindowResizeCallback);
}

void Context::DestroyWindow() {

    if (m_window == nullptr) {
        return;
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();

    m_window = nullptr;
}


void Context::InitializeVulkan() {

    VkResult result = volkInitialize();

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Unable to find vulkan loader: " + std::to_string(result));
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "GPUInstancing",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "None",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
    };

    const auto extensions = this->GetVulkanInstanceExtensions();
    const auto layers = this->GetVulkanValidationLayers();

    const VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
    };

    result = vkCreateInstance(&createInfo, nullptr, &m_instance);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not create Vulkan instance: " + std::to_string(result));
    }

    volkLoadInstance(m_instance);
}

void Context::DestroyVulkan() {
    vkDestroyInstance(m_instance, nullptr);
    volkFinalize();

    m_instance = VK_NULL_HANDLE;
}

void Context::CreateQueues() {

    const auto graphicsQueue = m_mainDevice->AddQueue(DeviceQueue::Type::Graphics, 1.0f, m_surface.get());
    if (!graphicsQueue.has_value()) {
        throw std::runtime_error("[Context] Could not create graphics queue");
    }
    m_graphicsQueue = graphicsQueue.value();
    spdlog::info("[Context] Graphics queue: {} family, {} queue", m_graphicsQueue->GetFamilyIndex(), m_graphicsQueue->GetQueueIndex());

    m_transferQueue = m_mainDevice->AddQueue(DeviceQueue::Type::Transfer, 1.0f);
    if (!m_transferQueue.has_value()) {
        spdlog::warn("[Context] Could not create transfer queue");
    }
    else {
        spdlog::info("[Context] Transfer queue: {} family, {} queue", m_transferQueue.value()->GetFamilyIndex(), m_transferQueue.value()->GetQueueIndex());
    }
}




void Context::CreateCommandPools() {
    const VkCommandPoolCreateInfo graphicsPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_graphicsQueue->GetFamilyIndex()
    };

    VkResult result = vkCreateCommandPool(m_mainDevice->GetVkDevice(), &graphicsPoolCreateInfo, nullptr, &m_graphicsCommandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not create a graphics command pool: " + std::to_string(result));
    }

    if (!m_transferQueue.has_value()) {
        return;
    }

    const VkCommandPoolCreateInfo transferPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_transferQueue.value()->GetFamilyIndex()
    };

    VkCommandPool transferCommandPool;
    result = vkCreateCommandPool(m_mainDevice->GetVkDevice(), &transferPoolCreateInfo, nullptr, &transferCommandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not create a transfer command pool: " + std::to_string(result));
    }

    m_transferCommandPool = transferCommandPool;
}

void Context::DestroyCommandPools() {

    if (m_transferCommandPool.has_value()) {
        vkDestroyCommandPool(m_mainDevice->GetVkDevice(), m_transferCommandPool.value(), nullptr);
    }
    vkDestroyCommandPool(m_mainDevice->GetVkDevice(), m_graphicsCommandPool, nullptr);

    m_transferCommandPool = VK_NULL_HANDLE;
    m_graphicsCommandPool = VK_NULL_HANDLE;
}

void Context::CreateCommandBuffers() {

    const VkCommandBufferAllocateInfo graphicsBuffersInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(m_mainDevice->GetVkDevice(), &graphicsBuffersInfo, &m_graphicsCommandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not allocate graphics command buffers: " + std::to_string(result));
    }

    const VkCommandBufferAllocateInfo transferBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_transferCommandPool.has_value() ? m_transferCommandPool.value() : m_graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    result = vkAllocateCommandBuffers(m_mainDevice->GetVkDevice(), &transferBufferInfo, &m_transferCommandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[Context] Could not allocate transfer command buffer: " + std::to_string(result));
    }
}

void Context::DestroyCommandBuffers() {

    vkFreeCommandBuffers(m_mainDevice->GetVkDevice(),
        m_transferCommandPool.has_value() ? m_transferCommandPool.value() : m_graphicsCommandPool,
        1, &m_transferCommandBuffer);

    vkFreeCommandBuffers(m_mainDevice->GetVkDevice(), m_graphicsCommandPool, 1, &m_graphicsCommandBuffer);

    m_graphicsCommandBuffer = VK_NULL_HANDLE;
    m_transferCommandBuffer = VK_NULL_HANDLE;

}


void Context::ChooseMainDevice() {

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("[Context] No GPUs were found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    std::vector <DeviceID> deviceIDs;

    spdlog::info("[Context] Detected GPUs on the current machine:");


    int index = 0;
    for (const auto& device : devices) {

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        auto deviceID = Device::CalculateID(deviceProperties);
        deviceIDs.push_back(deviceID);

        auto deviceName = VkHelper::DeviceTypeToString(deviceProperties.deviceType);
        spdlog::info("\t[{}] {}: {}. ID: {}",
            index, deviceName,
            deviceProperties.deviceName, deviceID);

        index += 1;

    }

    if (m_config->mainDeviceId.has_value()) {
        m_mainDeviceID = m_config->mainDeviceId.value();
        spdlog::info("[Context] Config queried a device with ID: {}", m_config->mainDeviceId.value());
        return;
    }

    spdlog::warn("[Context] Choose device: (device index)");

    int specifiedDeviceIndex;
    std::cin >> specifiedDeviceIndex;

    m_mainDeviceID = deviceIDs[specifiedDeviceIndex];
}




void Context::ResizeImmediately() {

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

void Context::CreateFramebuffers() {

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
            throw std::runtime_error("[Context] Could not create framebuffer " + std::to_string(ind) + ": " + std::to_string(result));
        }
    }
}

void Context::DestroyFramebuffers() {
    for (const auto& framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_mainDevice->GetVkDevice(), framebuffer, nullptr);
    }

    m_framebuffers.clear();
}

void Context::CreateSyncObjects() {

    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    constexpr VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };


    if (vkCreateSemaphore(m_mainDevice->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_mainDevice->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(m_mainDevice->GetVkDevice(), &fenceCreateInfo, nullptr, &m_submitFrameFence) != VK_SUCCESS)
    {
        throw std::runtime_error("[Context] Could not create sync objects");
    }


}

void Context::DestroySyncObjects() {

    vkDestroySemaphore(m_mainDevice->GetVkDevice(), m_imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_mainDevice->GetVkDevice(), m_renderFinishedSemaphore, nullptr);
    vkDestroyFence(m_mainDevice->GetVkDevice(), m_submitFrameFence, nullptr);

    m_imageAvailableSemaphore = VK_NULL_HANDLE;
    m_renderFinishedSemaphore = VK_NULL_HANDLE;
    m_submitFrameFence = VK_NULL_HANDLE;
}


void Context::InitializeImGui() {

	const VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1
    };

	const VkDescriptorPoolCreateInfo descriptorCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    vkCreateDescriptorPool(m_mainDevice->GetVkDevice(), &descriptorCreateInfo, nullptr, &m_imguiDescriptorPool);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
        return vkGetInstanceProcAddr(static_cast<VkInstance>(vulkan_instance), function_name);
    }, m_instance);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_mainDevice->GetVkPhysicalDevice();
    init_info.Device = m_mainDevice->GetVkDevice();
    init_info.QueueFamily = m_graphicsQueue->GetFamilyIndex();
    init_info.Queue = m_graphicsQueue->GetVkQueue();
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.RenderPass = m_renderPass->GetVkRenderPass();
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info);
}

void Context::DestroyImGui() {

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(m_mainDevice->GetVkDevice(), m_imguiDescriptorPool, nullptr);
    m_imguiDescriptorPool = VK_NULL_HANDLE;
}


std::vector<const char*> Context::GetVulkanInstanceExtensions() const {

	uint32_t extensionCount;

    // Gets freed after glfwTerminate.
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char*> result{};

	for (uint32_t ind = 0; ind < extensionCount; ind++) {
        result.push_back(glfwExtensions[ind]);
    }

    return result;
}


std::vector<const char*> Context::GetVulkanValidationLayers() const {

#if !NDEBUG

    std::vector<const char*> result{};

    uint32_t supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    for (const auto layer : m_config->vkValidationLayers) {

        const auto found = std::ranges::find_if(supportedLayers, [layer](const VkLayerProperties& layerProperty)
        {
            return std::strcmp(layerProperty.layerName, layer);
        });

        if (found == supportedLayers.end()) {
            spdlog::error("[Context] The queried validation layer is not supported: " + std::string(layer));
            continue;
        }

        result.push_back(layer);
    }

    std::string missingLayer;
    const bool support = VkHelper::DoesContainNames<VkLayerProperties>(m_config->vkValidationLayers, supportedLayers,
        [](auto& layerProperty) { return layerProperty.layerName; },
        missingLayer);

    if (!support) {
        throw std::runtime_error("[App] Instance does not support validation layer: " + missingLayer);
    }

    return result;
#else
    return std::vector<const char*>{};
#endif
}





void Context::GetScreenSize(int& width, int& height) const {
    glfwGetFramebufferSize(m_window, &width, &height);
}

const IRenderPass* Context::GetRenderPass() const {
    return m_renderPass;
}

const Swapchain* Context::GetSwapchain() const {
    return m_swapchain.get();
}

const Device* Context::GetDevice() const {
    return m_mainDevice.get();
}

const DeviceQueue* Context::GetGraphicsQueue() const {
    return m_graphicsQueue.get();
}

std::optional<const DeviceQueue*> Context::GetTransferQueue() const {

    std::optional<const DeviceQueue*> transferQueue = std::nullopt;
    if (m_transferQueue.has_value()) {
        transferQueue = m_transferQueue.value().get();
    }
    return transferQueue;
}

const DeviceQueue* Context::GetActualTransferQueue() const {
    return m_transferQueue.has_value() ? m_transferQueue.value().get() : m_graphicsQueue.get();
}

VkCommandBuffer Context::GetGraphicsCommandBuffer() const {
    return m_graphicsCommandBuffer;
}

VkCommandBuffer Context::GetTransferCommandBuffer() const {
    return m_transferCommandBuffer;
}

void Context::SetSwapchainImageCount(uint32_t count) const {
    m_config->swapChainImageCount = count;
}

uint32_t Context::GetSwapchainImageCount() const {
    return m_config->swapChainImageCount;
}

const Context::Config* Context::GetConfig() const {
    return m_config.get();
}

Context::ShareInfo Context::GetTransferShareInfo() const {
    std::vector<uint32_t> queueFamilyIndices;
    VkSharingMode sharingMode;

    if (!m_transferQueue.has_value() || m_graphicsQueue->GetFamilyIndex() == m_transferQueue.value()->GetFamilyIndex()) {
        queueFamilyIndices.push_back(m_graphicsQueue->GetFamilyIndex());
        sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else {
        queueFamilyIndices.push_back(m_graphicsQueue->GetFamilyIndex());
        queueFamilyIndices.push_back(m_transferQueue.value()->GetFamilyIndex());

        sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    return ShareInfo{
        .queueFamilyIndices = queueFamilyIndices,
        .sharingMode = sharingMode
    };
}

VkInstance Context::GetVkInstance() const {
    return m_instance;
}

GLFWwindow* Context::GetGlfwWindow() const {
    return m_window;
}
