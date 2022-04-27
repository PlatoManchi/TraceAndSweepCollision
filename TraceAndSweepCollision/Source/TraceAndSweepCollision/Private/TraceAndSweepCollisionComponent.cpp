#include "TraceAndSweepCollisionComponent.h"
#include "TraceAndSweepDebugDraw.h"
#include "TraceAndSweepCollisionManager.h"

#include "EngineUtils.h"


//For Unreal Profiler 
DECLARE_CYCLE_STAT(TEXT("DoCollisionTest"), STAT_DoCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);
DECLARE_CYCLE_STAT(TEXT("DoSynchronousLineSingleCollisionTest"), STAT_DoSynchronousLineSingleCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);
DECLARE_CYCLE_STAT(TEXT("DoSynchronousSweepSingleCollisionTest"), STAT_DoSynchronousSweepSingleCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);
DECLARE_CYCLE_STAT(TEXT("DoSynchronousLineMultiCollisionTest"), STAT_DoSynchronousLineMultiCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);
DECLARE_CYCLE_STAT(TEXT("DoSynchronousSweepMultiCollisionTest"), STAT_DoSynchronousSweepMultiCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);
DECLARE_CYCLE_STAT(TEXT("DoAsynchronousLineCollisionTest"), STAT_DoAsynchronousLineCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);
DECLARE_CYCLE_STAT(TEXT("DoAsynchronousSweepCollisionTest"), STAT_DoAsynchronousSweepCollisionTest, STATGROUP_TraceAndSweepCollisionComponent);

DECLARE_CYCLE_STAT(TEXT("GetDynamicMeshElements"), STAT_TraceAndSweepCollisionSceneProxy_GetDynamicMeshElements, STATGROUP_TraceAndSweepCollisionComponent);


UTraceAndSweepCollisionComponent::FHitResultWrapper::FHitResultWrapper(const FHitResult& result) :
	m_result(result)
{}

bool UTraceAndSweepCollisionComponent::FHitResultWrapper::operator==(const FHitResultWrapper& rhs) const
{
	return m_result.GetActor() == rhs.m_result.GetActor()
		&& m_result.GetComponent() == rhs.m_result.GetComponent()
		&& m_result.Item == rhs.m_result.Item;
}


UTraceAndSweepCollisionComponent::UTraceAndSweepCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bHiddenInGame = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetMobility(EComponentMobility::Movable);
}


// Called when the game starts
void UTraceAndSweepCollisionComponent::BeginPlay()
{
	Super::BeginPlay();

	m_async_trace_delegate.BindUObject(this, &UTraceAndSweepCollisionComponent::OnAsyncTraceComplete);

	// Add all child scene objects for tracing
	TArray<USceneComponent*> childern;
	this->GetChildrenComponents(true, childern);
	this->AddSceneComponentsToTrace(childern);


	SetIsTraceCollisionEnabled(m_start_with_collision_enabled);
	SetTracePerSecond(m_traces_per_second);

	TActorIterator<ATraceAndSweepCollisionManager> ActorItr(GetWorld());
	if (ActorItr)
	{
		(*ActorItr)->RegisterComponent(this);
	}
}

void UTraceAndSweepCollisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TActorIterator<ATraceAndSweepCollisionManager> ActorItr(GetWorld());
	if (ActorItr)
	{
		(*ActorItr)->UnregisterComponent(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UTraceAndSweepCollisionComponent::ExternalTick()
{
	if (m_is_trace_collision_enabled && m_is_previous_trace_complete)
	{
		DoCollisionTest();
	}
}

void UTraceAndSweepCollisionComponent::TickComponent(float delta_time, ELevelTick tick_type, FActorComponentTickFunction* this_tick_function)
{
	Super::TickComponent(delta_time, tick_type, this_tick_function);

	if (m_is_trace_collision_enabled && m_is_previous_trace_complete)
	{
		DoCollisionTest();
	}
}

bool UTraceAndSweepCollisionComponent::DoCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoCollisionTest);
	
	// Clear out cached results from previous collision test
	m_forward_hit_results.Empty();
	m_reverse_hit_results.Empty();
	
	if (m_execution_type == ECollisionCompExecutionType::SYNCHRONOUS && m_style_type == ECollisionCompStyleType::LINE && m_trace_type == ECollisionCompTraceType::SINGLE)
	{
		return DoSynchronousLineSingleCollisionTest();
	}
	else if (m_execution_type == ECollisionCompExecutionType::SYNCHRONOUS && m_style_type == ECollisionCompStyleType::SWEEP && m_trace_type == ECollisionCompTraceType::SINGLE)
	{
		return DoSynchronousSweepSingleCollisionTest();
	}
	else if (m_execution_type == ECollisionCompExecutionType::SYNCHRONOUS && m_style_type == ECollisionCompStyleType::LINE && m_trace_type == ECollisionCompTraceType::MULTI)
	{
		return DoSynchronousLineMultiCollisionTest();
	}
	else if (m_execution_type == ECollisionCompExecutionType::SYNCHRONOUS && m_style_type == ECollisionCompStyleType::SWEEP && m_trace_type == ECollisionCompTraceType::MULTI)
	{
		return DoSynchronousSweepMultiCollisionTest();
	}
	else if (m_execution_type == ECollisionCompExecutionType::ASYNCHRONOUS && m_style_type == ECollisionCompStyleType::LINE)
	{
		return DoAsynchronousLineCollisionTest();
	}
	else if (m_execution_type == ECollisionCompExecutionType::ASYNCHRONOUS && m_style_type == ECollisionCompStyleType::SWEEP)
	{
		return DoAsynchronousSweepCollisionTest();
	}

	return false;
}

