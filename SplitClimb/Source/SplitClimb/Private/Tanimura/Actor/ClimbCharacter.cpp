#include "Tanimura/Actor/ClimbCharacter.h"
#include "Tanimura/Component/HandComponent.h"
#include "Tanimura/Data/ClimbSettings.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"

AClimbCharacter::AClimbCharacter()
{
	// 掌のデバッグ表示を毎フレーム更新する
	PrimaryActorTick.bCanEverTick = true;

	// 左手の目標位置を作る
	LeftHand = CreateDefaultSubobject<UHandComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(GetRootComponent());
	LeftHand->HandSide = EHandSide::Left;
	LeftHand->SetRelativeLocation(FVector(0.f, -30.f, 50.f));

	// 右手の目標位置を作る
	RightHand = CreateDefaultSubobject<UHandComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(GetRootComponent());
	RightHand->HandSide = EHandSide::Right;
	RightHand->SetRelativeLocation(FVector(0.f, 30.f, 50.f));
}

UHandComponent* AClimbCharacter::GetLeftHand() const
{
	return LeftHand.Get();
}

UHandComponent* AClimbCharacter::GetRightHand() const
{
	return RightHand.Get();
}

FVector AClimbCharacter::GetLeftShoulderWorldLocation() const
{
	return GetShoulderWorldLocation(true);
}

FVector AClimbCharacter::GetRightShoulderWorldLocation() const
{
	return GetShoulderWorldLocation(false);
}

FVector AClimbCharacter::GetLeftJointTargetWorldLocation() const
{
	const FVector Offset = GetActorRotation().RotateVector(LeftJointTargetOffset);
	return GetShoulderWorldLocation(true) + Offset;
}

FVector AClimbCharacter::GetRightJointTargetWorldLocation() const
{
	const FVector Offset = GetActorRotation().RotateVector(RightJointTargetOffset);
	return GetShoulderWorldLocation(false) + Offset;
}

void AClimbCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 掌の初期位置を設定する
	if (LeftHand) {
		LeftHand->SetRelativeLocation(LeftHandInitialLocation);
	}
	if (RightHand) {
		RightHand->SetRelativeLocation(RightHandInitialLocation);
	}

	// 設定アセットが無い場合はクラス既定値を使う
	if (!ClimbSettings.Get()) {
		UE_LOG(LogTemp, Warning, TEXT("ClimbSettings is not assigned. Using class defaults."));
	}
}

void AClimbCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 掴んでいる掌の入力で体を動かす
	UpdateBodyMovement(DeltaSeconds);

	// 掌の移動を更新する
	UpdateHandMovement(DeltaSeconds);

	// 掌の目標位置と入力を可視化する
	DrawHandDebug();
}

void AClimbCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// キーボード操作を有効にする
	AddKeyboardMappingContext();

	// Enhanced Input 以外では掌の入力を受け取れない
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput) {
		UE_LOG(LogTemp, Warning, TEXT("EnhancedInputComponent is not available"));
		return;
	}

	// 未設定のアクションがあると入力が欠ける
	if (!LeftHandMoveAction || !RightHandMoveAction || !LeftGrabAction || !RightGrabAction) {
		UE_LOG(LogTemp, Warning, TEXT("Input actions are not assigned"));
		return;
	}

	// 左手の移動
	EnhancedInput->BindAction(LeftHandMoveAction, ETriggerEvent::Triggered, this, &AClimbCharacter::OnLeftHandMove);
	EnhancedInput->BindAction(LeftHandMoveAction, ETriggerEvent::Completed, this, &AClimbCharacter::OnLeftHandMoveCompleted);
	EnhancedInput->BindAction(LeftHandMoveAction, ETriggerEvent::Canceled, this, &AClimbCharacter::OnLeftHandMoveCompleted);

	// 右手の移動
	EnhancedInput->BindAction(RightHandMoveAction, ETriggerEvent::Triggered, this, &AClimbCharacter::OnRightHandMove);
	EnhancedInput->BindAction(RightHandMoveAction, ETriggerEvent::Completed, this, &AClimbCharacter::OnRightHandMoveCompleted);
	EnhancedInput->BindAction(RightHandMoveAction, ETriggerEvent::Canceled, this, &AClimbCharacter::OnRightHandMoveCompleted);

	// 左手の掴み
	EnhancedInput->BindAction(LeftGrabAction, ETriggerEvent::Started, this, &AClimbCharacter::OnLeftGrabStarted);
	EnhancedInput->BindAction(LeftGrabAction, ETriggerEvent::Completed, this, &AClimbCharacter::OnLeftGrabCompleted);
	EnhancedInput->BindAction(LeftGrabAction, ETriggerEvent::Canceled, this, &AClimbCharacter::OnLeftGrabCompleted);

	// 右手の掴み
	EnhancedInput->BindAction(RightGrabAction, ETriggerEvent::Started, this, &AClimbCharacter::OnRightGrabStarted);
	EnhancedInput->BindAction(RightGrabAction, ETriggerEvent::Completed, this, &AClimbCharacter::OnRightGrabCompleted);
	EnhancedInput->BindAction(RightGrabAction, ETriggerEvent::Canceled, this, &AClimbCharacter::OnRightGrabCompleted);
}

