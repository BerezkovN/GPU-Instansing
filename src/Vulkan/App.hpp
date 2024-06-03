#pragma once

#include <memory>

#include "helpers/Context.hpp"
#include "helpers/IRenderer.hpp"
#include "helpers/IRenderPass.hpp"

/* * *
 * NOTES:
 *
 * [] Check if alignment of 64 (cache line) makes any difference
 * [] ALWAYS check for reads from VRAM
 * [X] Implement memory heap allocation class and support for BAR
 * [-] Benchmark graphics vs transfer queue for copy operations
 */

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
