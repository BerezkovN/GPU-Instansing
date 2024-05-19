#include "App.hpp"

#include "pch.hpp"
#include "helpers/Swapchain.hpp"
#include "helpers/VkHelper.hpp"
#include "renderers/MainRenderer.hpp"
#include "renderers/MainRenderPass.hpp"



App::App() {

    spdlog::set_pattern("%^[%H:%M] [%l]%$ %v");

    m_renderPass = std::make_unique<MainRenderPass>();

    const auto config = std::make_shared <Context::Config>();
    config->vkValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
    config->vkDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    config->vkPreferredSurfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
    config->vkPreferredSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    config->vkPreferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    config->swapChainImageCount = 2;
    config->useImGui = true;

    Context::CreateDesc contextDesc = {
        .config = config,
        .renderPass = m_renderPass.get()
    };
    m_context = std::make_unique<Context>(contextDesc);

    m_renderer = std::make_unique<MainRenderer>(m_context.get());
}

void App::Run() {

    m_context->Run([this](const Context::RenderDesc& desc)
    {
        this->Update(desc);
    });
}

void App::Destroy() {

    m_renderPass->Destroy();
    m_renderer->Destroy();
    m_context->Destroy();

    m_renderPass = nullptr;
    m_renderer = nullptr;
    m_context = nullptr;
}

void App::Update(const Context::RenderDesc& desc) {

    const VkExtent2D swapchainExtent = m_context->GetSwapchain()->GetExtent();
    const MainRenderer::RecordDesc recordDesc = {
        .renderArea = {
            .offset = {0, 0},
            .extent = swapchainExtent
        },
        .commandBuffer = desc.commandBuffer,
        .renderPass = desc.renderPass,
        .framebuffer = desc.framebuffer,
    };
    m_renderer->Record(recordDesc);

}

