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

	// 左手の目標位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	UHandComponent* GetLeftHand() const;

	// 右手の目標位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	UHandComponent* GetRightHand() const;

	// 左肩のワールド位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector GetLeftShoulderWorldLocation() const;

	// 右肩のワールド位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector GetRightShoulderWorldLocation() const;

	// 左肘のターゲット位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector GetLeftJointTargetWorldLocation() const;

	// 右肘のターゲット位置を取得する
	UFUNCTION(BlueprintPure, Category = "Hand")
	FVector GetRightJointTargetWorldLocation() const;

protected:
	virtual void BeginPlay() override;

	// 左手の目標位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	TObjectPtr<UHandComponent> LeftHand;

	// 右手の目標位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand")
	TObjectPtr<UHandComponent> RightHand;

	// 左手の初期位置（カプセル基準）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FVector LeftHandInitialLocation = FVector(0.f, -30.f, 50.f);

	// 右手の初期位置（カプセル基準）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FVector RightHandInitialLocation = FVector(0.f, 30.f, 50.f);

	// 左肩のボーン名
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FName LeftShoulderBoneName = TEXT("upperarm_l");

	// 右肩のボーン名
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FName RightShoulderBoneName = TEXT("upperarm_r");

	// 左肘の向きを決めるオフセット
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FVector LeftJointTargetOffset = FVector(0.f, -40.f, -40.f);

	// 右肘の向きを決めるオフセット
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FVector RightJointTargetOffset = FVector(0.f, 40.f, -40.f);

	// 掌の法線の基準方向（キャラクターのローカル空間）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FVector PalmNormalDirection = FVector(1.f, 0.f, 0.f);

	// 左手の回転補正
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FRotator LeftHandRotationOffset = FRotator::ZeroRotator;

	// 右手の回転補正
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand")
	FRotator RightHandRotationOffset = FRotator::ZeroRotator;

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
