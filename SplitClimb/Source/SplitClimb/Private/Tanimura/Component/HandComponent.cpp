#include "Tanimura/Component/HandComponent.h"

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

	// ローカルYを左右、ローカルZを上下として移動する
	const FVector Delta(0.f, InInput.X, InInput.Y);
	AddLocalOffset(Delta * Speed * DeltaTime);

	// 肩から最大距離を超えた位置を境界に補正する
	FVector Offset = GetComponentLocation() - ShoulderWorldLocation;
	if (Offset.SizeSquared() > FMath::Square(MaxDistance)) {
		Offset = Offset.GetSafeNormal() * MaxDistance;
		SetWorldLocation(ShoulderWorldLocation + Offset);
	}
}

FVector2D UHandComponent::GetMoveInput() const
{
	return MoveInput;
}

bool UHandComponent::IsGrabHeld() const
{
	return bGrabHeld;
}
