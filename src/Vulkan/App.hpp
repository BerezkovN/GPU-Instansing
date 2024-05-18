#pragma once

#include <memory>

#include "helpers/Context.hpp"
#include "helpers/IRenderer.hpp"
#include "helpers/IRenderPass.hpp"

class App {
public:

    App();
	void Run();
    void Destroy();

private:

	void Update(const Context::RenderDesc& desc);

	std::unique_ptr<Context> m_context;
	std::unique_ptr<IRenderer> m_renderer;
	std::unique_ptr<IRenderPass> m_renderPass;

};
