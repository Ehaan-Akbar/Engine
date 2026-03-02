#pragma once
#include <cstdint>

class Entity
{
public:

	Entity();
	~Entity();

	uint32_t id;

private:

	static uint32_t nextId;

};

