#pragma once
#include "ECS.h"

class System
{
public:
	
	virtual ~System() = default;

	virtual void update(ECS& ecs) = 0;
};