#pragma region Synchronous
bool UTraceAndSweepCollisionComponent::DoSynchronousLineSingleCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoSynchronousLineSingleCollisionTest);
	
	UWorld* world = GetWorld();
	if (!world) return false;

	m_is_previous_trace_complete = false;

	FCollisionQueryParams params;
	params.bTraceComplex = m_trace_complex;
	params.bReturnFaceIndex = m_return_face_index;
	params.bReturnPhysicalMaterial = m_return_physical_material;
	params.bIgnoreBlocks = m_ignore_blocks;
	params.bIgnoreTouches = m_ignore_touches;
	params.bSkipNarrowPhase = m_skip_narrow_phase;

	params.AddIgnoredActor(GetOwner());

	const USkeletalMeshComponent* parent_skeletal_mesh = Cast<USkeletalMeshComponent>(GetAttachParent());

	for (FCollisionLineData& line_data : m_collision_line_data)
	{
		bool valid_line_data = false;
		FVector start = line_data.m_prev_location;
		FVector end = start;

		if (line_data.m_scene_component_to_follow->IsValidLowLevel())
		{
			end = line_data.m_scene_component_to_follow->GetComponentLocation();
			valid_line_data = true;
		}
		else if (parent_skeletal_mesh && line_data.m_socket_name != NAME_None && parent_skeletal_mesh->DoesSocketExist(line_data.m_socket_name))
		{
			end = parent_skeletal_mesh->GetSocketLocation(line_data.m_socket_name);
			valid_line_data = true;
		}

		if (valid_line_data)
		{
			// check for forward hits, these results will be used for begin overlap check
			FHitResult forward_hit;
			DoLineSingle(forward_hit, world, start, end, params);
			if (forward_hit.bBlockingHit)
			{
				m_forward_hit_results.AddUnique(forward_hit);
			}

			if (m_should_generate_end_overlap)
			{
				// check for reverse hits, these results will be used for end overlap check
				TArray<FHitResult> reverse_hits;
				DoLineMulti(reverse_hits, world, end, start, params, true);

				for (FHitResult& reverse_hit : reverse_hits)
				{
					if (reverse_hit.bBlockingHit)
					{
						m_reverse_hit_results.AddUnique(reverse_hit);
					}
				}
			}

			line_data.m_prev_location = end;

#if !UE_BUILD_SHIPPING
			// Draw debug trace line
			TraceAndSweepDebugDraw::DrawLineTraces(world
				, start
				, end
				, { forward_hit }
				, m_debug_draw_trace_lines
				, m_debug_line_color
				, m_debug_line_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
#endif
		}
	}

	// All hit results are ready to be processed
	ProcessForwardHitResults();

	if (m_should_generate_end_overlap)
	{
		ProcessReverseHitResults();
	}

	m_is_previous_trace_complete = true;

	return true;
}

