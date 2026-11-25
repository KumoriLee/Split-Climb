#include "Tanimura/Component/HandComponent.h"
#include "Math/RotationMatrix.h"

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
	// 入力が無い、または可動範囲が0なら移動しない
	if (InInput.IsNearlyZero() || MaxRadius <= 0.f) {
		return;
	}

	// 体の向きを基準に、壁と平行な方向へ移動する
	const FVector LocalDelta(0.f, InInput.X, InInput.Y);
	const FRotator BodyRotation = GetAttachParent() ? GetAttachParent()->GetComponentRotation() : GetComponentRotation();
	AddWorldOffset(BodyRotation.RotateVector(LocalDelta) * Speed * DeltaTime);

	// 壁面内の可動半径を超えた位置を境界に補正する
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
