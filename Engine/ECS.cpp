#include "ECS.h"

ECS::ECS()
{
}

ECS::~ECS()
{
}

ECS* ECS::addEntity(std::shared_ptr<Entity> entity)
{
	entities[entity->id] = entity;

	changeFlag = true;

	return this;
}

std::shared_ptr<Entity> ECS::getEntityById(uint32_t id)
{
	return entities[id];
}

ECS* ECS::removeEntity(uint32_t id)
{
	entities.erase(id);

	changeFlag = true;

	return this;
}

void ECS::onEachEntity(std::function<void(std::shared_ptr<Entity>)> callback)
{
	for (const auto& entityPair : entities) {
		callback(entityPair.second);
	}
}
