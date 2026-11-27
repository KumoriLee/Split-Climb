#include "Tanimura/Component/HandComponent.h"
#include "Math/RotationMatrix.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UHandComponent::UHandComponent()
{
	// 掌は毎フレームの処理を持たない
	PrimaryComponentTick.bCanEverTick = false;
}

void UHandComponent::SetMoveInput(const FVector2D& InMoveInput)
{
	MoveInput = InMoveInput;
}

void UHandComponent::SetGrabHeld(bool bInGrabHeld)
{
	bGrabHeld = bInGrabHeld;
}

void UHandComponent::MoveHand(const FVector2D& InInput, float Speed, float MaxRadius, const FVector& ShoulderWorldLocation, float DeltaTime)
{
	// 掴んでいる間は掌を掴んだ位置へ固定する
	if (bIsGrabbing) {
		SetWorldLocation(GrabLocation);
		return;
	}

	// 可動範囲が0なら移動しない
	if (MaxRadius <= 0.f) {
		return;
	}

	// 入力がある場合だけ体の向きを基準に壁と平行な方向へ移動する
	if (!InInput.IsNearlyZero()) {
		const FVector LocalDelta(0.f, InInput.X, InInput.Y);
		const FRotator BodyRotation = GetAttachParent() ? GetAttachParent()->GetComponentRotation() : GetComponentRotation();
		AddWorldOffset(BodyRotation.RotateVector(LocalDelta) * Speed * DeltaTime);
	}

	// 壁面内の可動半径を超えた位置を境界に補正する
	ClampToMoveRadius(ShoulderWorldLocation, MaxRadius);
}

void UHandComponent::UpdateHandRotation(const FVector& ShoulderWorldLocation, const FVector& PalmNormalWorld, const FRotator& RotationOffset)
{
	FVector PalmNormal = PalmNormalWorld.GetSafeNormal();

	// 掌の法線が未設定の場合は前方向を基準にする
	if (PalmNormal.IsNearlyZero()) {
		PalmNormal = FVector::ForwardVector;
	}

	const FVector HandLocation = GetComponentLocation();

	// 肩から掌への方向を壁面へ投影する
	FVector FingerDirection = HandLocation - ShoulderWorldLocation;
	FingerDirection -= PalmNormal * FVector::DotProduct(FingerDirection, PalmNormal);
	FingerDirection = FingerDirection.GetSafeNormal();

	// 肩と掌が同じ位置にある場合は上方向を壁面へ投影する
	if (FingerDirection.IsNearlyZero()) {
		FVector FallbackDirection = FVector::UpVector;
		FallbackDirection -= PalmNormal * FVector::DotProduct(FallbackDirection, PalmNormal);
		if (FallbackDirection.IsNearlyZero()) {
			FallbackDirection = FVector::ForwardVector;
			FallbackDirection -= PalmNormal * FVector::DotProduct(FallbackDirection, PalmNormal);
		}
		FingerDirection = FallbackDirection.GetSafeNormal();
	}

	FVector XAxis;
	FVector YAxis;
	if (HandSide == EHandSide::Left) {
		// 左手は指先がローカルX、掌の法線がローカル-Y
		XAxis = FingerDirection;
		YAxis = -PalmNormal;
	} else {
		// 右手は指先がローカル-X、掌の法線がローカル+Y
		XAxis = -FingerDirection;
		YAxis = PalmNormal;
	}

	const FMatrix RotationMatrix = FRotationMatrix::MakeFromXY(XAxis, YAxis);
	const FQuat FinalRotation = RotationMatrix.ToQuat() * RotationOffset.Quaternion();
	SetWorldRotation(FinalRotation.Rotator());
}

FVector2D UHandComponent::GetMoveInput() const
{
	return MoveInput;
}

bool UHandComponent::IsGrabHeld() const
{
	return bGrabHeld;
}

bool UHandComponent::TryGrab(const FVector& TraceDirection, float ReachDistance, float SurfaceOffset, ECollisionChannel TraceChannel)
{
	// 掴んでいる、または探索距離が0の場合は掴まない
	if (bIsGrabbing || ReachDistance <= 0.f) {
		return false;
	}

	UWorld* World = GetWorld();
	if (!World) {
		return false;
	}

	// 掌の向きを基準に探索方向を決める
	FVector Direction = TraceDirection.GetSafeNormal();
	if (Direction.IsNearlyZero()) {
		Direction = FVector::ForwardVector;
	}

	const FVector Start = GetComponentLocation();
	const FVector End = Start + Direction * ReachDistance;

	FCollisionQueryParams Params(FName(TEXT("HandGrabTrace")), false, GetOwner());
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	// 掴める面が無い、または面の中から始まる場合は掴まない
	if (!bHit || Hit.bStartPenetrating) {
		return false;
	}

	// 掴んだ位置に掌を固定する
	bIsGrabbing = true;
	GrabNormal = Hit.ImpactNormal.GetSafeNormal();
	GrabLocation = Hit.ImpactPoint + GrabNormal * SurfaceOffset;
	SetWorldLocation(GrabLocation);
	return true;
}

void UHandComponent::ReleaseGrab()
{
	// 掴んでいなければ何もしない
	if (!bIsGrabbing) {
		return;
	}

	// 掌は掴んだ位置に残し、掴み状態だけを解除する
	bIsGrabbing = false;
	GrabLocation = FVector::ZeroVector;
	GrabNormal = FVector::ForwardVector;
}

bool UHandComponent::IsGrabbing() const
{
	return bIsGrabbing;
}

FVector UHandComponent::GetGrabLocation() const
{
	return GrabLocation;
}

FVector UHandComponent::GetGrabNormal() const
{
	return GrabNormal;
}

void UHandComponent::ClampToMoveRadius(const FVector& ShoulderWorldLocation, float MaxRadius)
{
	// 可動半径が0の場合はクランプしない
	if (MaxRadius <= 0.f) {
		return;
	}

	FVector Offset = GetComponentLocation() - ShoulderWorldLocation;
	FVector PlaneOffset(0.f, Offset.Y, Offset.Z);

	// 前後位置は変えず、左右と上下だけを半径に収める
	if (PlaneOffset.SizeSquared() > FMath::Square(MaxRadius)) {
		PlaneOffset = PlaneOffset.GetSafeNormal() * MaxRadius;
		Offset.Y = PlaneOffset.Y;
		Offset.Z = PlaneOffset.Z;
		SetWorldLocation(ShoulderWorldLocation + Offset);
	}
}
