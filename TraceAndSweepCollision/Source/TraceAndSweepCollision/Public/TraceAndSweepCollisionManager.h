#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TraceAndSweepCollisionManager.generated.h"

class UTraceAndSweepCollisionComponent;

UCLASS()
class TRACEANDSWEEPCOLLISION_API ATraceAndSweepCollisionManager : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	ATraceAndSweepCollisionManager();

	void RegisterComponent(UTraceAndSweepCollisionComponent* component);
	void UnregisterComponent(UTraceAndSweepCollisionComponent* component);

protected:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:	
	TArray<UTraceAndSweepCollisionComponent*> m_components;

};

