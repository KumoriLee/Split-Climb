#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "ClimbSettings.generated.h"

/**
 * 壁登りの調整値をまとめる
 */

UCLASS(BlueprintType)
class SPLITCLIMB_API UClimbSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 掌の移動速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand", meta = (ClampMin = "0.0"))
	float HandMoveSpeed = 300.f;

	// 壁面内の可動半径
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hand", meta = (ClampMin = "0.0"))
	float MaxHandMoveRadius = 75.f;

	// 腕ごとの最大体力
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.f;

	// 片手で掴んでいる間の体力減少量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaDrainPerSecond = 10.f;

	// 手を離している間の体力回復量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaRecoveryPerSecond = 20.f;

	// 引き上げ移動の速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float PullSpeed = 200.f;

	// 落下距離
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fall", meta = (ClampMin = "0.0"))
	float FallDistance = 200.f;

	// 掴み判定に使うトレースチャンネル
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab")
	TEnumAsByte<ECollisionChannel> GrabTraceChannel = ECC_GameTraceChannel1;

	// 掴み判定を探す距離
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab", meta = (ClampMin = "0.0"))
	float GrabReach = 120.f;

	// 掴んだ掌を面から浮かせる距離
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab", meta = (ClampMin = "0.0"))
	float GrabSurfaceOffset = 2.f;

	// 体と掌の最大距離
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float MaxHandBodyDistance = 90.f;

	// 掴んでいる間の落下速度の上限
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fall", meta = (ClampMin = "0.0"))
	float MaxFallSpeed = 500.f;
};
