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


	void addEntity(std::shared_ptr<Entity> entity);
	std::shared_ptr<Entity> getEntityById(uint32_t id);

	template <typename T> ECS* addComponent(std::shared_ptr<Entity> entity, std::shared_ptr<T> component);
	template <typename T> std::shared_ptr<T> getComponent(std::shared_ptr<Entity> entity);

	void onEachEntity(std::function<void(std::shared_ptr<Entity>)> callback);

	size_t getEntityCount() {
		return entities.size();
	}

private:

	std::unordered_map<uint32_t, std::shared_ptr<Entity>> entities;
	std::unordered_map<uint32_t, std::unordered_map<std::type_index, std::shared_ptr<Component>>> components;
};

template<typename T> inline ECS* ECS::addComponent(std::shared_ptr<Entity> entity, std::shared_ptr<T> component)
{
	components[entity->id][std::type_index(typeid(T))] = component;

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
