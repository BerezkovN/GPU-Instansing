#pragma once

#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../helpers/IComponentSystem.hpp"

class MainComponentSystem : public IComponentSystem
{
public:

	static constexpr uint32_t kMaxEntityCount = 100000;

	struct Transform
	{
		glm::vec4 translate;
	};

	struct Sprite
	{
		float topLeftX;
		float bottomRightX;
		float topLeftY;
		float bottomRightY;
	};

	struct MoveComponent
	{
		glm::vec4 center;
		float amplitude;
	};

	struct Animation
	{
		Sprite originalSprite;

		uint32_t frameCount;
		float delay;
	};



	MainComponentSystem();

	void Update() override;

	void SetEntityCount(uint32_t newEntityCount);
	[[nodiscard]] uint32_t GetEntityCount() const;

	[[nodiscard]] const std::vector<Transform>& GetTransforms() const;
	[[nodiscard]] const std::vector<Sprite>& GetSprites() const;
private:

	uint32_t m_entityCount;

	std::vector<Transform> m_transforms;
	std::vector<MoveComponent> m_moveComponents;

	std::vector<Sprite> m_sprites;
	std::vector<Animation> m_animations;
};