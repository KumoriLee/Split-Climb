#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "HandComponent.generated.h"

// 掌の左右を区別する
UENUM(BlueprintType)
enum class EHandSide : uint8
{
	Left	UMETA(DisplayName = "Left"),
	Right	UMETA(DisplayName = "Right")
};

/**
 * 掌の目標位置と入力状態を保持する
 */

UCLASS(ClassGroup = (Climb), meta = (BlueprintSpawnableComponent))
class SPLITCLIMB_API UHandComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UHandComponent();

	// 掌の左右
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand")
	EHandSide HandSide = EHandSide::Left;

	// スティック入力の値
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	FVector2D MoveInput = FVector2D::ZeroVector;

	// 掴みボタンを押しているか
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	bool bGrabHeld = false;

	// 実際に掴めているか
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	bool bIsGrabbing = false;

	// 掴んでいる面の位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	FVector GrabLocation = FVector::ZeroVector;

	// 掴んでいる面の法線
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	FVector GrabNormal = FVector::ForwardVector;

	// 入力値を設定する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void SetMoveInput(const FVector2D& InMoveInput);

	// 掴み状態を設定する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void SetGrabHeld(bool bInGrabHeld);

	// 肩のワールド位置を基準に掌を移動する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void MoveHand(const FVector2D& InInput, float Speed, float MaxRadius, const FVector& ShoulderWorldLocation, float DeltaTime);

	// 肩と掌の位置から手の向きを更新する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void UpdateHandRotation(const FVector& ShoulderWorldLocation, const FVector& PalmNormalWorld, const FRotator& RotationOffset);

	// 入力値を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector2D GetMoveInput() const;

	// 掴み状態を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	bool IsGrabHeld() const;

	// 掌の位置から掴める面を探して掴む
	UFUNCTION(BlueprintCallable, Category = "Hand")
	bool TryGrab(const FVector& TraceDirection, float ReachDistance, float SurfaceOffset, ECollisionChannel TraceChannel);

	// 掴みを解除する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void ReleaseGrab();

	// 掴んでいるかを取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	bool IsGrabbing() const;

	// 掴んでいる位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector GetGrabLocation() const;

	// 掴んでいる面の法線を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector GetGrabNormal() const;

private:
	// 掌を肩からの可動半径へ収める
	void ClampToMoveRadius(const FVector& ShoulderWorldLocation, float MaxRadius);
};