void AClimbCharacter::UpdateHandMovement(float DeltaTime)
{
	if (!LeftHand || !RightHand) {
		return;
	}

	// 壁登りの設定値を取得する
	const UClimbSettings* Settings = GetClimbSettings();

	// 肩ボーンのワールド位置を基準にする
	const FVector LeftShoulderLocation = GetShoulderWorldLocation(true);
	const FVector RightShoulderLocation = GetShoulderWorldLocation(false);

	LeftHand->MoveHand(LeftHand->GetMoveInput(), Settings->HandMoveSpeed, Settings->MaxHandMoveRadius, LeftShoulderLocation, DeltaTime);
	RightHand->MoveHand(RightHand->GetMoveInput(), Settings->HandMoveSpeed, Settings->MaxHandMoveRadius, RightShoulderLocation, DeltaTime);

	// 掌の向きを肩から掌への方向と壁の法線から決める
	// 掴んでいない掌は体の向きを基準にする
	FVector LeftPalmNormal = GetPalmNormalWorld();
	FVector RightPalmNormal = GetPalmNormalWorld();

	// 掴んでいる掌は掴んだ面へ掌を向ける
	if (LeftHand->IsGrabbing()) {
		LeftPalmNormal = -LeftHand->GetGrabNormal();
	}
	if (RightHand->IsGrabbing()) {
		RightPalmNormal = -RightHand->GetGrabNormal();
	}

	LeftHand->UpdateHandRotation(LeftShoulderLocation, LeftPalmNormal, LeftHandRotationOffset);
	RightHand->UpdateHandRotation(RightShoulderLocation, RightPalmNormal, RightHandRotationOffset);
}

FVector AClimbCharacter::GetShoulderWorldLocation(bool bLeft) const
{
	const UHandComponent* Hand = bLeft ? LeftHand.Get() : RightHand.Get();
	const FName BoneName = bLeft ? LeftShoulderBoneName : RightShoulderBoneName;

	// メッシュまたはボーンが無い場合は掌の位置を基準にする
	if (!Hand || !GetMesh() || GetMesh()->GetBoneIndex(BoneName) == INDEX_NONE) {
		return Hand ? Hand->GetComponentLocation() : GetActorLocation();
	}

	return GetMesh()->GetSocketLocation(BoneName);
}

void AClimbCharacter::AddKeyboardMappingContext()
{
	// 未設定なら掌の操作を登録できない
	if (!KeyboardMappingContext) {
		UE_LOG(LogTemp, Warning, TEXT("KeyboardMappingContext is not assigned"));
		return;
	}

	// ローカルプレイヤーを持たない場合は入力サブシステムを取得できない
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) {
		return;
	}

	// ローカルプレイヤー不在時は入力サブシステムを取得できない
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) {
		return;
	}

	// Enhanced Input が無効な場合は登録できない
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!InputSubsystem) {
		return;
	}

	// テンプレートの入力を外し、掌操作だけを有効にする
	InputSubsystem->ClearAllMappings();
	InputSubsystem->AddMappingContext(KeyboardMappingContext, 0);
}

