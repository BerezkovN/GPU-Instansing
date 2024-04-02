#include "App.hpp"

#include <algorithm>
#include <limits>

#include "pch.hpp"
#include "VkHelper.hpp"

void App::Start() {

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow_ = glfwCreateWindow(800, 600, "GPUInstancing", nullptr, nullptr);

    this->InitializeVulkan();
    this->CreateSurface();
    this->PickGPU();
    this->CreateLogicalDevice();
    this->CreateSwapChain();
    this->CreateSwapChainImageViews();
    this->CreateRenderPass();
    this->CreateGraphicsPipeline();
    this->CreateFramebuffers();
    this->CreateCommandPool();
    this->CreateCommandBuffer();
    this->CreateSyncObjects();

    while (!glfwWindowShouldClose(glfwWindow_)) {
    	glfwPollEvents();

    	this->Update();

        vkDeviceWaitIdle(vkLogicalDevice_);
    }

    this->DestroyVulkan();

    glfwDestroyWindow(glfwWindow_);
    glfwTerminate();
 
}


void App::InitializeVulkan() {

    VkResult result = volkInitialize();
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Unable to find vulkan loader: " + std::to_string(result));
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

    const auto extensions = this->GetVulkanExtensions(extensionCount);
    const auto layers = this->GetVulkanValidationLayers(layerCount);

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layers,
    	.enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensions,
    };

    result = vkCreateInstance(&createInfo, nullptr, &vkInstance_);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create Vulkan instance: " + std::to_string(result));
    }

    volkLoadInstance(vkInstance_);
}

void App::DestroyVulkan() const {

    vkDestroySemaphore(vkLogicalDevice_, imageAvailableSemaphore_, nullptr);
    vkDestroySemaphore(vkLogicalDevice_, renderFinishedSemaphore_, nullptr);
    vkDestroyFence(vkLogicalDevice_, inFlightFence_, nullptr);

    vkDestroyCommandPool(vkLogicalDevice_, vkGraphicsCommandPool_, nullptr);

    for (const auto& framebuffer : vkSwapChainFramebuffers_) {
        vkDestroyFramebuffer(vkLogicalDevice_, framebuffer, nullptr);
    }

    vkDestroyPipeline(vkLogicalDevice_, vkGraphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(vkLogicalDevice_, vkPipelineLayout_, nullptr);
    vkDestroyRenderPass(vkLogicalDevice_, vkRenderPass_, nullptr);

    shaderManager_->DestroyAllShaders();

    for (const auto imageView : vkSwapChainImageViews_) {
        vkDestroyImageView(vkLogicalDevice_, imageView, nullptr);
    }

    vkDestroySwapchainKHR(vkLogicalDevice_, vkSwapChain_, nullptr);
    vkDestroyDevice(vkLogicalDevice_, nullptr);
    vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
    vkDestroyInstance(vkInstance_, nullptr);
    volkFinalize();
}

void App::Update() {

    vkWaitForFences(vkLogicalDevice_, 1, &inFlightFence_, VK_TRUE, UINT64_MAX);
    vkResetFences(vkLogicalDevice_, 1, &inFlightFence_);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkLogicalDevice_, vkSwapChain_, UINT64_MAX, imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(vkGraphicsCommandBuffer_, 0);
    this->RecordCommandBuffer(vkGraphicsCommandBuffer_, imageIndex);

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore_ };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphore[] = { renderFinishedSemaphore_ };

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGraphicsCommandBuffer_,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphore
    };


    VkResult result = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Error submitting a graphics queue: " + std::to_string(result));
    }

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vkSwapChain_,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(graphicsQueue_, &presentInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Error presenting graphics queue: " + std::to_string(result));
    }
}

void App::RecordCommandBuffer(const VkCommandBuffer commandBuffer, const uint32_t imageIndex) {

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        // TODO: Learn about this
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not begin command buffer: " + std::to_string(result));
    }

    VkClearValue clearValue = {
        .color = {0, 0, 0, 1.0f}
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vkRenderPass_,
        .framebuffer = vkSwapChainFramebuffers_[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = vkSwapChainExtent_
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline_);

    const VkViewport viewport = {
	    .x = 0, .y = 0,
	    .width = static_cast<float>(vkSwapChainExtent_.width),
	    .height = static_cast<float>(vkSwapChainExtent_.height),
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = vkSwapChainExtent_
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not end command buffer: " + std::to_string(result));
    }

}

void App::CreateSurface() {

	const VkResult result = glfwCreateWindowSurface(vkInstance_, glfwWindow_, nullptr, &vkSurface_);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create Vulkan surface: " + std::to_string(result));
    }
}

