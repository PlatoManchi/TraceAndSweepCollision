#pragma once

#include "CoreMinimal.h"
#include "TraceAndSweepCollisionTypes.generated.h"

UENUM(BlueprintType)
enum class ECollisionCompExecutionType : uint8
{
	SYNCHRONOUS = 0 UMETA(DisplayName = "Synchronous"),
	ASYNCHRONOUS UMETA(DisplayName = "Asynchronous")
};

UENUM(BlueprintType)
enum class ECollisionCompStyleType : uint8
{
	LINE = 0 UMETA(DisplayName = "Line"),
	SWEEP UMETA(DisplayName = "Sweep")
};

UENUM(BlueprintType)
enum class ECollisionCompTraceType : uint8
{
	SINGLE = 0 UMETA(DisplayName = "Single"),
	MULTI UMETA(DisplayName = "Multi")
};


UENUM(BlueprintType)
enum class ECollisionCompChannelType : uint8
{
	TRACE_CHANNEL = 0 UMETA(DisplayName = "Trace Channel"),
	OBJECT_CHANNEL UMETA(DisplayName = "Object Channel"),
	COLLISION_PRESET UMETA(DisplayName = "Collision Preset")
};

UENUM(BlueprintType)
enum class ECollisionCompShapeType : uint8
{
	BOX = 0 UMETA(DisplayName = "Box"),
	CAPSULE UMETA(DisplayName = "Capsule"),
	SPHERE UMETA(DisplayName = "Sphere")
};

USTRUCT(BlueprintType)
struct TRACEANDSWEEPCOLLISION_API FCollisionLineData
{
	GENERATED_USTRUCT_BODY()
public:
	// Name of bone or socket to track for collision.
	// NOTE: This only works if parent component is skeletal mesh component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision Line Data", meta = (DisplayName = "Socket Name"))
	FName m_socket_name = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Collision Line Data", meta = (DisplayName = "Scene Component To Follow"))
	USceneComponent* m_scene_component_to_follow = nullptr;

	FVector m_prev_location = FVector::ZeroVector;
	
	// Handle for asynchronous tracing
	FTraceHandle m_forward_trace_handle = FTraceHandle();
	FTraceHandle m_reverse_trace_handle = FTraceHandle();
};

USTRUCT(BlueprintType)
struct TRACEANDSWEEPCOLLISION_API FCollisionShapeData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Shape Data", meta = (DisplayName = "Shape Type"))
	ECollisionCompShapeType m_shape_type = ECollisionCompShapeType::BOX;

	// Box shape values
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Shape Data", meta = (DisplayName = "Box Half Extent", EditCondition = "m_shape_type == ECollisionCompShapeType::BOX", EditConditionHides))
	FVector m_box_half_extent = FVector(50, 50, 50);


	// Capsule shape values
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Shape Data", meta = (DisplayName = "Capsule Half Height", EditCondition = "m_shape_type == ECollisionCompShapeType::CAPSULE", EditConditionHides))
	float m_capsule_half_height = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Shape Data", meta = (DisplayName = "Capsule Radius", EditCondition = "m_shape_type == ECollisionCompShapeType::CAPSULE", EditConditionHides))
	float m_capsule_radius = 10;


	// Sphere values
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Shape Data", meta = (DisplayName = "Sphere Radius", EditCondition = "m_shape_type == ECollisionCompShapeType::SPHERE", EditConditionHides))
	float m_sphere_radius = 10;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Shape Data", meta = (DisplayName = "Offset"))
	FTransform m_offset = FTransform::Identity;


	FVector m_prev_location = FVector::ZeroVector;
	FQuat m_prev_rotation = FQuat::Identity;

	// Handle for asynchronous tracing
	FTraceHandle m_forward_trace_handle = FTraceHandle();
	FTraceHandle m_reverse_trace_handle = FTraceHandle();
};