void AClimbCharacter::OnLeftHandMove(const FInputActionValue& Value)
{
	// 2D軸の入力を取得する
	FVector2D MoveValue = Value.Get<FVector2D>();

	// 斜め入力の大きさを1に揃える
	if (MoveValue.SizeSquared() > 1.f) {
		MoveValue.Normalize();
	}

	// 左手の入力を更新する
	LeftHand->SetMoveInput(MoveValue);
}

void AClimbCharacter::OnRightHandMove(const FInputActionValue& Value)
{
	// 2D軸の入力を取得する
	FVector2D MoveValue = Value.Get<FVector2D>();

	// 斜め入力の大きさを1に揃える
	if (MoveValue.SizeSquared() > 1.f) {
		MoveValue.Normalize();
	}

	// 右手の入力を更新する
	RightHand->SetMoveInput(MoveValue);
}

void AClimbCharacter::OnLeftHandMoveCompleted(const FInputActionValue&)
{
	// 入力終了時に値を0へ戻す
	LeftHand->SetMoveInput(FVector2D::ZeroVector);
}

void AClimbCharacter::OnRightHandMoveCompleted(const FInputActionValue&)
{
	// 入力終了時に値を0へ戻す
	RightHand->SetMoveInput(FVector2D::ZeroVector);
}

void AClimbCharacter::OnLeftGrabStarted(const FInputActionValue&)
{
	// 左手で掴みを試みる
	HandleGrabStarted(LeftHand);
}

void AClimbCharacter::OnLeftGrabCompleted(const FInputActionValue&)
{
	// 左手の掴みを解除する
	HandleGrabCompleted(LeftHand);
}

void AClimbCharacter::OnRightGrabStarted(const FInputActionValue&)
{
	// 右手で掴みを試みる
	HandleGrabStarted(RightHand);
}

void AClimbCharacter::OnRightGrabCompleted(const FInputActionValue&)
{
	// 右手の掴みを解除する
	HandleGrabCompleted(RightHand);
}

void AClimbCharacter::HandleGrabStarted(UHandComponent* Hand)
{
	if (!Hand) {
		return;
	}

	// ボタンを押している間であることを記録する
	Hand->SetGrabHeld(true);

	const UClimbSettings* Settings = GetClimbSettings();

	// 掌の位置から掴める面を探す
	const bool bGrabbed = Hand->TryGrab(GetPalmNormalWorld(), Settings->GrabReach, Settings->GrabSurfaceOffset, Settings->GrabTraceChannel);

	// 掴めた場合だけ体の保持へ切り替える
	if (bGrabbed) {
		FallSpeed = 0.f;
		UpdateMovementMode();
	}

	UE_LOG(LogTemp, Log, TEXT("%s hand grab: %s"), Hand == LeftHand.Get() ? TEXT("Left") : TEXT("Right"), bGrabbed ? TEXT("grabbed") : TEXT("missed"));
}

void AClimbCharacter::HandleGrabCompleted(UHandComponent* Hand)
{
	if (!Hand) {
		return;
	}

	Hand->SetGrabHeld(false);

	// 掴んでいた場合は掴みを解除する
	if (Hand->IsGrabbing()) {
		Hand->ReleaseGrab();
		UpdateMovementMode();
	}

	UE_LOG(LogTemp, Log, TEXT("%s hand grab: released"), Hand == LeftHand.Get() ? TEXT("Left") : TEXT("Right"));
}

bool AClimbCharacter::HasGrabbingHand() const
{
	return (LeftHand && LeftHand->IsGrabbing()) || (RightHand && RightHand->IsGrabbing());
}

