#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "TraceAndSweepCollisionTypes.h"
#include "TraceAndSweepCollisionComponent.generated.h"

//For Unreal Profiler
DECLARE_STATS_GROUP(TEXT("TraceAndSweepCollisionComponent"), STATGROUP_TraceAndSweepCollisionComponent, STATCAT_Advanced);

UCLASS(ClassGroup=(TraceAndSweepCollision), meta = (BlueprintSpawnableComponent))
class TRACEANDSWEEPCOLLISION_API UTraceAndSweepCollisionComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

	// making component a friend of manager so that manager can manage the class without restrictions
	friend class ATraceAndSweepCollisionManager;

public:	
	UTraceAndSweepCollisionComponent();

	UFUNCTION(BlueprintCallable, Category = "TraceAndSweepCollision")
	void SetTracePerSecond(float traces_per_second);

	UFUNCTION(BlueprintCallable, Category = "TraceAndSweepCollision")
	void SetIsTraceCollisionEnabled(bool is_enabled);

	UFUNCTION(BlueprintCallable, Category = "TraceAndSweepCollision")
	void AddSceneComponentToTrace(USceneComponent* scene_component);

	UFUNCTION(BlueprintCallable, Category = "TraceAndSweepCollision")
	void AddSceneComponentsToTrace(TArray<USceneComponent*> scene_components);

	FORCEINLINE bool IsTraceCollisionEnabled() const { return m_is_trace_collision_enabled; }

protected:
	// UPrimitiveComponent overrides
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float delta_time, ELevelTick tick_type, FActorComponentTickFunction* this_tick_function) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent overrides

	//~ Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// virtual void CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const override;
	//~ End USceneComponent Interface
