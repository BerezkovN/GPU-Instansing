#include "App.hpp"

#include "pch.hpp"
#include "helpers/Swapchain.hpp"
#include "helpers/VkHelper.hpp"
#include "renderers/MainRenderer.hpp"
#include "renderers/MainRenderPass.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include "renderers/InstancedCoherentRenderer.hpp"
#include "renderers/InstancedCachedRenderer.hpp"

App::App() {

    spdlog::set_pattern("%^[%H:%M] [%l]%$ %v");

    m_renderPass = std::make_unique<MainRenderPass>();

    const auto config = std::make_shared <Context::Config>();
    config->vkValidationLayers.push_back("VK_LAYER_KHRONOS_validation");
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

    OnInitializeRenderer();
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

    static float clearColor[4]{ 0.2f, 0.25f, 0.45f, 1.0f };
    const VkClearValue clearValue = {
		.color = {.float32 = {clearColor[0], clearColor[1], clearColor[2], clearColor[3]}}
    };

    const VkRenderPassBeginInfo info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .renderPass = desc.renderPass->GetVkRenderPass(),
	    .framebuffer = desc.framebuffer,
	    .renderArea = {
	        .offset = {0, 0},
	        .extent = m_context->GetSwapchain()->GetExtent()
    },
	    .clearValueCount = 1,
	    .pClearValues = &clearValue
    };
    vkCmdBeginRenderPass(desc.commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    bool open = true;
    ImGui::Begin("Debug", &open);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::ColorEdit3("Clear color", clearColor);

    ImGui::ShowDemoWindow();

    ImGui::Combo("Renderer", reinterpret_cast<int*>(&m_selectedRenderer), m_rendererLabels.data(), static_cast<int>(m_rendererLabels.size()));

    ImGui::SameLine();
    if (ImGui::Button("Initialize Renderer")) {
        this->OnInitializeRenderer();
    }

    const MainRenderer::RecordDesc recordDesc = {
        .renderArea = {
            .offset = {0, 0},
            .extent = m_context->GetSwapchain()->GetExtent()
        },
        .commandBuffer = desc.commandBuffer,
        .renderPass = desc.renderPass,
        .framebuffer = desc.framebuffer,
    };
    m_renderer->Record(recordDesc);


    ImGui::End();
    ImGui::Render();
    ImDrawData* imGuiDrawData = ImGui::GetDrawData();
    const bool isMinimized = (imGuiDrawData->DisplaySize.x <= 0.0f || imGuiDrawData->DisplaySize.y <= 0.0f);
    if (!isMinimized)
    {
        ImGui_ImplVulkan_RenderDrawData(imGuiDrawData, desc.commandBuffer);
    }

    vkCmdEndRenderPass(desc.commandBuffer);
}

void App::OnInitializeRenderer() {

    if (m_renderer != nullptr) {
		m_renderer->Destroy();
    }

    switch (m_selectedRenderer) {
    case InstancedCoherentDefault:
        m_renderer = std::make_unique<InstancedCoherentRenderer>(m_context.get());
        m_renderer->Initialize("shaders/triangle.vert.spv", "shaders/triangle.frag.spv");
        break;
    case InstancedCachedDefault:
        m_renderer = std::make_unique<InstancedCachedRenderer>(m_context.get());
        m_renderer->Initialize("shaders/triangle.vert.spv", "shaders/triangle.frag.spv");
	    break;
    }
}

