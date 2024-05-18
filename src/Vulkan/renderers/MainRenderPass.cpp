#include "MainRenderPass.hpp"

#include "../pch.hpp"
#include "../helpers/Swapchain.hpp"
#include "../helpers/Device.hpp"

void MainRenderPass::Initialize(const Device* device, const Swapchain* swapchain) {

    m_device = device;

    VkAttachmentDescription attachmentDescription = {
        .format = swapchain->GetFormat(),
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

    // TODO: Learn more about this
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

    const VkResult result = vkCreateRenderPass(device->GetVkDevice(), &renderPassCreateInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("[MainRenderPass] Could not create render pass: " + std::to_string(result));
    }
}

void MainRenderPass::Destroy() {
    vkDestroyRenderPass(m_device->GetVkDevice(), m_renderPass, nullptr);
}

VkRenderPass MainRenderPass::GetVkRenderPass() const {
    return m_renderPass;
}
