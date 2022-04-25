#pragma once

#include "Math/Color.h"


namespace TraceAndSweepDebugDraw
{
    const float normal_length = 20.f;

    void DrawLineTraces(const UWorld* world
        , const FVector& start
        , const FVector& end
        , const TArray<FHitResult>& hits
        , const bool is_draw_debug_trace_line
        , const FColor& debug_line_color
        , const float debug_line_duration
        , const bool is_draw_hit_normal
        , const FColor& hit_normal_color
        , const FColor& touch_normal_color
        , const float hit_marker_duration
        , const float debug_line_thickness);

    void DrawBoxSweep(const UWorld* world
        , const FVector& start
        , const FVector& end
        , const FVector& extent
        , const FQuat& rot
        , const TArray<FHitResult>& hits
        , const bool is_draw_debug_box
        , const FColor& debug_box_color
        , const float debug_box_duration
        , const bool is_draw_hit_normal
        , const FColor& hit_normal_color
        , const FColor& touch_normal_color
        , const float hit_marker_duration
        , const float debug_line_thickness);

    void DrawSphereSweep(const UWorld* world
        , const FVector& start
        , const FVector& end
        , const float radius
        , const TArray<FHitResult>& hits
        , const bool is_draw_debug_sphere
        , const FColor& debug_sphere_color
        , const float debug_sphere_duration
        , const bool is_draw_hit_normal
        , const FColor& hit_normal_color
        , const FColor& touch_normal_color
        , const float hit_marker_duration
        , const float debug_line_thickness);

    void DrawCapsuleSweep(const UWorld* world
        , const FVector& start
        , const FVector& end
        , const float half_height
        , const float radius
        , const FQuat& rot
        , const TArray<FHitResult>& hits
        , const bool is_draw_debug_capsule
        , const FColor& debug_capsule_color
        , const float debug_capsule_duration
        , const bool is_draw_hit_normal
        , const FColor& hit_normal_color
        , const FColor& touch_normal_color
        , const float hit_marker_duration
        , const float debug_line_thickness);
};