void App::CreateLogicalDevice() {

    VkDeviceQueueCreateInfo graphicsQueueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueIndex_.value(),
        .queueCount = 1,
        .pQueuePriorities = &graphicsQueuePriority_
    };

    VkPhysicalDeviceFeatures deviceFeatures{};

    // Used for compatibility with older devices.
    uint32_t layerCount;
    const auto layers = this->GetVulkanValidationLayers(layerCount);

    const VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &graphicsQueueInfo,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = static_cast<uint32_t>(vkDeviceExtensions_.size()),
        .ppEnabledExtensionNames = vkDeviceExtensions_.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    const VkResult result = vkCreateDevice(vkPhysicalDevice_, &deviceCreateInfo, nullptr, &vkLogicalDevice_);

    // https://gpuopen.com/learn/reducing-vulkan-api-call-overhead/
    volkLoadDevice(vkLogicalDevice_);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create logical device: " + std::to_string(result));
    }

    vkGetDeviceQueue(vkLogicalDevice_, graphicsQueueIndex_.value(), 0, &graphicsQueue_);
}

void App::CreateSwapChain() {

    const SwapChainSupportInfo swapChainInfo = this->QuerySwapChainSupport(vkPhysicalDevice_);

    const VkSurfaceFormatKHR surfaceFormat = this->ChooseSurfaceFormat(swapChainInfo.formats);
    const VkPresentModeKHR presentMode = this->ChoosePresentMode(swapChainInfo.presentModes);
    const VkExtent2D extent = this->ChooseSwapExtent(swapChainInfo.capabilities);

    std::cout << "[App] Maximum supported number of swap chain images by the device: " << swapChainInfo.capabilities.maxImageCount << '\n';
    if (swapChainImageCount_ > swapChainInfo.capabilities.maxImageCount) {
        std::cout << "[App] The device doesn't support swap chain image count of " << swapChainImageCount_ << '\n';
        swapChainImageCount_ = swapChainInfo.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkSurface_,
        .minImageCount = swapChainImageCount_,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    // TODO: implement separation of presentation and graphics queue.
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;

    VkResult result = vkCreateSwapchainKHR(vkLogicalDevice_, &createInfo, nullptr, &vkSwapChain_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create swap chain: " + std::to_string(result));
    }

    vkSwapChainImages_.resize(swapChainImageCount_);
    result = vkGetSwapchainImagesKHR(vkLogicalDevice_, vkSwapChain_, &swapChainImageCount_, vkSwapChainImages_.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not retrieve swap chain images: " + std::to_string(result));
    }

    vkSwapChainExtent_ = extent;
    vkSwapChainImageFormat_ = surfaceFormat.format;
}

void App::CreateSwapChainImageViews() {

    vkSwapChainImageViews_.resize(vkSwapChainImages_.size());

    for (size_t ind = 0; ind < vkSwapChainImages_.size(); ind++) {

        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vkSwapChainImages_[ind],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vkSwapChainImageFormat_,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        const VkResult result = vkCreateImageView(vkLogicalDevice_, &createInfo, nullptr, &vkSwapChainImageViews_[ind]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[App] Could not create a swap chain image view: " + std::to_string(result));
        }
    }

}

void App::CreateRenderPass() {

    VkAttachmentDescription attachmentDescription = {
        .format = vkSwapChainImageFormat_,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency subpassDependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    const VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency
    };

    const VkResult result = vkCreateRenderPass(vkLogicalDevice_, &renderPassCreateInfo, nullptr, &vkRenderPass_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create render pass: " + std::to_string(result));
    }
}

void App::CreateGraphicsPipeline() {

    shaderManager_ = std::make_unique<ShaderManager>(vkLogicalDevice_);

    const VkShaderModule fragModule = shaderManager_->LoadShader("shaders/triangle.frag.spv");
    const VkShaderModule vertModule = shaderManager_->LoadShader("shaders/triangle.vert.spv");

    // TODO: Use pUseSpecializationInfo for GPU Instancing variants.
    const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertModule,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragModule,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Viewport is a dynamic state and updated later.
    VkPipelineViewportStateCreateInfo viewportCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    // TODO: Implement depth.
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    VkPipelineColorBlendAttachmentState blendAttachmentState = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentState,
    };

    // TODO: Learn more about this
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    VkResult result = vkCreatePipelineLayout(vkLogicalDevice_, &pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create pipeline layout: " + std::to_string(result));
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &blendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = vkPipelineLayout_,
        .renderPass = vkRenderPass_,
        .subpass = 0,
        // Learn more about this
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(vkLogicalDevice_, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vkGraphicsPipeline_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create graphics pipeline: " + std::to_string(result));
    }

    std::cout << "[App] Graphics pipeline was created successfully!\n";
}

void App::CreateFramebuffers() {

    vkSwapChainFramebuffers_.resize(vkSwapChainImageViews_.size());

    for (size_t ind = 0; ind < vkSwapChainFramebuffers_.size(); ind++)
    {
        // Currently, supports only color attachment.
        VkImageView framebufferAttachments[]{ vkSwapChainImageViews_[ind] };

        VkFramebufferCreateInfo framebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vkRenderPass_,
            .attachmentCount = 1,
            .pAttachments = framebufferAttachments,
            .width = vkSwapChainExtent_.width,
            .height = vkSwapChainExtent_.height,
            .layers = 1
        };

        const VkResult result = vkCreateFramebuffer(vkLogicalDevice_, &framebufferCreateInfo, nullptr, &vkSwapChainFramebuffers_[ind]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("[App] Could not create framebuffer " + std::to_string(ind) + ": " + std::to_string(result));
        }
    }
}