bool UTraceAndSweepCollisionComponent::DoSynchronousSweepSingleCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoSynchronousSweepSingleCollisionTest);
	
	UWorld* world = GetWorld();
	if (!world) return false;

	m_is_previous_trace_complete = false;

	FCollisionQueryParams params;
	params.bTraceComplex = m_trace_complex;
	params.bReturnFaceIndex = m_return_face_index;
	params.bReturnPhysicalMaterial = m_return_physical_material;
	params.bIgnoreBlocks = m_ignore_blocks;
	params.bIgnoreTouches = m_ignore_touches;
	params.bSkipNarrowPhase = m_skip_narrow_phase;

	params.AddIgnoredActor(GetOwner());

	FTransform current_comp_transform = GetComponentTransform();

	for (FCollisionShapeData& shape_data : m_collision_shape_data)
	{
		FVector start = shape_data.m_prev_location;
		FTransform end_transform = shape_data.m_offset * current_comp_transform;
		FVector end_location = end_transform.GetLocation();
		FQuat end_rotation = end_transform.GetRotation();

		// Forward sweep
		FHitResult forward_hit;
		DoSweepSingle(forward_hit, world, start, end_location, end_rotation, shape_data, params);
		if (forward_hit.bBlockingHit)
		{
			m_forward_hit_results.AddUnique(forward_hit);
		}

		// Reverse Sweep
		if (m_should_generate_end_overlap)
		{
			// check for reverse hits, these results will be used for end overlap check
			TArray<FHitResult> reverse_hits;
			DoSweepMulti(reverse_hits, world, end_location, start, shape_data.m_prev_rotation, shape_data, params, true);
			for (FHitResult& reverse_hit : reverse_hits)
			{
				if (reverse_hit.bBlockingHit)
				{
					m_reverse_hit_results.AddUnique(reverse_hit);
				}
			}
		}

		shape_data.m_prev_location = end_location;
		shape_data.m_prev_rotation = end_rotation;

#if !UE_BUILD_SHIPPING
		if (shape_data.m_shape_type == ECollisionCompShapeType::BOX)
		{
			TraceAndSweepDebugDraw::DrawBoxSweep(world
				, start
				, end_location
				, shape_data.m_box_half_extent
				, end_rotation
				, { forward_hit }
				, m_debug_draw_sweep_shape
				, m_debug_sweep_shape_color
				, m_debug_sweep_shape_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
		else if (shape_data.m_shape_type == ECollisionCompShapeType::CAPSULE)
		{
			TraceAndSweepDebugDraw::DrawCapsuleSweep(world
				, start
				, end_location
				, shape_data.m_capsule_half_height
				, shape_data.m_capsule_radius
				, end_rotation
				, { forward_hit }
				, m_debug_draw_sweep_shape
				, m_debug_sweep_shape_color
				, m_debug_sweep_shape_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
		else if (shape_data.m_shape_type == ECollisionCompShapeType::SPHERE)
		{
			TraceAndSweepDebugDraw::DrawSphereSweep(world
				, start
				, end_location
				, shape_data.m_sphere_radius
				, { forward_hit }
				, m_debug_draw_sweep_shape
				, m_debug_sweep_shape_color
				, m_debug_sweep_shape_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
#endif
	}
	
	// All hit results are ready to be processed
	ProcessForwardHitResults();

	if (m_should_generate_end_overlap)
	{
		ProcessReverseHitResults();
	}

	m_is_previous_trace_complete = true;

	return true;
}

bool UTraceAndSweepCollisionComponent::DoSynchronousLineMultiCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoSynchronousLineMultiCollisionTest);

	UWorld* world = GetWorld();
	if (!world) return false;

	m_is_previous_trace_complete = false;

	FCollisionQueryParams params;
	params.bTraceComplex = m_trace_complex;
	params.bReturnFaceIndex = m_return_face_index;
	params.bReturnPhysicalMaterial = m_return_physical_material;
	params.bIgnoreBlocks = m_ignore_blocks;
	params.bIgnoreTouches = m_ignore_touches;
	params.bSkipNarrowPhase = m_skip_narrow_phase;

	params.AddIgnoredActor(GetOwner());

	const USkeletalMeshComponent* parent_skeletal_mesh = Cast<USkeletalMeshComponent>(GetAttachParent());

	for (FCollisionLineData& line_data : m_collision_line_data)
	{
		bool valid_line_data = false;
		FVector start = line_data.m_prev_location;
		FVector end = start;

		if (line_data.m_scene_component_to_follow->IsValidLowLevel())
		{
			end = line_data.m_scene_component_to_follow->GetComponentLocation();
			valid_line_data = true;
		}
		else if (parent_skeletal_mesh && line_data.m_socket_name != NAME_None && parent_skeletal_mesh->DoesSocketExist(line_data.m_socket_name))
		{
			end = parent_skeletal_mesh->GetSocketLocation(line_data.m_socket_name);
			valid_line_data = true;
		}

		if (valid_line_data)
		{
			// check for forward hits, these results will be used for begin overlap check
			TArray<FHitResult> forward_hits;
			DoLineMulti(forward_hits, world, start, end, params);
			for (FHitResult& forward_hit : forward_hits)
			{
				if (forward_hit.bBlockingHit)
				{
					m_forward_hit_results.AddUnique(forward_hit);
				}
			}
			
			if (m_should_generate_end_overlap)
			{
				// check for reverse hits, these results will be used for end overlap check
				TArray<FHitResult> reverse_hits;
				DoLineMulti(reverse_hits, world, end, start, params, true);
				for (FHitResult& reverse_hit : reverse_hits)
				{
					if (reverse_hit.bBlockingHit)
					{
						m_reverse_hit_results.AddUnique(reverse_hit);
					}
				}
			}

			line_data.m_prev_location = end;

#if !UE_BUILD_SHIPPING
			// Draw debug trace line
			TraceAndSweepDebugDraw::DrawLineTraces(world
				, start
				, end
				, forward_hits
				, m_debug_draw_trace_lines
				, m_debug_line_color
				, m_debug_line_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
#endif
		}
	}

	// All hit results are ready to be processed
	ProcessForwardHitResults();

	if (m_should_generate_end_overlap)
	{
		ProcessReverseHitResults();
	}

	m_is_previous_trace_complete = true;

	return true;
}

bool UTraceAndSweepCollisionComponent::DoSynchronousSweepMultiCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoSynchronousSweepMultiCollisionTest);

	UWorld* world = GetWorld();
	if (!world) return false;

	m_is_previous_trace_complete = false;

	FCollisionQueryParams params;
	params.bTraceComplex = m_trace_complex;
	params.bReturnFaceIndex = m_return_face_index;
	params.bReturnPhysicalMaterial = m_return_physical_material;
	params.bIgnoreBlocks = m_ignore_blocks;
	params.bIgnoreTouches = m_ignore_touches;
	params.bSkipNarrowPhase = m_skip_narrow_phase;

	params.AddIgnoredActor(GetOwner());

	FTransform current_comp_transform = GetComponentTransform();

	for (FCollisionShapeData& shape_data : m_collision_shape_data)
	{
		FVector start = shape_data.m_prev_location;
		FTransform end_transform = shape_data.m_offset * current_comp_transform;
		FVector end_location = end_transform.GetLocation();
		FQuat end_rotation = end_transform.GetRotation();

		// Forward sweep
		TArray<FHitResult> forward_hits;
		DoSweepMulti(forward_hits, world, start, end_location, end_rotation, shape_data, params);
		for (FHitResult& forward_hit : forward_hits)
		{
			if (forward_hit.bBlockingHit)
			{
				m_forward_hit_results.AddUnique(forward_hit);
			}
		}
		
		// Reverse Sweep
		if (m_should_generate_end_overlap)
		{
			// check for reverse hits, these results will be used for end overlap check
			TArray<FHitResult> reverse_hits;
			DoSweepMulti(reverse_hits, world, end_location, start, shape_data.m_prev_rotation, shape_data, params, true);
			for (FHitResult& reverse_hit : reverse_hits)
			{
				if (reverse_hit.bBlockingHit)
				{
					m_reverse_hit_results.AddUnique(reverse_hit);
				}
			}
		}

		shape_data.m_prev_location = end_location;
		shape_data.m_prev_rotation = end_rotation;

#if !UE_BUILD_SHIPPING
		if (shape_data.m_shape_type == ECollisionCompShapeType::BOX)
		{
			TraceAndSweepDebugDraw::DrawBoxSweep(world
				, start
				, end_location
				, shape_data.m_box_half_extent
				, end_rotation
				, forward_hits
				, m_debug_draw_sweep_shape
				, m_debug_sweep_shape_color
				, m_debug_sweep_shape_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
		else if (shape_data.m_shape_type == ECollisionCompShapeType::CAPSULE)
		{
			TraceAndSweepDebugDraw::DrawCapsuleSweep(world
				, start
				, end_location
				, shape_data.m_capsule_half_height
				, shape_data.m_capsule_radius
				, end_rotation
				, forward_hits
				, m_debug_draw_sweep_shape
				, m_debug_sweep_shape_color
				, m_debug_sweep_shape_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
		else if (shape_data.m_shape_type == ECollisionCompShapeType::SPHERE)
		{
			TraceAndSweepDebugDraw::DrawSphereSweep(world
				, start
				, end_location
				, shape_data.m_sphere_radius
				, forward_hits
				, m_debug_draw_sweep_shape
				, m_debug_sweep_shape_color
				, m_debug_sweep_shape_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
#endif
	}

	// All hit results are ready to be processed
	ProcessForwardHitResults();

	if (m_should_generate_end_overlap)
	{
		ProcessReverseHitResults();
	}

	m_is_previous_trace_complete = true;

	return true;
}
#pragma endregion

bool UTraceAndSweepCollisionComponent::DoAsynchronousLineCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoAsynchronousLineCollisionTest);

	UWorld* world = GetWorld();
	if (!world) return false;

	m_is_previous_trace_complete = false;

	FCollisionQueryParams params;
	params.bTraceComplex = m_trace_complex;
	params.bReturnFaceIndex = m_return_face_index;
	params.bReturnPhysicalMaterial = m_return_physical_material;
	params.bIgnoreBlocks = m_ignore_blocks;
	params.bIgnoreTouches = m_ignore_touches;
	params.bSkipNarrowPhase = m_skip_narrow_phase;

	params.AddIgnoredActor(GetOwner());

	const USkeletalMeshComponent* parent_skeletal_mesh = Cast<USkeletalMeshComponent>(GetAttachParent());
	EAsyncTraceType trace_type = m_trace_type == ECollisionCompTraceType::SINGLE ? EAsyncTraceType::Single : EAsyncTraceType::Multi;

	for (FCollisionLineData& line_data : m_collision_line_data)
	{
		bool valid_line_data = false;
		FVector start = line_data.m_prev_location;
		FVector end = start;

		if (line_data.m_scene_component_to_follow->IsValidLowLevel())
		{
			end = line_data.m_scene_component_to_follow->GetComponentLocation();
			valid_line_data = true;
		}
		else if (parent_skeletal_mesh && line_data.m_socket_name != NAME_None && parent_skeletal_mesh->DoesSocketExist(line_data.m_socket_name))
		{
			end = parent_skeletal_mesh->GetSocketLocation(line_data.m_socket_name);
			valid_line_data = true;
		}

		if (valid_line_data)
		{
			m_is_previous_trace_complete = false;

			line_data.m_forward_trace_handle = DoAsyncLine(world, trace_type, start, end, params);
			line_data.m_reverse_trace_handle = DoAsyncLine(world, EAsyncTraceType::Multi, end, start, params, true);

			line_data.m_prev_location = end;
		}
	}
	
	return true;
}

bool UTraceAndSweepCollisionComponent::DoAsynchronousSweepCollisionTest()
{
	SCOPE_CYCLE_COUNTER(STAT_DoAsynchronousSweepCollisionTest);
	
	UWorld* world = GetWorld();
	if (!world) return false;

	m_is_previous_trace_complete = false;

	FCollisionQueryParams params;
	params.bTraceComplex = m_trace_complex;
	params.bReturnFaceIndex = m_return_face_index;
	params.bReturnPhysicalMaterial = m_return_physical_material;
	params.bIgnoreBlocks = m_ignore_blocks;
	params.bIgnoreTouches = m_ignore_touches;
	params.bSkipNarrowPhase = m_skip_narrow_phase;

	params.AddIgnoredActor(GetOwner());

	FTransform current_comp_transform = GetComponentTransform();
	EAsyncTraceType trace_type = m_trace_type == ECollisionCompTraceType::SINGLE ? EAsyncTraceType::Single : EAsyncTraceType::Multi;

	for (FCollisionShapeData& shape_data : m_collision_shape_data)
	{
		FVector start = shape_data.m_prev_location;
		FTransform end_transform = shape_data.m_offset * current_comp_transform;
		FVector end_location = end_transform.GetLocation();
		FQuat end_rotation = end_transform.GetRotation();

		shape_data.m_forward_trace_handle = DoAsyncSweep(world, trace_type, start, end_location, end_rotation, shape_data, params);
		shape_data.m_reverse_trace_handle = DoAsyncSweep(world, EAsyncTraceType::Multi, end_location, start, shape_data.m_prev_rotation, shape_data, params, true);

		shape_data.m_prev_location = end_location;
		shape_data.m_prev_rotation = end_rotation;
	}

	return true;
}


bool UTraceAndSweepCollisionComponent::DoLineSingle(FHitResult& out_result, UWorld* world, const FVector& start, const FVector& end, const FCollisionQueryParams& params) const
{
	check(world != nullptr);

	if (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL)
	{
		world->LineTraceSingleByChannel(out_result, start, end, m_trace_channel, params);
		return true;
	}
	else if (m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL)
	{
		FCollisionObjectQueryParams object_params(m_object_channels);
		world->LineTraceSingleByObjectType(out_result, start, end, object_params, params);
		return true;
	}
	else if (m_channel_type == ECollisionCompChannelType::COLLISION_PRESET)
	{
		world->LineTraceSingleByProfile(out_result, start, end, m_collision_preset.Name, params);
		return true;
	}

	return false;
}

bool UTraceAndSweepCollisionComponent::DoLineMulti(TArray<FHitResult>& out_results, UWorld* world, const FVector& start, const FVector& end, const FCollisionQueryParams& params, bool is_reverse /* = false*/) const
{
	check(world != nullptr);

	const bool is_special_case = is_reverse && (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL || m_channel_type == ECollisionCompChannelType::COLLISION_PRESET);

	if (!is_special_case)
	{
		if (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL)
		{
			world->LineTraceMultiByChannel(out_results, start, end, m_trace_channel, params);
			return true;
		}
		else if (m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL)
		{
			FCollisionObjectQueryParams object_params(m_object_channels);
			world->LineTraceMultiByObjectType(out_results, start, end, object_params, params);
			return true;
		}
		else if (m_channel_type == ECollisionCompChannelType::COLLISION_PRESET)
		{
			world->LineTraceMultiByProfile(out_results, start, end, m_collision_preset.Name, params);
			return true;
		}
	}
	else
	{
		world->LineTraceMultiByObjectType(out_results, start, end, FCollisionObjectQueryParams::AllObjects, params);
		return true;
	}

	return false;
}

bool UTraceAndSweepCollisionComponent::DoSweepSingle(FHitResult& out_result, UWorld* world, const FVector& start, const FVector& end, const FQuat rotation, const FCollisionShapeData& shape_data, const FCollisionQueryParams& params) const
{
	check(world != nullptr);

	// Create shape
	FCollisionShape shape;
	if (shape_data.m_shape_type == ECollisionCompShapeType::BOX)
	{
		shape = FCollisionShape::MakeBox(shape_data.m_box_half_extent);
	}
	else if (shape_data.m_shape_type == ECollisionCompShapeType::CAPSULE)
	{
		shape = FCollisionShape::MakeCapsule(shape_data.m_capsule_radius, shape_data.m_capsule_half_height);
	}
	else if (shape_data.m_shape_type == ECollisionCompShapeType::SPHERE)
	{
		shape = FCollisionShape::MakeSphere(shape_data.m_sphere_radius);
	}

	if (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL)
	{
		world->SweepSingleByChannel(out_result, start, end, rotation, m_trace_channel, shape, params);
		return true;
	}
	else if (m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL)
	{
		FCollisionObjectQueryParams object_params(m_object_channels);
		world->SweepSingleByObjectType(out_result, start, end, rotation, object_params, shape, params);
		return true;
	}
	else if (m_channel_type == ECollisionCompChannelType::COLLISION_PRESET)
	{
		world->SweepSingleByProfile(out_result, start, end, rotation, m_collision_preset.Name, shape, params);
		return true;
	}

	return false;
}

bool UTraceAndSweepCollisionComponent::DoSweepMulti(TArray<FHitResult>& out_results, UWorld* world, const FVector& start, const FVector& end, const FQuat rotation, const FCollisionShapeData& shape_data, const FCollisionQueryParams& params, bool is_reverse /* = false*/) const
{
	check(world != nullptr);

	// Create shape
	FCollisionShape shape;
	if (shape_data.m_shape_type == ECollisionCompShapeType::BOX)
	{
		shape = FCollisionShape::MakeBox(shape_data.m_box_half_extent);
	}
	else if (shape_data.m_shape_type == ECollisionCompShapeType::CAPSULE)
	{
		shape = FCollisionShape::MakeCapsule(shape_data.m_capsule_radius, shape_data.m_capsule_half_height);
	}
	else if (shape_data.m_shape_type == ECollisionCompShapeType::SPHERE)
	{
		shape = FCollisionShape::MakeSphere(shape_data.m_sphere_radius);
	}

	// do trace
	const bool is_special_case = is_reverse && (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL || m_channel_type == ECollisionCompChannelType::COLLISION_PRESET);
	if (!is_special_case)
	{
		if (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL)
		{
			world->SweepMultiByChannel(out_results, start, end, rotation, m_trace_channel, shape, params);
			return true;
		}
		else if (m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL)
		{
			FCollisionObjectQueryParams object_params(m_object_channels);
			world->SweepMultiByObjectType(out_results, start, end, rotation, object_params, shape, params);
			return true;
		}
		else if (m_channel_type == ECollisionCompChannelType::COLLISION_PRESET)
		{
			world->SweepMultiByProfile(out_results, start, end, rotation, m_collision_preset.Name, shape, params);
			return true;
		}
	}
	else
	{
		world->SweepMultiByObjectType(out_results, start, end, rotation, FCollisionObjectQueryParams::AllObjects, shape, params);
		return true;
	}

	return false;
}

FTraceHandle UTraceAndSweepCollisionComponent::DoAsyncLine(UWorld* world, const EAsyncTraceType trace_type, const FVector& start, const FVector& end, const FCollisionQueryParams& params, bool is_reverse /* = false*/)
{
	const bool is_special_case = is_reverse && trace_type == EAsyncTraceType::Multi &&(m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL || m_channel_type == ECollisionCompChannelType::COLLISION_PRESET);

	if (!is_special_case)
	{
		if (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL)
		{
			return world->AsyncLineTraceByChannel(trace_type, start, end, m_trace_channel, params, FCollisionResponseParams::DefaultResponseParam, &m_async_trace_delegate);
		}
		else if (m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL)
		{
			FCollisionObjectQueryParams object_params(m_object_channels);
			return world->AsyncLineTraceByObjectType(trace_type, start, end, object_params, params, &m_async_trace_delegate);
		}
		else if (m_channel_type == ECollisionCompChannelType::COLLISION_PRESET)
		{
			return world->AsyncLineTraceByProfile(trace_type, start, end, m_collision_preset.Name, params, &m_async_trace_delegate);
		}
	}
	else
	{
		return world->AsyncLineTraceByObjectType(trace_type, start, end, FCollisionObjectQueryParams::AllObjects, params, &m_async_trace_delegate);
	}

	return FTraceHandle();
}

FTraceHandle UTraceAndSweepCollisionComponent::DoAsyncSweep(UWorld* world, const EAsyncTraceType trace_type, const FVector& start, const FVector& end, const FQuat rotation, const FCollisionShapeData& shape_data, const FCollisionQueryParams& params, bool is_reverse /* = false*/)
{
	// Create shape
	FCollisionShape shape;
	if (shape_data.m_shape_type == ECollisionCompShapeType::BOX)
	{
		shape = FCollisionShape::MakeBox(shape_data.m_box_half_extent);
	}
	else if (shape_data.m_shape_type == ECollisionCompShapeType::CAPSULE)
	{
		shape = FCollisionShape::MakeCapsule(shape_data.m_capsule_radius, shape_data.m_capsule_half_height);
	}
	else if (shape_data.m_shape_type == ECollisionCompShapeType::SPHERE)
	{
		shape = FCollisionShape::MakeSphere(shape_data.m_sphere_radius);
	}

	// do trace
	const bool is_special_case = is_reverse && trace_type == EAsyncTraceType::Multi && (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL || m_channel_type == ECollisionCompChannelType::COLLISION_PRESET);
	if (!is_special_case)
	{
		if (m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL)
		{
			return world->AsyncSweepByChannel(trace_type, start, end, rotation, m_trace_channel, shape, params, FCollisionResponseParams::DefaultResponseParam, &m_async_trace_delegate);
		}
		else if (m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL)
		{
			FCollisionObjectQueryParams object_params(m_object_channels);
			return world->AsyncSweepByObjectType(trace_type, start, end, rotation, object_params, shape, params, &m_async_trace_delegate);
		}
		else if (m_channel_type == ECollisionCompChannelType::COLLISION_PRESET)
		{
			return world->AsyncSweepByProfile(trace_type, start, end, rotation, m_collision_preset.Name, shape, params, &m_async_trace_delegate);
		}
	}
	else
	{
		return world->AsyncSweepByObjectType(trace_type, start, end, rotation, FCollisionObjectQueryParams::AllObjects, shape, params, &m_async_trace_delegate);
	}

	return FTraceHandle();
}

void UTraceAndSweepCollisionComponent::OnAsyncTraceComplete(const FTraceHandle& handle, FTraceDatum& data)
{
	FCollisionLineData* line_data_ptr = nullptr;
	FCollisionShapeData* shape_data_ptr = nullptr;

	if (m_style_type == ECollisionCompStyleType::LINE)
	{
		line_data_ptr = m_collision_line_data.FindByPredicate(
			[&](const FCollisionLineData& other)
			{
				return other.m_forward_trace_handle.IsValid() && other.m_forward_trace_handle == handle;
			});
	}
	else if (m_style_type == ECollisionCompStyleType::SWEEP)
	{
		shape_data_ptr = m_collision_shape_data.FindByPredicate(
			[&](const FCollisionShapeData& other)
			{
				return other.m_forward_trace_handle.IsValid() && other.m_forward_trace_handle == handle;
			});
	}

	if (line_data_ptr || shape_data_ptr)
	{
		// Forward trace
		// Clear the handle
		if (line_data_ptr)
			line_data_ptr->m_forward_trace_handle = FTraceHandle();
		else if (shape_data_ptr)
			shape_data_ptr->m_forward_trace_handle = FTraceHandle();

		for (const FHitResult& forward_hit : data.OutHits)
		{
			if (forward_hit.bBlockingHit)
			{
				m_forward_hit_results.AddUnique(forward_hit);
			}
		}

#if !UE_BUILD_SHIPPING
		if (line_data_ptr)
		{
			// Draw debug trace line
			TraceAndSweepDebugDraw::DrawLineTraces(GetWorld()
				, data.Start
				, data.End
				, data.OutHits
				, m_debug_draw_trace_lines
				, m_debug_line_color
				, m_debug_line_duration
				, m_draw_debug_hit_point
				, m_debug_hit_normal_color
				, m_debug_touch_normal_color
				, m_debug_hit_marker_duration
				, m_debug_line_thickness);
		}
		else if (shape_data_ptr)
		{
			if (shape_data_ptr->m_shape_type == ECollisionCompShapeType::BOX)
			{
				TraceAndSweepDebugDraw::DrawBoxSweep(GetWorld()
					, data.Start
					, data.End
					, shape_data_ptr->m_box_half_extent
					, data.Rot
					, data.OutHits
					, m_debug_draw_sweep_shape
					, m_debug_sweep_shape_color
					, m_debug_sweep_shape_duration
					, m_draw_debug_hit_point
					, m_debug_hit_normal_color
					, m_debug_touch_normal_color
					, m_debug_hit_marker_duration
					, m_debug_line_thickness);
			}
			else if (shape_data_ptr->m_shape_type == ECollisionCompShapeType::CAPSULE)
			{
				TraceAndSweepDebugDraw::DrawCapsuleSweep(GetWorld()
					, data.Start
					, data.End
					, shape_data_ptr->m_capsule_half_height
					, shape_data_ptr->m_capsule_radius
					, data.Rot
					, data.OutHits
					, m_debug_draw_sweep_shape
					, m_debug_sweep_shape_color
					, m_debug_sweep_shape_duration
					, m_draw_debug_hit_point
					, m_debug_hit_normal_color
					, m_debug_touch_normal_color
					, m_debug_hit_marker_duration
					, m_debug_line_thickness);
			}
			else if (shape_data_ptr->m_shape_type == ECollisionCompShapeType::SPHERE)
			{
				TraceAndSweepDebugDraw::DrawSphereSweep(GetWorld()
					, data.Start
					, data.End
					, shape_data_ptr->m_sphere_radius
					, data.OutHits
					, m_debug_draw_sweep_shape
					, m_debug_sweep_shape_color
					, m_debug_sweep_shape_duration
					, m_draw_debug_hit_point
					, m_debug_hit_normal_color
					, m_debug_touch_normal_color
					, m_debug_hit_marker_duration
					, m_debug_line_thickness);
			}
		}
#endif
	}

	if (m_style_type == ECollisionCompStyleType::LINE)
	{
		line_data_ptr = m_collision_line_data.FindByPredicate(
			[&](const FCollisionLineData& other)
			{
				return other.m_reverse_trace_handle.IsValid() && other.m_reverse_trace_handle == handle;
			});
	}
	else if (m_style_type == ECollisionCompStyleType::SWEEP)
	{
		shape_data_ptr = m_collision_shape_data.FindByPredicate(
			[&](const FCollisionShapeData& other)
			{
				return other.m_reverse_trace_handle.IsValid() && other.m_reverse_trace_handle == handle;
			});
	}

	if (line_data_ptr || shape_data_ptr)
	{
		// Reverse trace
		// Clear the handle
		if (line_data_ptr)
			line_data_ptr->m_reverse_trace_handle = FTraceHandle();
		else if (shape_data_ptr)
			shape_data_ptr->m_reverse_trace_handle = FTraceHandle();

		for (const FHitResult& reverse_hit : data.OutHits)
		{
			if (reverse_hit.bBlockingHit)
			{
				m_reverse_hit_results.AddUnique(reverse_hit);
			}
		}
	}

	// check if all the async traces are finished
	bool is_all_traces_finished = true;
	if (m_style_type == ECollisionCompStyleType::LINE)
	{
		for (const FCollisionLineData& line_data : m_collision_line_data)
		{
			if (line_data.m_forward_trace_handle.IsValid() || line_data.m_reverse_trace_handle.IsValid())
			{
				is_all_traces_finished = false;
				break;
			}
		}
	}
	else if (m_style_type == ECollisionCompStyleType::SWEEP)
	{
		for (const FCollisionShapeData& shape_data : m_collision_shape_data)
		{
			if (shape_data.m_forward_trace_handle.IsValid() || shape_data.m_reverse_trace_handle.IsValid())
			{
				is_all_traces_finished = false;
				break;
			}
		}
	}

	// if all traces are completed then process the hit results.
	if (is_all_traces_finished)
	{
		// All hit results are ready to be processed
		ProcessForwardHitResults();

		if (m_should_generate_end_overlap)
		{
			ProcessReverseHitResults();
		}
	}

	m_is_previous_trace_complete = is_all_traces_finished;
}


void UTraceAndSweepCollisionComponent::ProcessForwardHitResults()
{
	for (const FHitResultWrapper& result : m_forward_hit_results)
	{
		if (result.m_result.Distance != 0.0f)
		{
			int index = -1;
			bool is_contain_result = m_overlapped_results.Find(result, index);
			if (!is_contain_result)
			{
				index = m_overlapped_results.Add(result);

				// broadcast begin overlapevent
				if (OnComponentBeginOverlap.IsBound())
				{
					OnComponentBeginOverlap.Broadcast(this, result.m_result.GetActor(), result.m_result.GetComponent(), result.m_result.Item, true, result.m_result);
				}
				if (result.m_result.GetComponent()->GetGenerateOverlapEvents())
				{
					result.m_result.GetComponent()->OnComponentBeginOverlap.Broadcast(result.m_result.GetComponent(), Cast<AActor>(this->GetOwner()), this, -1, true, result.m_result);
				}
			}
			if (index != -1)
			{
				m_overlapped_results[index].m_count++;
			}
		}
	}

	if (!m_should_generate_end_overlap)
	{
		m_overlapped_results.Empty();
	}
}

void UTraceAndSweepCollisionComponent::ProcessReverseHitResults()
{
	for (const FHitResultWrapper& result : m_reverse_hit_results)
	{
		if (result.m_result.Distance != 0.0f)
		{
			int32 index = -1;
			bool is_contain_result = m_overlapped_results.Find(result, index);
			if (is_contain_result)
			{
				m_overlapped_results[index].m_count--;
				if (m_overlapped_results[index].m_count == 0)
				{
					m_overlapped_results.RemoveAtSwap(index, 1, false);

					// broadcast end overlapevent
					if (OnComponentEndOverlap.IsBound())
					{
						OnComponentEndOverlap.Broadcast(this, result.m_result.GetActor(), result.m_result.GetComponent(), result.m_result.Item);
					}
					if (result.m_result.GetComponent()->GetGenerateOverlapEvents())
					{
						result.m_result.GetComponent()->OnComponentEndOverlap.Broadcast(result.m_result.GetComponent(), Cast<AActor>(this->GetOwner()), this, -1);
					}
				}
			}
		}
	}
}

void UTraceAndSweepCollisionComponent::SetTracePerSecond(float traces_per_second)
{
	m_traces_per_second = FMath::Max(0.0f, traces_per_second);
	if (m_traces_per_second > 0.0f)
	{
		float trace_interval = 1.0f / m_traces_per_second;
		m_tick_interval = trace_interval;
		SetComponentTickInterval(trace_interval);
	}
	else
	{
		m_tick_interval = 0.0f;
		SetComponentTickInterval(0.0f);
	}
}

void UTraceAndSweepCollisionComponent::SetIsTraceCollisionEnabled(bool is_enabled)
{
	m_is_trace_collision_enabled = is_enabled;

	if (m_is_trace_collision_enabled)
	{
		m_time_elapsed = 0.0f;

		// If collision is enabled then save the locations as previous location so that trace starts from correct location
		const USkeletalMeshComponent* parent_skeletal_mesh = Cast<USkeletalMeshComponent>(GetAttachParent());
		for (FCollisionLineData& line_data : m_collision_line_data)
		{
			if (line_data.m_scene_component_to_follow->IsValidLowLevel())
			{
				line_data.m_prev_location = line_data.m_scene_component_to_follow->GetComponentLocation();
			}
			else if (parent_skeletal_mesh && line_data.m_socket_name != NAME_None && parent_skeletal_mesh->DoesSocketExist(line_data.m_socket_name))
			{
				line_data.m_prev_location = parent_skeletal_mesh->GetSocketLocation(line_data.m_socket_name);
			}
		}

		// For shapes save both location and rotation of the shape since rotation will effect the collision detection for shapes
		const FTransform comp_transform = GetComponentTransform();
		for (FCollisionShapeData& shape_data : m_collision_shape_data)
		{
			const FTransform shape_transform = comp_transform * shape_data.m_offset;
			shape_data.m_prev_location = shape_transform.GetLocation();
			shape_data.m_prev_rotation = shape_transform.GetRotation();
		}
	}
}

void UTraceAndSweepCollisionComponent::AddSceneComponentToTrace(USceneComponent* scene_component)
{
	if (!scene_component) return;

	FCollisionLineData* data = m_collision_line_data.FindByPredicate([&](const FCollisionLineData& other) {return other.m_scene_component_to_follow == scene_component; });
	// Add if the scene component isn't being followed
	if (!data)
	{
		FCollisionLineData line_data;
		line_data.m_scene_component_to_follow = scene_component;
		m_collision_line_data.Add(line_data);
	}
}

void UTraceAndSweepCollisionComponent::AddSceneComponentsToTrace(TArray<USceneComponent*> scene_components)
{
	for (USceneComponent* comp : scene_components)
	{
		AddSceneComponentToTrace(comp);
	}
}


#if WITH_EDITOR
bool UTraceAndSweepCollisionComponent::CanEditChange(const FProperty* property) const
{
	if (!property) return false;

	// if other logic prevents editing, we want to respect that
	const bool can_edit = Super::CanEditChange(property);

	// Channel type
	if (property->GetFName() == GET_MEMBER_NAME_CHECKED(UTraceAndSweepCollisionComponent, m_trace_channel))
	{
		return can_edit && m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL;
	}
	if (property->GetFName() == GET_MEMBER_NAME_CHECKED(UTraceAndSweepCollisionComponent, m_object_channels))
	{
		return can_edit && m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL;
	}
	if (property->GetFName() == GET_MEMBER_NAME_CHECKED(UTraceAndSweepCollisionComponent, m_collision_preset))
	{
		return can_edit && m_channel_type == ECollisionCompChannelType::COLLISION_PRESET;
	}

	// Style type
	if (property->GetFName() == GET_MEMBER_NAME_CHECKED(UTraceAndSweepCollisionComponent, m_collision_line_data))
	{
		// Get attached parent isn't returning parent, so comment it out
		// TODO: Fix this so that line data can only be edited when its chid of skeletal mesh
		//USkeletalMesh* parent_mesh = Cast<USkeletalMesh>(GetAttachParent());
		return can_edit && m_style_type == ECollisionCompStyleType::LINE /* && parent_mesh*/;
	}

	if (property->GetFName() == GET_MEMBER_NAME_CHECKED(UTraceAndSweepCollisionComponent, m_collision_shape_data))
	{
		return can_edit && m_style_type == ECollisionCompStyleType::SWEEP;
	}

	// Shape type
	if (property->GetFName() == GET_MEMBER_NAME_CHECKED(FCollisionShapeData, m_box_half_extent))
	{
		// TODO: Figure out how to check for struct properties inside array.
	}

	return can_edit;
}
#endif


FPrimitiveSceneProxy* UTraceAndSweepCollisionComponent::CreateSceneProxy()
{
	/** Represents a UTraceAndSweepCollisionComponent to the scene manager. */
	class FTraceAndSweepCollisionSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FTraceAndSweepCollisionSceneProxy(const UTraceAndSweepCollisionComponent* comp)
			: FPrimitiveSceneProxy(comp)
			, m_comp(comp)
		{
			bWillEverBeLit = false;
			bWantsSelectionOutline = true;
			//bAlwaysVisible = true;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			SCOPE_CYCLE_COUNTER( STAT_TraceAndSweepCollisionSceneProxy_GetDynamicMeshElements );
			
			const FMatrix& local_to_world = GetLocalToWorld();
			
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];

					const FLinearColor color = GetViewSelectionColor(m_comp->m_debug_collision_shape_color, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected());
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					if (m_comp->m_style_type == ECollisionCompStyleType::LINE)
					{
						for (const FCollisionLineData& line : m_comp->m_collision_line_data)
						{
							FTransform transform = FTransform::Identity;
							bool is_valid_line_data = false;
							if (line.m_scene_component_to_follow->IsValidLowLevel())
							{
								is_valid_line_data = true;
								transform = line.m_scene_component_to_follow->GetComponentTransform();
							}
							else if (line.m_socket_name != NAME_None)
							{
								USkeletalMeshComponent* parent_component = Cast<USkeletalMeshComponent>(m_comp->GetAttachParent());
								if (parent_component && parent_component->DoesSocketExist(line.m_socket_name))
								{
									is_valid_line_data = true;
									transform = parent_component->GetSocketTransform(line.m_socket_name);
								}
							}

							if (is_valid_line_data)
							{
								// nullify scale
								transform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
								float middle_gap = 1.0f;
								FTransform temp = FTransform(FVector(-m_comp->m_debug_arrow_length - middle_gap, 0.0f, 0.0f)) * transform;
								DrawDirectionalArrow(PDI, temp.ToMatrixNoScale(), color, m_comp->m_debug_arrow_length, 2, SDPG_World, m_comp->m_debug_thickness);

								temp = FTransform(FQuat::MakeFromEuler(FVector(0.0f, 180.0f, 0.0f))) * FTransform(FVector(m_comp->m_debug_arrow_length + middle_gap, 0.0f, 0.0f)) * transform;
								DrawDirectionalArrow(PDI, temp.ToMatrixNoScale(), color, m_comp->m_debug_arrow_length, 2, SDPG_World, m_comp->m_debug_thickness);

								temp = FTransform(FQuat::MakeFromEuler(FVector(0.0f, -90.0f, 0.0f))) * FTransform(FVector(0.0f, 0.0f, m_comp->m_debug_arrow_length + middle_gap)) * transform;
								DrawDirectionalArrow(PDI, temp.ToMatrixNoScale(), color, m_comp->m_debug_arrow_length, 2, SDPG_World, m_comp->m_debug_thickness);

								temp = FTransform(FQuat::MakeFromEuler(FVector(0.0f, 90.0f, 0.0f))) * FTransform(FVector(0.0f, 0.0f, -m_comp->m_debug_arrow_length - middle_gap)) * transform;
								DrawDirectionalArrow(PDI, temp.ToMatrixNoScale(), color, m_comp->m_debug_arrow_length, 2, SDPG_World, m_comp->m_debug_thickness);

								temp = FTransform(FQuat::MakeFromEuler(FVector(0.0f, 0.0, 90.0f))) * FTransform(FVector(0.0f, -m_comp->m_debug_arrow_length - middle_gap, 0.0f)) * transform;
								DrawDirectionalArrow(PDI, temp.ToMatrixNoScale(), color, m_comp->m_debug_arrow_length, 2, SDPG_World, m_comp->m_debug_thickness);

								temp = FTransform(FQuat::MakeFromEuler(FVector(0.0f, 0.0, -90.0f))) * FTransform(FVector(0.0f, m_comp->m_debug_arrow_length + middle_gap, 0.0f)) * transform;
								DrawDirectionalArrow(PDI, temp.ToMatrixNoScale(), color, m_comp->m_debug_arrow_length, 2, SDPG_World, m_comp->m_debug_thickness);
							}
						}
					}
					else if (m_comp->m_style_type == ECollisionCompStyleType::SWEEP)
					{
						for (const FCollisionShapeData& shape : m_comp->m_collision_shape_data)
						{
							const FMatrix& shape_transform = shape.m_offset.ToMatrixWithScale() * local_to_world;

							if (shape.m_shape_type == ECollisionCompShapeType::BOX)
							{
								DrawWireBox(PDI
									, shape_transform
									, FBox(-shape.m_box_half_extent, shape.m_box_half_extent)
									, color
									, SDPG_World
									, m_comp->m_debug_thickness);
							}
							else if (shape.m_shape_type == ECollisionCompShapeType::CAPSULE)
							{
								const FVector& comp_scale = (shape.m_offset * m_comp->GetComponentTransform()).GetScale3D();

								FVector x, y, z;
								shape_transform.GetUnitAxes(x, y, z);

								// capsule should have uniform radius
								const float height = shape.m_capsule_half_height * comp_scale.Z;
								const float radius = shape.m_capsule_radius * FMath::Max(0.0f, FMath::Max(comp_scale.X, comp_scale.Y));

								DrawWireCapsule(PDI
									, shape_transform.GetOrigin()
									, x
									, y
									, z
									, color
									, radius
									, height
									, 32
									, SDPG_World
									, m_comp->m_debug_thickness);
							}
							else if (shape.m_shape_type == ECollisionCompShapeType::SPHERE)
							{
								const FVector& comp_scale = (shape.m_offset * m_comp->GetComponentTransform()).GetScale3D();

								// sphere should have uniform radius
								const float max_scale = FMath::Max(0.0f, FMath::Max(comp_scale.X, FMath::Max(comp_scale.Y, comp_scale.Z)));
								const float radius = shape.m_sphere_radius * max_scale;

								DrawWireSphere(PDI
									, shape_transform.GetOrigin()
									, color
									, radius
									, FMath::Max(radius / 2.f, 20.f)
									, SDPG_World
									, m_comp->m_debug_thickness);
							}
						}
					}
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			// TODO: Have to figure out which values to set so that the shapes are always visible instead of being visible only when selected.

			const bool bProxyVisible = true;

			// Should we draw this because collision drawing is enabled, and we have collision
			const bool bShowForCollision = View->Family->EngineShowFlags.Collision && IsCollisionEnabled();

			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = true;//(IsShown(View) && bProxyVisible) || bShowForCollision;
			Result.bStaticRelevance = true;
			Result.bDynamicRelevance = true;
			Result.bVelocityRelevance = true;

			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}
		virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + FPrimitiveSceneProxy::GetAllocatedSize()); }

	private:
		const UTraceAndSweepCollisionComponent* m_comp = nullptr;
	};

	return new FTraceAndSweepCollisionSceneProxy(this);
}
