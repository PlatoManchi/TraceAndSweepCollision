#include "TraceAndSweepCollisionManager.h"
#include "TraceAndSweepCollisionComponent.h"

// Sets default values
ATraceAndSweepCollisionManager::ATraceAndSweepCollisionManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called every frame
void ATraceAndSweepCollisionManager::Tick(float delta_time)
{
	Super::Tick(delta_time);

	for (UTraceAndSweepCollisionComponent* comp : m_components)
	{
		if (comp->IsTraceCollisionEnabled())
		{
			comp->m_time_elapsed += delta_time;
			if (comp->m_time_elapsed >= comp->m_tick_interval)
			{
				comp->m_time_elapsed = 0.0f;
				comp->ExternalTick();
			}
		}
	}
}


void ATraceAndSweepCollisionManager::RegisterComponent(UTraceAndSweepCollisionComponent* component)
{
	m_components.Add(component);
}

void ATraceAndSweepCollisionManager::UnregisterComponent(UTraceAndSweepCollisionComponent* component)
{
	m_components.RemoveSwap(component, false);
}

