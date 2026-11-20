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

void UHandComponent::MoveHand(const FVector2D& InInput, float Speed, float MaxDistance, const FVector& ShoulderWorldLocation, float DeltaTime)
{
	// 入力が無い、または可動範囲が0なら移動しない
	if (InInput.IsNearlyZero() || MaxDistance <= 0.f) {
		return;
	}

	// 体の向きを基準に、壁と平行な方向へ移動する
	const FVector LocalDelta(0.f, InInput.X, InInput.Y);
	const FRotator BodyRotation = GetAttachParent() ? GetAttachParent()->GetComponentRotation() : GetComponentRotation();
	AddWorldOffset(BodyRotation.RotateVector(LocalDelta) * Speed * DeltaTime);

	// 肩から最大距離を超えた位置を境界に補正する
	FVector Offset = GetComponentLocation() - ShoulderWorldLocation;
	if (Offset.SizeSquared() > FMath::Square(MaxDistance)) {
		Offset = Offset.GetSafeNormal() * MaxDistance;
		SetWorldLocation(ShoulderWorldLocation + Offset);
	}
}

void UHandComponent::UpdateHandRotation(const FVector& ShoulderWorldLocation, const FVector& PalmNormalWorld)
{
	const FVector HandLocation = GetComponentLocation();
	FVector FingerDirection = (HandLocation - ShoulderWorldLocation).GetSafeNormal();

	// 掌が肩と同じ位置にある場合は上方向を指す
	if (FingerDirection.IsNearlyZero()) {
		FingerDirection = FVector::UpVector;
	}

	// 掌の法線を指先方向に直交化する
	FVector PalmNormal = PalmNormalWorld - FingerDirection * FVector::DotProduct(PalmNormalWorld, FingerDirection);
	if (PalmNormal.IsNearlyZero()) {
		PalmNormal = FVector::UpVector - FingerDirection * FVector::DotProduct(FVector::UpVector, FingerDirection);
	}
	PalmNormal = PalmNormal.GetSafeNormal();

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
	SetWorldRotation(RotationMatrix.Rotator());
}

FVector2D UHandComponent::GetMoveInput() const
{
	return MoveInput;
}

bool UHandComponent::IsGrabHeld() const
{
	return bGrabHeld;
}
