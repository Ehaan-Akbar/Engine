#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Entity.h"
#include "Component.h"

class ECS
{
public:

	ECS();
	~ECS();


	ECS* addEntity(std::shared_ptr<Entity> entity);
	std::shared_ptr<Entity> getEntityById(uint32_t id);
	ECS* removeEntity(uint32_t id);

	template <typename T> ECS* addComponent(std::shared_ptr<Entity> entity, std::shared_ptr<T> component);
	template <typename T> std::shared_ptr<T> getComponent(std::shared_ptr<Entity> entity);
	template <typename T> ECS* removeComponent(uint32_t entityId, std::type_index componentType);

	void onEachEntity(std::function<void(std::shared_ptr<Entity>)> callback);

	size_t getEntityCount() {
		return entities.size();
	}

	bool changeFlag = true;
	void resetChangeFlag() {
		changeFlag = false;
	}

private:

	std::unordered_map<uint32_t, std::shared_ptr<Entity>> entities;
	std::unordered_map<uint32_t, std::unordered_map<std::type_index, std::shared_ptr<Component>>> components;
};

template<typename T> inline ECS* ECS::addComponent(std::shared_ptr<Entity> entity, std::shared_ptr<T> component)
{
	components[entity->id][std::type_index(typeid(T))] = component;

	changeFlag = true;

	return this;
}

template<typename T> inline std::shared_ptr<T> ECS::getComponent(std::shared_ptr<Entity> entity)
{
	auto componentID = std::type_index(typeid(T));

	auto& componentsForEntity = components[entity->id];
	if (componentsForEntity.find(componentID) != componentsForEntity.end()) {
		return std::static_pointer_cast<T>(componentsForEntity[componentID]);
	}
}

template<typename T> inline ECS* ECS::removeComponent(uint32_t entityId, std::type_index componentType)
{
	components[entityId].erase(componentType);

	changeFlag = true;

	return this;
}
