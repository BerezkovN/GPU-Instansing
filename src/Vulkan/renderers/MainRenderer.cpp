#include "MainRenderer.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include "MainRenderPipeline.hpp"

#include "../pch.hpp"
#include "../App.hpp"
#include "../helpers/buffers/LocalBuffer.hpp"
#include "../helpers/textures/Sampler.hpp"
#include "../helpers/Shader.hpp"
#include "../helpers/ShaderLayout.hpp"
#include "../helpers/IRenderPass.hpp"


MainRenderer::MainRenderer(const Context* context) {

    m_context = context;

    this->CreateVertexBuffer();
    this->CreateIndexBuffer();
    this->CreateUniformBuffers();

    m_vertexShader = std::make_unique<Shader>(m_context->GetDevice(), "shaders/triangle.vert.spv", Shader::Type::Vertex);
    m_fragmentShader = std::make_unique<Shader>(m_context->GetDevice(), "shaders/triangle.frag.spv", Shader::Type::Fragment);

    m_shaderLayout = std::make_unique<ShaderLayout>(m_context->GetDevice(), m_vertexShader.get(), m_fragmentShader.get());
    m_mainRenderPipeline = std::make_unique<MainRenderPipeline>(m_context, m_shaderLayout.get());

    m_sampler = std::make_unique<Sampler>(m_context, "textures/Tree.png");

    m_shaderLayout->AttachBuffer("UniformBufferObject", m_uniformBuffer.get(), 0, m_uniformBuffer->GetBufferSize());
    m_shaderLayout->AttackSampler("texSampler", m_sampler.get());
}

void MainRenderer::Destroy() {

    m_sampler->Destroy();

    m_mainRenderPipeline->Destroy();
	m_vertexShader->Destroy();
    m_fragmentShader->Destroy();
    m_shaderLayout->Destroy();

    this->DestroyUniformBuffers();
    this->DestroyIndexBuffer();
    this->DestroyVertexBuffer();
}

void MainRenderer::Record(const MainRenderer::RecordDesc& desc) {

    const VkCommandBuffer commandBuffer = desc.commandBuffer;

    static bool show_demo_window = true;
    static bool show_another_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }


    VkClearValue clearValue = {
        .color = {.float32 = { clear_color.x, clear_color.y, clear_color.z, clear_color.w } }
    };

    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = desc.renderPass->GetVkRenderPass();
    info.framebuffer = desc.framebuffer;
    info.renderArea = desc.renderArea;
    info.clearValueCount = 1;
    info.pClearValues = &clearValue;
    vkCmdBeginRenderPass(desc.commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
    }

    const VkViewport viewport = {
        .x = 0, .y = 0,
        .width = static_cast<float>(desc.renderArea.extent.width),
        .height = static_cast<float>(desc.renderArea.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = desc.renderArea.extent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_mainRenderPipeline->GetVkPipeline());

    this->UpdateUniformBuffers();

    // Learn about vkCmdBindVertexBuffers
    const VkBuffer vertexBuffers[] = { m_vertexBuffer->GetVkBuffer() };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

    m_shaderLayout->BindDescriptors(commandBuffer);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void MainRenderer::CreateUniformBuffers() {

    GenericBuffer::Desc desc = {
        .bufferCreateInfo = VkBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(MainRenderer::UniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
        .memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    m_uniformBuffer = std::make_unique<GenericBuffer>(m_context, desc);
    m_uniformBuffer->MapMemory(m_uniformBuffer->GetBufferSize());

}

void MainRenderer::UpdateUniformBuffers() const {

    static auto startTime = std::chrono::high_resolution_clock::now();

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float>(currentTime - startTime).count();

    int width, height;
    m_context->GetScreenSize(width, height);

    MainRenderer::UniformBufferObject ubo = {
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 10.0f)
    };

    ubo.proj[1][1] *= -1;

    // TODO: Learn about push constants
    void* mappedUboPtr = m_uniformBuffer->GetMappedMemory();
    std::memcpy(mappedUboPtr, &ubo, sizeof(ubo));
}

void MainRenderer::DestroyUniformBuffers() {

    m_uniformBuffer->Destroy();
    m_uniformBuffer = nullptr;
}

void MainRenderer::CreateVertexBuffer() {
    const std::vector<MainRenderPipeline::Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, -1, {0.0, 0.0}},
        {{-0.5f,  0.5f, 0.0f}, 0xFF03102, {0.0, 1.0}},
        {{ 0.5f,  0.5f, 0.0f}, -1, {1.0, 1.0}},
        {{ 0.5f, -0.5f, 0.0f}, 0xFF03102, {1.0, 0.0}},
    };

    LocalBuffer::Desc desc = {
        .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .buffer = vertices.data(),
        .bufferSize = static_cast <uint32_t>(sizeof(MainRenderPipeline::Vertex) * vertices.size())
    };

    m_vertexBuffer = std::make_unique<LocalBuffer>(m_context, desc);
}

void MainRenderer::DestroyVertexBuffer() {
    m_vertexBuffer->Destroy();
    m_vertexBuffer = nullptr;
}

void MainRenderer::CreateIndexBuffer() {
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    LocalBuffer::Desc desc = {
        .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .buffer = indices.data(),
        .bufferSize = static_cast <uint32_t>(sizeof(uint16_t) * indices.size())
    };

    m_indexBuffer = std::make_unique<LocalBuffer>(m_context, desc);
}

void MainRenderer::DestroyIndexBuffer() {
    m_indexBuffer->Destroy();
    m_indexBuffer = nullptr;
}