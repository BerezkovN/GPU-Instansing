#include "MainComponentSystem.hpp"

#include <tracy/Tracy.hpp>

#include "../pch.hpp"
#include "MainRenderer.hpp"

#include <omp.h>

MainComponentSystem::MainComponentSystem() : m_entityCount(kMaxEntityCount / 10) {

    std::random_device rndDevice;
    std::mt19937 rndEngine(rndDevice());

    std::uniform_real_distribution<> offsetDist(-100, 100);
    std::uniform_real_distribution<> zDist(-1, 1);
    std::uniform_real_distribution<> amplitudeDist(0.7, 1.2);

    m_transforms.resize(kMaxEntityCount);
    m_sprites.resize(kMaxEntityCount);

    m_moveComponents.reserve(kMaxEntityCount);
    m_animations.reserve(kMaxEntityCount);

    for (uint32_t ind = 0; ind < kMaxEntityCount; ind++) {

        m_moveComponents.push_back(MoveComponent{
            .center = { offsetDist(rndEngine), offsetDist(rndEngine), zDist(rndEngine), 0 },
            .amplitude = static_cast<float>(amplitudeDist(rndEngine))
        });

        m_animations.push_back(Animation{
            .originalSprite = Sprite{
	            .topLeftX = 0,
                .bottomRightX = 1 / 8.0f,
                .topLeftY = 0,
                .bottomRightY = 1.0f
			},
            .frameCount = 8,
            .delay = 0.6f
        });
    }
}

void MainComponentSystem::Update() {

    ZoneScoped;

    const double currentTime = glfwGetTime();

    const MoveComponent* __restrict moveComponentsPtr = m_moveComponents.data();
	Transform* __restrict transformsPtr = m_transforms.data();

	#pragma omp parallel for schedule(static)
    for (int ind = 0; ind < m_entityCount; ind++) {

        auto translate = moveComponentsPtr[ind].center;
        translate.y += sin(ind + currentTime) * moveComponentsPtr[ind].amplitude;

        transformsPtr[ind].translate = translate;
    }

    const Animation* __restrict animationsPtr = m_animations.data();
    Sprite* __restrict spritesPtr = m_sprites.data();

	#pragma omp parallel for schedule(static)
    for (int ind = 0; ind < m_entityCount; ind++) {

        uint32_t currentFrame = static_cast<float>(currentTime) / animationsPtr[ind].delay * static_cast<float>(animationsPtr[ind].frameCount);
    	currentFrame += ind;  // For randomness

        const float uOffset = static_cast<float>(currentFrame) / static_cast<float>(animationsPtr[ind].frameCount);

        spritesPtr[ind].topLeftX     = animationsPtr[ind].originalSprite.topLeftX + uOffset;
        spritesPtr[ind].bottomRightX = animationsPtr[ind].originalSprite.bottomRightX + uOffset;
        spritesPtr[ind].topLeftY     = animationsPtr[ind].originalSprite.topLeftY;
        spritesPtr[ind].bottomRightY = animationsPtr[ind].originalSprite.bottomRightY;
    }

}

void MainComponentSystem::SetEntityCount(uint32_t newEntityCount) {
    m_entityCount = newEntityCount;
}

uint32_t MainComponentSystem::GetEntityCount() const {
    return m_entityCount;
}

const std::vector<MainComponentSystem::Transform>& MainComponentSystem::GetTransforms() const {
    return m_transforms;
}

const std::vector<MainComponentSystem::Sprite>& MainComponentSystem::GetSprites() const {
    return m_sprites;
}
