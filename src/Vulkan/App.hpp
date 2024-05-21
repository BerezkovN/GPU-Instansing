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
	void OnInitializeRenderer();

	std::unique_ptr<Context> m_context;
	std::unique_ptr<IRenderer> m_renderer;
	std::unique_ptr<IRenderPass> m_renderPass;

	std::vector<const char*> m_rendererLabels{
		"Instanced Coherent Normal",
		"Instanced Cached Normal"
	};

	enum Renderers
	{
		InstancedCoherentDefault,
		InstancedCachedDefault
	};
	Renderers m_selectedRenderer = InstancedCoherentDefault;
};
