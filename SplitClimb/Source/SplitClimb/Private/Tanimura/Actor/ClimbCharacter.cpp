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

	// 設定アセットが無い場合はクラス既定値を使う
	const UClimbSettings* Settings = ClimbSettings.Get();
	if (!Settings) {
		Settings = GetDefault<UClimbSettings>();
	}

	// 肩ボーンのワールド位置を基準にする
	const FVector LeftShoulderLocation = GetShoulderWorldLocation(true);
	const FVector RightShoulderLocation = GetShoulderWorldLocation(false);

	LeftHand->MoveHand(LeftHand->GetMoveInput(), Settings->HandMoveSpeed, Settings->MaxHandMoveRadius, LeftShoulderLocation, DeltaTime);
	RightHand->MoveHand(RightHand->GetMoveInput(), Settings->HandMoveSpeed, Settings->MaxHandMoveRadius, RightShoulderLocation, DeltaTime);

	// 掌の向きを肩から掌への方向と壁の法線から決める
	const FVector PalmNormal = GetActorTransform().TransformVectorNoScale(PalmNormalDirection).GetSafeNormal();
	LeftHand->UpdateHandRotation(LeftShoulderLocation, PalmNormal, LeftHandRotationOffset);
	RightHand->UpdateHandRotation(RightShoulderLocation, PalmNormal, RightHandRotationOffset);
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
	// 左手の掴み状態を有効にする
	LeftHand->SetGrabHeld(true);
	UE_LOG(LogTemp, Log, TEXT("Left hand grab: pressed"));
}

void AClimbCharacter::OnLeftGrabCompleted(const FInputActionValue&)
{
	// 左手の掴み状態を解除する
	LeftHand->SetGrabHeld(false);
	UE_LOG(LogTemp, Log, TEXT("Left hand grab: released"));
}

void AClimbCharacter::OnRightGrabStarted(const FInputActionValue&)
{
	// 右手の掴み状態を有効にする
	RightHand->SetGrabHeld(true);
	UE_LOG(LogTemp, Log, TEXT("Right hand grab: pressed"));
}

void AClimbCharacter::OnRightGrabCompleted(const FInputActionValue&)
{
	// 右手の掴み状態を解除する
	RightHand->SetGrabHeld(false);
	UE_LOG(LogTemp, Log, TEXT("Right hand grab: released"));
}

void AClimbCharacter::DrawHandDebug() const
{
	// 掌が未生成なら描画しない
	if (!LeftHand || !RightHand) {
		return;
	}

	// 設定アセットが無い場合はクラス既定値を使う
	const UClimbSettings* Settings = ClimbSettings.Get();
	if (!Settings) {
		Settings = GetDefault<UClimbSettings>();
	}

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
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Cyan, FString::Printf(TEXT("L Move (%.2f, %.2f) Grab %s"), LeftHand->GetMoveInput().X, LeftHand->GetMoveInput().Y, LeftHand->IsGrabHeld() ? TEXT("ON") : TEXT("OFF")));
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Orange, FString::Printf(TEXT("R Move (%.2f, %.2f) Grab %s"), RightHand->GetMoveInput().X, RightHand->GetMoveInput().Y, RightHand->IsGrabHeld() ? TEXT("ON") : TEXT("OFF")));
	}
}
