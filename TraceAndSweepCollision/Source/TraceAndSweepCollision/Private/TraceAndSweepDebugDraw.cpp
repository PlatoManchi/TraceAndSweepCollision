#include "TraceAndSweepDebugDraw.h"

#include "Engine.h"
#include "DrawDebugHelpers.h"
#include "Math/RotationMatrix.h"

void TraceAndSweepDebugDraw::DrawLineTraces(const UWorld* world
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
	, const float debug_line_thickness)
{
#if !UE_BUILD_SHIPPING
	//FColor color = (hits.Num() > 0) ? CollisionDebugDrawing::HitColor : CollisionDebugDrawing::TraceColor;
	if (is_draw_debug_trace_line)
	{
		DrawDebugPoint(world, start, debug_line_thickness + 10.0f, debug_line_color, false, debug_line_duration, 0);
		DrawDebugPoint(world, end, debug_line_thickness + 10.0f, debug_line_color, false, debug_line_duration, 0);
		DrawDebugLine(world, start, end, debug_line_color, false, debug_line_duration, 0, debug_line_thickness);
	}
	
	if (is_draw_hit_normal)
	{
		for (const FHitResult& hit : hits)
		{
			FVector normal_start = hit.Location;
			FVector normal_end = normal_start + (hit.Normal * normal_length);
			const FColor normal_color = hit.bBlockingHit ? hit_normal_color : touch_normal_color;
			//DrawDebugPoint(world, normal_start, debug_line_thickness + 10.0f, normal_color, false, hit_marker_duration, 0);
			DrawDebugDirectionalArrow(world, normal_start, normal_end, debug_line_thickness + 5.f, normal_color, false, hit_marker_duration, 0, debug_line_thickness);

			normal_start = hit.ImpactPoint;
			normal_end = normal_start + (hit.ImpactNormal * normal_length);
			//DrawDebugPoint(world, normal_start, debug_line_thickness + 10.0f, normal_color, false, hit_marker_duration, 0);
			DrawDebugDirectionalArrow(world, normal_start, normal_end, debug_line_thickness + 5.f, normal_color, false, hit_marker_duration, 0, debug_line_thickness);
		}
	}
#endif
}

