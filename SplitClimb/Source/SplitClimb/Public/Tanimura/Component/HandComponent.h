#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
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

	// 入力値を設定する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void SetMoveInput(const FVector2D& InMoveInput);

	// 掴み状態を設定する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void SetGrabHeld(bool bInGrabHeld);

	// 肩のワールド位置を基準に掌を移動する
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void MoveHand(const FVector2D& InInput, float Speed, float MaxDistance, const FVector& ShoulderWorldLocation, float DeltaTime);

	// 入力値を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector2D GetMoveInput() const;

	// 掴み状態を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	bool IsGrabHeld() const;
};