void AClimbCharacter::UpdateBodyMovement(float DeltaTime)
{
	const UClimbSettings* Settings = GetClimbSettings();

	// 掴んでいない間は重力に任せる
	if (!HasGrabbingHand()) {
		FallSpeed = 0.f;
		return;
	}

	const FRotator BodyRotation = GetActorRotation();

	// 掴んでいる掌の入力から体に加わる力を合成する
	FVector PullDirection = FVector::ZeroVector;
	UHandComponent* Hands[] = { LeftHand.Get(), RightHand.Get() };
	for (UHandComponent* Hand : Hands) {
		if (!Hand || !Hand->IsGrabbing() || !Hand->IsGrabHeld()) {
			continue;
		}

		// スティック入力と逆向きのベクトルを加える
		const FVector2D Input = Hand->GetMoveInput();
		PullDirection += BodyRotation.RotateVector(FVector(0.f, -Input.X, -Input.Y));
	}

	// 合成した力の大きさを1に収める
	if (PullDirection.SizeSquared() > 1.f) {
		PullDirection.Normalize();
	}

	// 引き上げ中は手が重力に抗い、それ以外は重力で落ちる
	const bool bPullingUp = PullDirection.Z > KINDA_SMALL_NUMBER;
	if (bPullingUp) {
		FallSpeed = 0.f;
	} else {
		FallSpeed = FMath::Min(FallSpeed + GetGravityAcceleration() * DeltaTime, Settings->MaxFallSpeed);
	}

	// スティックの引きと重力を合成して体を動かす
	FVector Delta = PullDirection * Settings->PullSpeed * DeltaTime;
	Delta.Z -= FallSpeed * DeltaTime;

	FHitResult Hit;
	if (!Delta.IsNearlyZero()) {
		AddActorWorldOffset(Delta, true, &Hit, ETeleportType::None);

		// 床に阻まれた場合は落下速度を止める
		if (Hit.bBlockingHit && Delta.Z < 0.f) {
			FallSpeed = 0.f;
		}
	}

	// 掴んだ掌が体を支えた場合は落下速度を止める
	if (SolveGrabConstraints(Settings->MaxHandBodyDistance)) {
		FallSpeed = 0.f;
	}
}

bool AClimbCharacter::SolveGrabConstraints(float MaxDistance)
{
	bool bSupported = false;

	// 腕の長さの制約を複数回の反復で満たす
	for (int32 Iteration = 0; Iteration < 3; ++Iteration) {
		bool bCorrected = false;
		UHandComponent* Hands[] = { LeftHand.Get(), RightHand.Get() };

		for (UHandComponent* Hand : Hands) {
			if (!Hand || !Hand->IsGrabbing()) {
				continue;
			}

			const FVector ShoulderLocation = GetShoulderWorldLocation(Hand == LeftHand.Get());
			const FVector ArmVector = ShoulderLocation - Hand->GetGrabLocation();
			const FVector GrabNormal = Hand->GetGrabNormal();

			// 面の法線方向は壁に阻まれるため、面に沿った距離だけで制約する
			const double NormalDistance = FVector::DotProduct(ArmVector, GrabNormal);
			const FVector PlanarOffset = ArmVector - GrabNormal * NormalDistance;
			const double PlanarDistance = PlanarOffset.Size();
			const double MaxDistanceSquared = static_cast<double>(MaxDistance) * MaxDistance;
			const double PlanarMax = FMath::Sqrt(FMath::Max(0.0, MaxDistanceSquared - NormalDistance * NormalDistance));

			// 腕の長さを超えた分だけ体を掌へ引き戻す
			if (PlanarDistance > PlanarMax && PlanarDistance > KINDA_SMALL_NUMBER) {
				AddActorWorldOffset(PlanarOffset / PlanarDistance * (PlanarMax - PlanarDistance), true, nullptr, ETeleportType::None);
				bCorrected = true;
				bSupported = true;
			}
		}

		// 補正が無ければ反復を打ち切る
		if (!bCorrected) {
			break;
		}
	}

	return bSupported;
}