void TraceAndSweepDebugDraw::DrawBoxSweep(const UWorld* world
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
	, const float debug_line_thickness)
{
#if !UE_BUILD_SHIPPING
	FBox start_box(start - extent, start + extent);
	FBox end_box(end - extent, end + extent);

	if (is_draw_debug_box)
	{
		FColor color = debug_box_color;
		FVector direction = rot.Vector();

		//DrawDebugLine(world, start, end, color, false, debug_box_duration, 0, debug_line_thickness);
		DrawDebugBox(world, start_box.GetCenter(), start_box.GetExtent(), rot, color, false, debug_box_duration, 0, debug_line_thickness);
		DrawDebugBox(world, end_box.GetCenter(), end_box.GetExtent(), rot, color, false, debug_box_duration, 0, debug_line_thickness);

		// connect the vertices of start box with end box
		// since start_box and end_box are already translated applying rotation will lead to rotating translated box around origin.
		// so construct each vertices of the box as if the center of box is at origin and rotate the point and then translate it back to start and end positions.
		FTransform const box_transform(rot);
		// (-1, -1, 1)
		FVector transform_point = box_transform.TransformPosition(FVector(-extent.X, -extent.Y, extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		// (1, -1, 1)
		transform_point = box_transform.TransformPosition(FVector(extent.X, -extent.Y, extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		// (1, 1, 1)
		transform_point = box_transform.TransformPosition(FVector(extent.X, extent.Y, extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		// (-1, 1, 1)
		transform_point = box_transform.TransformPosition(FVector(-extent.X, extent.Y, extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		//(-1, -1, -1)
		transform_point = box_transform.TransformPosition(FVector(-extent.X, -extent.Y, -extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		// (1, -1, -1)
		transform_point = box_transform.TransformPosition(FVector(extent.X, -extent.Y, -extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		//(1, 1, -1)
		transform_point = box_transform.TransformPosition(FVector(extent.X, extent.Y, -extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
		// (-1, 1, -1)
		transform_point = box_transform.TransformPosition(FVector(-extent.X, extent.Y, -extent.Z));
		DrawDebugLine(world, transform_point + start, transform_point + end, color, false, debug_box_duration, 0, debug_line_thickness);
	}

	if (is_draw_hit_normal)
	{
		for (const FHitResult& hit : hits)
		{
			FVector normal_start = hit.Location;
			FVector normal_end = normal_start + (hit.Normal * normal_length);
			const FColor normal_color = hit.bBlockingHit ? hit_normal_color : touch_normal_color;
			DrawDebugDirectionalArrow(world, normal_start, normal_end, 5.f, normal_color, false, hit_marker_duration);
			normal_start = hit.ImpactPoint;
			normal_end = normal_start + (hit.ImpactNormal * normal_length);
			DrawDebugDirectionalArrow(world, normal_start, normal_end, 5.f, normal_color, false, hit_marker_duration);
		}
	}

#endif
}


void TraceAndSweepDebugDraw::DrawSphereSweep(const UWorld* world
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
	, const float debug_line_thickness)
{
#if !UE_BUILD_SHIPPING
	if (is_draw_debug_sphere)
	{
		FColor color = debug_sphere_color;
		DrawDebugSphere(world, start, radius, FMath::Max(radius / 4.f, 2.f), color, false, debug_sphere_duration, 0, debug_line_thickness);
		DrawDebugSphere(world, end, radius, FMath::Max(radius / 4.f, 2.f), color, false, debug_sphere_duration, 0, debug_line_thickness);
		DrawDebugLine(world, start + FVector(0, 0, radius), end + FVector(0, 0, radius), color, false, debug_sphere_duration, 0, debug_line_thickness);
		DrawDebugLine(world, start - FVector(0, 0, radius), end - FVector(0, 0, radius), color, false, debug_sphere_duration, 0, debug_line_thickness);
	}

	if (is_draw_hit_normal)
	{
		for (const FHitResult& hit : hits)
		{
			FVector normal_start = hit.Location;
			FVector normal_end = normal_start + (hit.Normal * normal_length);
			const FColor normal_color = hit.bBlockingHit ? hit_normal_color : touch_normal_color;
			DrawDebugDirectionalArrow(world, normal_start, normal_end, 5.f, normal_color, false, hit_marker_duration, 0, debug_line_thickness);
			normal_start = hit.ImpactPoint;
			normal_end = normal_start + (hit.ImpactNormal * normal_length);
			DrawDebugDirectionalArrow(world, normal_start, normal_end, 5.f, normal_color, false, hit_marker_duration, 0, debug_line_thickness);
		}
	}

#endif
}


void TraceAndSweepDebugDraw::DrawCapsuleSweep(const UWorld* world
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
	, const float debug_line_thickness)
{
#if !UE_BUILD_SHIPPING
	if (is_draw_debug_capsule)
	{
		FColor color = debug_capsule_color;
		/*if (Hits.Num() > 0 && Hits[0].bBlockingHit)
		{
			Color = CollisionDebugDrawing::PenetratingColor;
			//		LifeTime=0.f;
		}*/
		DrawDebugLine(world, start, end, color, false, debug_capsule_duration, 0, debug_line_thickness);
		DrawDebugCapsule(world, start, half_height, radius, rot, color, false, debug_capsule_duration, 0, debug_line_thickness);
		DrawDebugCapsule(world, end, half_height, radius, rot, color, false, debug_capsule_duration, 0, debug_line_thickness);

		FVector center_to_end_tip = rot.RotateVector(FVector(0, 0, half_height));

		DrawDebugLine(world, start + center_to_end_tip, end + center_to_end_tip, color, false, debug_capsule_duration, 0, debug_line_thickness);
		DrawDebugLine(world, start - center_to_end_tip, end - center_to_end_tip, color, false, debug_capsule_duration, 0, debug_line_thickness);

		FVector direction = (end - start);
		direction.Normalize();
		FVector up(0, 0, 1);
		FVector right = direction ^ up;
		right *= radius;

		DrawDebugLine(world, start - right, end - right, color, false, debug_capsule_duration, 0, debug_line_thickness);
		DrawDebugLine(world, start + right, end + right, color, false, debug_capsule_duration, 0, debug_line_thickness);
	}

	if (is_draw_hit_normal)
	{
		for (const FHitResult& hit : hits)
		{
			FVector normal_start = hit.Location;
			FVector normal_end = normal_start + (hit.Normal * normal_length);
			const FColor normal_color = hit.bBlockingHit ? hit_normal_color : touch_normal_color;
			DrawDebugDirectionalArrow(world, normal_start, normal_end, 5.f, normal_color, false, hit_marker_duration, 0, debug_line_thickness);
			normal_start = hit.ImpactPoint;
			normal_end = normal_start + (hit.ImpactNormal * normal_length);
			DrawDebugDirectionalArrow(world, normal_start, normal_end, 5.f, FColor(255, 255, 0), false, hit_marker_duration, 0, debug_line_thickness);
		}
	}
#endif
}
