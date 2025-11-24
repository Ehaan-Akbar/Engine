#include "Entity.h"


uint32_t Entity::nextId = 1;

Entity::Entity()
{
	id = nextId++;
}

Entity::~Entity()
{
}