void AClimbCharacter::UpdateMovementMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) {
		return;
	}

	if (HasGrabbingHand()) {
		// 掴んでいる間は手動の重力で動かす
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);
	} else {
		// 全ての手を離したら重力で落下させる
		Movement->SetMovementMode(MOVE_Falling);
		Movement->Velocity.Z = -FallSpeed;
		FallSpeed = 0.f;
	}
}

FVector AClimbCharacter::GetPalmNormalWorld() const
{
	return GetActorTransform().TransformVectorNoScale(PalmNormalDirection).GetSafeNormal();
}

const UClimbSettings* AClimbCharacter::GetClimbSettings() const
{
	const UClimbSettings* Settings = ClimbSettings.Get();
	if (!Settings) {
		Settings = GetDefault<UClimbSettings>();
	}
	return Settings;
}

float AClimbCharacter::GetGravityAcceleration() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();

	// 移動コンポーネントが無い場合は既定の重力を使う
	if (!Movement) {
		return 980.f;
	}

	return FMath::Abs(Movement->GetGravityZ());
}

void AClimbCharacter::DrawHandDebug() const
{
	// 掌が未生成なら描画しない
	if (!LeftHand || !RightHand) {
		return;
	}

	// 壁登りの設定値を取得する
	const UClimbSettings* Settings = GetClimbSettings();

	// 壁面内の可動範囲を表示する
	const float MaxRadius = Settings->MaxHandMoveRadius;
	const FVector LeftShoulderLocation = GetShoulderWorldLocation(true);
	const FVector RightShoulderLocation = GetShoulderWorldLocation(false);
	const FVector LeftCenter(LeftHand->GetComponentLocation().X, LeftShoulderLocation.Y, LeftShoulderLocation.Z);
	const FVector RightCenter(RightHand->GetComponentLocation().X, RightShoulderLocation.Y, RightShoulderLocation.Z);
	DrawDebugCircle(GetWorld(), LeftCenter, MaxRadius, 32, FColor::Cyan, false, -1.f, SDPG_World, 0.5f, FVector::RightVector, FVector::UpVector, false);
	DrawDebugCircle(GetWorld(), RightCenter, MaxRadius, 32, FColor::Orange, false, -1.f, SDPG_World, 0.5f, FVector::RightVector, FVector::UpVector, false);

	// 掌の目標位置を球で表示する
	DrawDebugSphere(GetWorld(), LeftHand->GetComponentLocation(), 6.f, 12, FColor::Cyan, false, -1.f, SDPG_Foreground, 1.f);
	DrawDebugSphere(GetWorld(), RightHand->GetComponentLocation(), 6.f, 12, FColor::Orange, false, -1.f, SDPG_Foreground, 1.f);

	// 入力値と掴み状態を画面に表示する
	if (GEngine) {
		// 掴み状態と肩から掌までの距離を表示する
		const float LeftArmDistance = LeftHand->IsGrabbing() ? (LeftShoulderLocation - LeftHand->GetGrabLocation()).Size() : 0.f;
		const float RightArmDistance = RightHand->IsGrabbing() ? (RightShoulderLocation - RightHand->GetGrabLocation()).Size() : 0.f;
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Cyan, FString::Printf(TEXT("L Move (%.2f, %.2f) Held %s Grab %s Arm %.0f"), LeftHand->GetMoveInput().X, LeftHand->GetMoveInput().Y, LeftHand->IsGrabHeld() ? TEXT("ON") : TEXT("OFF"), LeftHand->IsGrabbing() ? TEXT("ON") : TEXT("OFF"), LeftArmDistance));
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Orange, FString::Printf(TEXT("R Move (%.2f, %.2f) Held %s Grab %s Arm %.0f"), RightHand->GetMoveInput().X, RightHand->GetMoveInput().Y, RightHand->IsGrabHeld() ? TEXT("ON") : TEXT("OFF"), RightHand->IsGrabbing() ? TEXT("ON") : TEXT("OFF"), RightArmDistance));
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::White, FString::Printf(TEXT("FallSpeed %.0f"), FallSpeed));
	}
}
