#include "ECS.h"

ECS::ECS()
{
}

ECS::~ECS()
{
}

void ECS::addEntity(std::shared_ptr<Entity> entity)
{
	entities[entity->id] = entity;
}

std::shared_ptr<Entity> ECS::getEntityById(uint32_t id)
{
	return entities[id];
}

void ECS::onEachEntity(std::function<void(std::shared_ptr<Entity>)> callback)
{
	for (const auto& entityPair : entities) {
		callback(entityPair.second);
	}
}
