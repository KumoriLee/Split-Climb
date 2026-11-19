#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ClimbCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UHandComponent;
class UClimbSettings;
struct FInputActionValue;

/**
 * 二人で操作する壁登りキャラクター
 */

UCLASS()
class SPLITCLIMB_API AClimbCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AClimbCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	// 左手の目標位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	TObjectPtr<UHandComponent> LeftHand;

	// 右手の目標位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	TObjectPtr<UHandComponent> RightHand;

	// 左肩のボーン名
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FName LeftShoulderBoneName = TEXT("upperarm_l");

	// 右肩のボーン名
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FName RightShoulderBoneName = TEXT("upperarm_r");

	// 左手の移動アクション
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LeftHandMoveAction;

	// 右手の移動アクション
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RightHandMoveAction;

	// 左手の掴みアクション
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LeftGrabAction;

	// 右手の掴みアクション
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RightGrabAction;

	// キーボード操作のマッピングコンテキスト
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> KeyboardMappingContext;

	// 壁登りの調整値
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Climb")
	TObjectPtr<UClimbSettings> ClimbSettings;

private:
	void UpdateHandMovement(float DeltaTime);

	FVector GetShoulderWorldLocation(bool bLeft) const;

	void AddKeyboardMappingContext();

	void OnLeftHandMove(const FInputActionValue& Value);
	void OnRightHandMove(const FInputActionValue& Value);
	void OnLeftHandMoveCompleted(const FInputActionValue& Value);
	void OnRightHandMoveCompleted(const FInputActionValue& Value);

	void OnLeftGrabStarted(const FInputActionValue& Value);
	void OnLeftGrabCompleted(const FInputActionValue& Value);
	void OnRightGrabStarted(const FInputActionValue& Value);
	void OnRightGrabCompleted(const FInputActionValue& Value);

	void DrawHandDebug() const;
};