private:
	// Wrapper for FHitResult since FHitResult doesn't have == operator
	struct FHitResultWrapper
	{
		int32 m_count = 0;
		FHitResult m_result;

		FHitResultWrapper(const FHitResult& result);
		bool operator==(const FHitResultWrapper& rhs) const;
	};

	bool DoCollisionTest();
	
	bool DoSynchronousLineSingleCollisionTest();
	bool DoSynchronousSweepSingleCollisionTest();
	bool DoSynchronousLineMultiCollisionTest();
	bool DoSynchronousSweepMultiCollisionTest();

	bool DoAsynchronousLineCollisionTest();
	bool DoAsynchronousSweepCollisionTest();

	// Helpers
	// returns true if trace or sweep is successfullly done
	// NOTE: return boolean isn't if its blocking hit or not, but weather trace of sweep is done
	bool DoLineSingle(FHitResult& out_result, UWorld* world, const FVector& start, const FVector& end, const FCollisionQueryParams& params) const;
	bool DoLineMulti(TArray<FHitResult>& out_results, UWorld* world, const FVector& start, const FVector& end, const FCollisionQueryParams& params, bool is_reverse = false) const;
	bool DoSweepSingle(FHitResult& out_result, UWorld* world, const FVector& start, const FVector& end, const FQuat rotation, const FCollisionShapeData& shape_data, const FCollisionQueryParams& params) const;
	bool DoSweepMulti(TArray<FHitResult>& out_results, UWorld* world, const FVector& start, const FVector& end, const FQuat rotation, const FCollisionShapeData& shape_data, const FCollisionQueryParams& params, bool is_reverse = false) const;
	
	FTraceHandle DoAsyncLine(UWorld* world, const EAsyncTraceType trace_type, const FVector& start, const FVector& end, const FCollisionQueryParams& params, bool is_reverse = false);
	FTraceHandle DoAsyncSweep(UWorld* world, const EAsyncTraceType trace_type, const FVector& start, const FVector& end, const FQuat rotation, const FCollisionShapeData& shape_data, const FCollisionQueryParams& params, bool is_reverse = false);

	void OnAsyncTraceComplete(const FTraceHandle& handle, FTraceDatum& data);

	void ProcessForwardHitResults();
	void ProcessReverseHitResults();

	// Called from manager
	void ExternalTick();

	// tick times being used by manager
	float m_time_elapsed = 0.0f;
	float m_tick_interval = 0.0f;

	bool m_is_previous_trace_complete = true;
	FTraceDelegate m_async_trace_delegate;

	// Temporary cache for all the hit results from current collision test
	TArray<FHitResultWrapper> m_forward_hit_results;
	TArray<FHitResultWrapper> m_reverse_hit_results;

	// Saved begin overlaps, so that end overlaps can be called
	TArray<FHitResultWrapper> m_overlapped_results;


	UPROPERTY(BlueprintReadonly, Transient, Category = "TraceAndSweepCollision", meta = (DisplayName = "Is Trace Collision Enabled", AllowPrivateAccess))
	bool m_is_trace_collision_enabled = false;

	// 0 to trace every frame
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Traces Per Second", MinValue = 0, AllowPrivateAccess))
	float m_traces_per_second = 30.0f;

	// Should start with collision enabled
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Start With Collision Enabled", AllowPrivateAccess))
	bool m_start_with_collision_enabled = true;

	// Should generate end overlap event. Set this to true if there are multiple points or shapes to trace
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Should Generate End Overlap", AllowPrivateAccess))
	bool m_should_generate_end_overlap = true;

	// Use Synchronous or Asynchronous tracing
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Execution Type", AllowPrivateAccess))
	ECollisionCompExecutionType m_execution_type = ECollisionCompExecutionType::ASYNCHRONOUS;

	// Use Single or Multi
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Trace Type", AllowPrivateAccess))
	ECollisionCompTraceType m_trace_type = ECollisionCompTraceType::SINGLE;


	// Use Object type or Trace channel or Collision preset
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Channel Type", AllowPrivateAccess))
	ECollisionCompChannelType m_channel_type = ECollisionCompChannelType::COLLISION_PRESET;
	
	// Trace channel to use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Trace Channel", EditCondition = "m_channel_type == ECollisionCompChannelType::TRACE_CHANNEL", EditConditionHides, AllowPrivateAccess))
	TEnumAsByte<ECollisionChannel> m_trace_channel;

	// Object channels to use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Object Channels", EditCondition = "m_channel_type == ECollisionCompChannelType::OBJECT_CHANNEL", EditConditionHides, AllowPrivateAccess))
	TArray<TEnumAsByte<EObjectTypeQuery>> m_object_channels;

	// Collision preset to use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Collision Preset", EditCondition = "m_channel_type == ECollisionCompChannelType::COLLISION_PRESET", EditConditionHides, AllowPrivateAccess))
	FCollisionProfileName m_collision_preset;


	// Use Trace or Sweep
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Style Type", AllowPrivateAccess))
	ECollisionCompStyleType m_style_type = ECollisionCompStyleType::LINE;

	// If trace, then list of scene components or bones to follow
	// Any scene component that is child of this, would be added into list of components to track.
	// 
	// NOTE: Add new elements in editor only if you want to track sockets in skeletal mesh. Tracking sockets only works if parent is skeletal mesh component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Line Data", EditCondition = "m_style_type == ECollisionCompStyleType::LINE", EditConditionHides, AllowPrivateAccess, TitleProperty = "m_socket_name"))
	TArray<FCollisionLineData> m_collision_line_data;

	// If sweep, then list of shapes to use for collision test
	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "TraceAndSweepCollision", meta = (DisplayName = "Shapes List", EditCondition = "m_style_type == ECollisionCompStyleType::SWEEP", EditConditionHides, AllowPrivateAccess, TitleProperty = "m_shape_type"))
	TArray<FCollisionShapeData> m_collision_shape_data;
	
	

	/** Whether we should trace against complex collision */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", AdvancedDisplay, meta = (DisplayName = "Trace Complex", AllowPrivateAccess))
	bool m_trace_complex = false;

	/** Whether we want to return the triangle face index for complex static mesh traces */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", AdvancedDisplay, meta = (DisplayName = "Return Face Index", AllowPrivateAccess))
	bool m_return_face_index = false;

	/** Whether we want to include the physical material in the results. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", AdvancedDisplay, meta = (DisplayName = "Return Physical Material", AllowPrivateAccess))
	bool m_return_physical_material = false;

	/** Whether to ignore blocking results. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", AdvancedDisplay, meta = (DisplayName = "Ignore Blocks", AllowPrivateAccess))
	bool m_ignore_blocks = false;

	/** Whether to ignore touch/overlap results. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", AdvancedDisplay, meta = (DisplayName = "Ignore Touches", AllowPrivateAccess))
	bool m_ignore_touches = false;

	/** Whether to skip narrow phase checks (only for overlaps). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TraceAndSweepCollision", AdvancedDisplay, meta = (DisplayName = "Skip Narrow Phase", AllowPrivateAccess))
	bool m_skip_narrow_phase = false;



#pragma region Debug
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Draw Collision Shape", DevelopmentOnly))
	bool m_debug_draw_collision_shape = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Collision Shape Color", EditCondition = "m_debug_draw_collision_shape", EditConditionHides, DevelopmentOnly))
	FColor m_debug_collision_shape_color = FColor::Blue;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Line Thickness", EditCondition = "m_debug_draw_collision_shape", EditConditionHides, DevelopmentOnly))
	float m_debug_thickness = 0.0f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Point Arrow Length", EditCondition = "m_debug_draw_collision_shape", EditConditionHides, DevelopmentOnly))
	float m_debug_arrow_length = 10.0f;

	// Line Trace debug variables
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Draw Debug Trace Line", EditCondition = "m_style_type == ECollisionCompStyleType::LINE", EditConditionHides, DevelopmentOnly))
	bool m_debug_draw_trace_lines = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Line Color", EditCondition = "m_style_type == ECollisionCompStyleType::LINE && m_debug_draw_trace_lines", EditConditionHides, DevelopmentOnly))
	FColor m_debug_line_color = FColor::Orange;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Line Duration", EditCondition = "m_style_type == ECollisionCompStyleType::LINE && m_debug_draw_trace_lines", EditConditionHides, DevelopmentOnly))
	float m_debug_line_duration = 2.0f;

	// Sweep trace debug variables
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Draw Sweep Shape", EditCondition = "m_style_type == ECollisionCompStyleType::SWEEP", EditConditionHides, DevelopmentOnly))
	bool m_debug_draw_sweep_shape = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Shape Color", EditCondition = "m_style_type == ECollisionCompStyleType::SWEEP && m_debug_draw_sweep_shape", EditConditionHides, DevelopmentOnly))
	FColor m_debug_sweep_shape_color = FColor::Orange;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Shape Duration", EditCondition = "m_style_type == ECollisionCompStyleType::SWEEP && m_debug_draw_sweep_shape", EditConditionHides, DevelopmentOnly))
	float m_debug_sweep_shape_duration = 2.0f;


	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Draw Debug Hit Point", DevelopmentOnly))
	bool m_draw_debug_hit_point = true;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Hit Normal Color", EditCondition = "m_draw_debug_hit_point", EditConditionHides, DevelopmentOnly))
	FColor m_debug_hit_normal_color = FColor(255, 64, 64);

	UPROPERTY(EditAnywhere, AdvancedDisplay, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Touch Normal Color", EditCondition = "m_draw_debug_hit_point", EditConditionHides, DevelopmentOnly))
	FColor m_debug_touch_normal_color = FColor(64, 255, 64);

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "TraceAndSweepCollision", meta = (DisplayName = "Debug Hit Marker Duration", EditCondition = "m_draw_debug_hit_point", EditConditionHides, DevelopmentOnly))
	float m_debug_hit_marker_duration = 2.0f;


	UPROPERTY(EditAnywhere, Category = "TraceAndSweepCollision", meta = (DisplayName = "Draw Line Thickness"))
	float m_debug_line_thickness = 0.0f;
#pragma endregion




	// -------------------------------------------------------------------------------------------------------------------------
	// --------------------------------------------- Editor Functions ----------------------------------------------------------
	// -------------------------------------------------------------------------------------------------------------------------
#if WITH_EDITOR
protected:
	// Before 4.23 edit condition with C++ statements isn't supported, so use this function for editors before 4.23
	// Uproperty is depecated and now use FProperty since 4.26, so this might not compile in older versions
	virtual bool CanEditChange(const FProperty* property) const override;
#endif
};