void App::CreateCommandPool() {

    const VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsQueueIndex_.value()
    };

    const VkResult result = vkCreateCommandPool(vkLogicalDevice_, &createInfo, nullptr, &vkGraphicsCommandPool_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not create a graphics command pool: " + std::to_string(result));
    }
}

void App::CreateCommandBuffer() {

	const VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkGraphicsCommandPool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    const VkResult result = vkAllocateCommandBuffers(vkLogicalDevice_, &allocateInfo, &vkGraphicsCommandBuffer_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[App] Could not allocate graphics command buffer: " + std::to_string(result));
    }
}

void App::CreateSyncObjects() {

	constexpr VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	constexpr VkFenceCreateInfo fenceCreateInfo = {
    	.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if (vkCreateSemaphore(vkLogicalDevice_, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
        vkCreateSemaphore(vkLogicalDevice_, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS ||
        vkCreateFence(vkLogicalDevice_, &fenceCreateInfo, nullptr, &inFlightFence_) != VK_SUCCESS) 
    {
        throw std::runtime_error("[App] Could not create sync objects");
    }

}


void App::PickGPU() {
    uint32_t deviceCount;

    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("[App] No GPUs were found");
    }

    std::vector<VkPhysicalDevice> gpus(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, gpus.data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto& gpu : gpus) {
	    if (this->IsGPUSupported(gpu)) {
            physicalDevice = gpu;
            break;
	    }
    }


    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("[App] No suitable GPUs were found");
    }

    vkPhysicalDevice_ = physicalDevice;
}

bool App::IsGPUSupported(const VkPhysicalDevice physicalDevice) {

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    std::cout << "[App] " << vk_to_string(deviceProperties.deviceType) << " : " << deviceProperties.deviceName << '\n';

    this->FindFamilyQueues(physicalDevice);

    const bool supportsExtensions = this->DoesSupportDeviceExtensions(physicalDevice);

    bool supportsSwapChain = false;
    if (supportsExtensions) {
	    const SwapChainSupportInfo swapInfo = this->QuerySwapChainSupport(physicalDevice);
        supportsSwapChain = !swapInfo.presentModes.empty() && !swapInfo.formats.empty();
    }

    return
		deviceProperties.deviceType == vkRequiredDeviceType_ &&
        supportsExtensions &&
        supportsSwapChain &&
        graphicsQueueIndex_.has_value();
}

void App::FindFamilyQueues(const VkPhysicalDevice physicalDevice) {

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t index{};

    std::cout << "[App] Supported queue families:\n";
    for (const auto& queueFamily : queueFamilyProperties) {

        std::cout << "\tQueue count: " << queueFamily.queueCount << '\n';
        std::cout << "\tQueue flags: " << vk_to_string(queueFamily.queueFlags) << '\n';

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

            VkBool32 isSurfaceSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, vkSurface_, &isSurfaceSupported);

            if (!isSurfaceSupported) {
                continue;
            }

            graphicsQueueIndex_ = index;
        }

        index++;
    }
}

bool App::DoesSupportDeviceExtensions(const VkPhysicalDevice physicalDevice) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProperties.data());

    return does_contain_names<VkExtensionProperties>(vkDeviceExtensions_, extensionProperties, [](auto& property) { return property.extensionName; });
}

App::SwapChainSupportInfo App::QuerySwapChainSupport(const VkPhysicalDevice physicalDevice) const {
    SwapChainSupportInfo details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkSurface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface_, &formatCount, nullptr);
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkSurface_, &formatCount, details.formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface_, &presentModeCount, nullptr);
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkSurface_, &presentModeCount, details.presentModes.data());

    return details;
}

VkSurfaceFormatKHR App::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {

	for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
	}

    std::cout << "[App] Could not find the specified swapchain format. Using a default format\n";

    return formats[0];
}

VkPresentModeKHR App::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const {

    if (std::ranges::find(presentModes, VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D App::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {

    // Special value.
    if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
        std::cout << "[App] Not High DPI\n";
        return capabilities.currentExtent;
    }

    std::cout << "[App] High DPI\n";

    int width, height;
    glfwGetFramebufferSize(glfwWindow_, &width, &height);

    VkExtent2D actualExtent = {
	    static_cast<uint32_t>(width),
	    static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

const char* const* App::GetVulkanValidationLayers(uint32_t& layerCount) const {

#if !NDEBUG
    layerCount = static_cast<uint32_t>(vkValidationLayers_.size());

    uint32_t supportedLayerCount;
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);

    std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
    vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());

    does_contain_names<VkLayerProperties>(
        vkValidationLayers_, supportedLayers, 
        [](auto& layerProperty) { return layerProperty.layerName; });

    return vkValidationLayers_.data();
#else
    layerCount = 0;
    return nullptr;
#endif
}

const char* const* App::GetVulkanExtensions(uint32_t& extensionCount) const {
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    // Add extra extensions if needed.

    return glfwExtensions;
}
