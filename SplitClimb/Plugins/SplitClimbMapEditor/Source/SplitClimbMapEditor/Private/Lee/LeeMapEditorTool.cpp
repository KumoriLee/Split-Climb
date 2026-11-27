#include "Lee/LeeMapEditorTool.h"

#include "Lee/LeeMapEditorTempWorld.h"

#include "CanvasItem.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/Blueprint.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/Level.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "LevelEditorViewport.h"
#include "PrimitiveDrawingUtils.h"
#include "ScopedTransaction.h"
#include "SceneView.h"
#include "Styling/CoreStyle.h"
#include "WorldCollision.h"

#define LEE_TRACE_RANGE (1.0e8)
#define LEE_ORTHO_BACKOFF (1.0e8)

namespace
{
	/** @brief ツール内部共通のトレース用クエリパラメータを生成する。 */
	FCollisionQueryParams MakeTraceParams(bool bTraceComplex)
	{
		return FCollisionQueryParams(TEXT("LeeSplitClimbMapEditorTrace"), bTraceComplex);
	}

	/** @brief 強弱タグの表示名を返す。 */
	const TCHAR* HoldTagDisplayName(LeeMapEditor::ELeeHoldType InType)
	{
		return InType == LeeMapEditor::ELeeHoldType::Strong ? TEXT("GripStrong") : TEXT("GripWeak");
	}

	/** @brief 衝突応答の表示名を返す。 */
	const TCHAR* CollisionResponseName(ECollisionResponse InResponse)
	{
		switch (InResponse)
		{
			case ECR_Block: return TEXT("Block");
			case ECR_Overlap: return TEXT("Overlap");
			case ECR_Ignore: return TEXT("Ignore");
			default: return TEXT("Default");
		}
	}

	// 2026.11.25 Lee start
	// 2026.11.27 李奕勋 start
	/**
	 * @brief 朝向ロック（世界 ±X への吸附・30° 適用拦截・パネル表示）の撤回に伴い、
	 *        下記の主攀爬面チェックは参照元が無くなったためコメント化して保持する。
	 *        元のコードは消さない。
	 */
	/** @brief 主攀爬面の法線に許容する ±X からの最大角度（度）。境界値を含む（閾値の単一ソース）。 */
	//constexpr double MainFaceMaxAngleDeg = 30.0;

	///**
	// * @brief 主攀爬面が世界 YZ 平面に平行かを判定する（法線が ±X の許容角内）。
	// *        制約の対象は局所的な落点・盒体の朝向（アンカー法線＝盒体ローカル Y の向き）であり、
	// *        岩壁全体の設置が正しいことを保証するものではない。
	// *        非有限法線は ContainsNaN() で先に拒否し、安全な単位法線の |X| と
	// *        cos(許容角) を比較する。+X と -X の両方向を許し、ちょうど許容角の境界値も合格とする。
	// * @param InNormal 面法線（ワールド。FLeeSurfaceAnchor::Normal を想定）。
	// * @return 法線が ±X となす角が許容角以下なら true。
	// */
	//bool IsMainFaceWithinTolerance(const FVector& InNormal)
	//{
	//	if (InNormal.ContainsNaN())
	//	{
	//		return false;
	//	}
	//	const FVector SafeNormal = InNormal.GetSafeNormal();
	//	if (SafeNormal.IsNearlyZero())
	//	{
	//		// 無効な法線（ゼロ近傍）は拒否する
	//		return false;
	//	}
	//	return FMath::Abs(SafeNormal.X) >= FMath::Cos(FMath::DegreesToRadians(MainFaceMaxAngleDeg));
	//}

	///**
	// * @brief 主攀爬面チェックの統一不合格文を生成する（エディタツール文字は中国語）。
	// *        適用拒否メッセージとパネル状態表示で共用し、閾値は MainFaceMaxAngleDeg の単一ソース。
	// * @return 「抓点朝向偏离世界 ±X 超过 30°…」形式の説明文。
	// */
	//FString MainFaceCheckRejectText()
	//{
	//	return FString::Printf(TEXT("抓点朝向偏离世界 ±X 超过 %.0f°。请将主攀爬面对齐世界 YZ 平面，或重新选面。"), MainFaceMaxAngleDeg);
	//}
	// 2026.11.27 李奕勋 end
	// 2026.11.25 Lee end
}

FLeeMapEditorTool::FLeeMapEditorTool()
	: FGCObject()
	, TSharedFromThis<FLeeMapEditorTool>()
{
	// 切図・PIE の開始/終了で古い参照を破棄する（レベルをまたぐぶら下がりポインタ防止）
	MapChangeHandle = FEditorDelegates::MapChange.AddRaw(this, &FLeeMapEditorTool::HandleMapChanged);
	PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddRaw(this, &FLeeMapEditorTool::HandlePreBeginPIE);
	PrePIEEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FLeeMapEditorTool::HandlePrePIEEnded);
}

FLeeMapEditorTool::~FLeeMapEditorTool()
{
	FEditorDelegates::MapChange.Remove(MapChangeHandle);
	FEditorDelegates::PreBeginPIE.Remove(PreBeginPIEHandle);
	FEditorDelegates::PrePIEEnded.Remove(PrePIEEndedHandle);
}

void FLeeMapEditorTool::HandleMapChanged(uint32 InFlags)
{
	InvalidateWorldReferences();
}

void FLeeMapEditorTool::HandlePreBeginPIE(bool bInIsSimulating)
{
	InvalidateWorldReferences();
}

void FLeeMapEditorTool::HandlePrePIEEnded(bool bInIsSimulating)
{
	InvalidateWorldReferences();
}

void FLeeMapEditorTool::InvalidateWorldReferences()
{
	CancelPreview();
	SpecifiedRefs.Reset();
	AttachOverride = nullptr;
	LastCheck = LeeMapEditor::FLeeCheckResult();
	NotifyChanged();
}

void FLeeMapEditorTool::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(HoldBlueprint);
	Collector.AddReferencedObject(RockMesh);
}

void FLeeMapEditorTool::OnModeEntered()
{
	bModeActive = true;
	NotifyChanged();
}

void FLeeMapEditorTool::OnModeExited()
{
	bModeActive = false;
	bDragging = false;
	PickMode = LeeMapEditor::ELeePickMode::None;
	CancelPreview();
	NotifyChanged();
}

void FLeeMapEditorTool::SetArmed(bool bArmed)
{
	if (bArmed)
	{
		GLevelEditorModeTools().ActivateMode(LeeMapEditor::ModeID);
	}
	else
	{
		GLevelEditorModeTools().DeactivateMode(LeeMapEditor::ModeID);
	}
}

bool FLeeMapEditorTool::IsEngaged() const
{
	return bModeActive && (bPreviewActive || bDragging || PickMode != LeeMapEditor::ELeePickMode::None);
}

void FLeeMapEditorTool::SetPlacementKind(LeeMapEditor::ELeePlacementKind InKind)
{
	if (PlacementKind == InKind)
	{
		return;
	}
	PlacementKind = InKind;
	CancelPreview();
	NotifyChanged();
}

void FLeeMapEditorTool::SetHoldType(LeeMapEditor::ELeeHoldType InType)
{
	// 強弱切替時も現在の寸法は保持する（既定寸法への復帰は RestorePresetSize で行う）
	HoldType = InType;
	LastCheck.bRan = false;
	NotifyChanged();
}

void FLeeMapEditorTool::RestorePresetSize()
{
	Size = (HoldType == LeeMapEditor::ELeeHoldType::Strong)
		? LeeMapEditor::StrongPresetSize
		: LeeMapEditor::WeakPresetSize;
	NotifyChanged();
}

void FLeeMapEditorTool::SetStepCm(double InStep)
{
	StepCm = FMath::Clamp(InStep, 0.1, 100.0);
	NotifyChanged();
}

void FLeeMapEditorTool::SetOffsetU(double InValue)
{
	OffsetU = InValue;
	ValidatePreviewAfterParameterChange();
}

void FLeeMapEditorTool::SetOffsetV(double InValue)
{
	OffsetV = InValue;
	ValidatePreviewAfterParameterChange();
}

bool FLeeMapEditorTool::ProjectSurfacePoint(const FVector& InTangentPoint, FVector& OutPoint)
{
	UWorld* World = GetWorldSafe();
	if (!World || !Anchor.bValid || !Anchor.Component.IsValid())
	{
		return false;
	}

	// 接線移動後の点を少し面の外へ持ち上げ、-法線方向へ同一コンポーネントに再投影する
	UPrimitiveComponent* AnchorComp = Anchor.Component.Get();
	const FVector Lifted = InTangentPoint + Anchor.Normal * 5.0;
	const FVector LiftUp = Anchor.Normal * FMath::Max(GetFocusSize().Y * 0.5, 50.0);

	FCollisionQueryParams Params = MakeTraceParams(/*bTraceComplex*/ true);
	if (AActor* EditActor = EditTarget.Get())
	{
		Params.AddIgnoredActor(EditActor);
	}
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType(Hit, Lifted + LiftUp, Lifted - LiftUp * 2.0, ObjectParams, Params))
	{
		return false;
	}
	if (Hit.Component != AnchorComp)
	{
		// 範囲外（同一コンポーネント外へ滑り落ちた）：保持する
		return false;
	}
	OutPoint = Hit.ImpactPoint;
	return true;
}

void FLeeMapEditorTool::ValidatePreviewAfterParameterChange()
{
	LastCheck.bRan = false;

	if (!bPreviewActive)
	{
		NotifyChanged();
		return;
	}

	// 数値 UV オフセットは同一コンポーネント上への再レイ投影で検証し、範囲外なら直前の有効値へ戻す
	const double AngleRad = FMath::DegreesToRadians(AngleDeg);
	const FVector AxisU2 = Anchor.AxisU * FMath::Cos(AngleRad) - Anchor.AxisV * FMath::Sin(AngleRad);
	const FVector AxisV2 = Anchor.AxisU * FMath::Sin(AngleRad) + Anchor.AxisV * FMath::Cos(AngleRad);
	const FVector TangentPoint = Anchor.Point + AxisU2 * OffsetU + AxisV2 * OffsetV;

	FVector ProjectedPoint;
	if (ProjectSurfacePoint(TangentPoint, ProjectedPoint))
	{
		ValidatedSurfacePoint = ProjectedPoint;
		ValidOffsetU = OffsetU;
		ValidOffsetV = OffsetV;
	}
	else
	{
		// 範囲外：プレビューを保持し、数値を最後の有効値へ戻す
		OffsetU = ValidOffsetU;
		OffsetV = ValidOffsetV;
	}
	NotifyChanged();
}

void FLeeMapEditorTool::SetAngleDeg(double InValue)
{
	AngleDeg = FMath::UnwindDegrees(InValue);
	LastCheck.bRan = false;
	NotifyChanged();
}

void FLeeMapEditorTool::SetOffsetN(double InValue)
{
	OffsetN = InValue;
	LastCheck.bRan = false;
	NotifyChanged();
}

void FLeeMapEditorTool::SetSize(const FVector& InSize)
{
	Size = FVector(
		FMath::Max(InSize.X, 1.0),
		FMath::Max(InSize.Y, 1.0),
		FMath::Max(InSize.Z, 1.0));
	LastCheck.bRan = false;
	NotifyChanged();
}

void FLeeMapEditorTool::SetSizeX(double InValue)
{
	FVector S = Size;
	S.X = FMath::Max(InValue, 1.0);
	SetSize(S);
}

void FLeeMapEditorTool::SetSizeY(double InValue)
{
	FVector S = Size;
	S.Y = FMath::Max(InValue, 1.0);
	SetSize(S);
}

void FLeeMapEditorTool::SetSizeZ(double InValue)
{
	FVector S = Size;
	S.Z = FMath::Max(InValue, 1.0);
	SetSize(S);
}

const TArray<FLeeRockAssetEntry>& FLeeMapEditorTool::GetRockEntries()
{
	// 既存 LP_RocksandCliffs メッシュを実在アセット（ディスク確認済み）に対応付ける。
	static const TArray<FLeeRockAssetEntry> Entries = {
		{ NSLOCTEXT("SplitClimbMapEditor", "RockCategoryCliffWall", "崖壁"), TEXT("/Game/LP_RocksandCliffs/Meshes/sm_cliff_11.sm_cliff_11"), TEXT("sm_cliff_11"), false },
		{ NSLOCTEXT("SplitClimbMapEditor", "RockCategoryPlatform", "平台"), TEXT("/Game/LP_RocksandCliffs/Meshes/sm_rock_flat_01.sm_rock_flat_01"), TEXT("sm_rock_flat_01"), false },
		{ NSLOCTEXT("SplitClimbMapEditor", "RockCategorySmallRock", "小岩"), TEXT("/Game/LP_RocksandCliffs/Meshes/sm_rock_small_02.sm_rock_small_02"), TEXT("sm_rock_small_02"), false },
		{ NSLOCTEXT("SplitClimbMapEditor", "RockCategoryRockSlab", "岩板"), TEXT("/Game/LP_RocksandCliffs/Meshes/sm_rock_slab.sm_rock_slab"), TEXT("sm_rock_slab"), false }
	};
	return Entries;
}

void FLeeMapEditorTool::SetRockMesh(const FLeeRockAssetEntry& Entry)
{
	UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, *Entry.AssetPath);
	RockMesh = LoadedMesh;

	const TArray<FLeeRockAssetEntry>& Entries = GetRockEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].AssetPath == Entry.AssetPath)
		{
			RockEntryIndex = Index;
			break;
		}
	}
	NotifyChanged();
}

UStaticMesh* FLeeMapEditorTool::GetRockMesh() const
{
	return RockMesh;
}

bool FLeeMapEditorTool::HandleViewportClick(FEditorViewportClient* InViewportClient, HHitProxy* InHitProxy, const FViewportClick& InClick)
{
	// 押下/解放の両方で HandleClick が呼ばれる 5.9 の二重派发に対し、
	// 実処理はすべて Mode::InputKey → ExecuteViewportPlacementRay 経由で行うため、
	// ここではモード有効時の当該交互を遮蔽するだけ（再度のレイ検出は行わない）。
	return bModeActive;
}

bool FLeeMapEditorTool::ExecuteViewportPlacementRay(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (!bModeActive || InViewportClient == nullptr || InViewport == nullptr)
	{
		return false;
	}

	const bool bPicking = (PickMode != LeeMapEditor::ELeePickMode::None);

	if (bPicking)
	{
		if (PickMode == LeeMapEditor::ELeePickMode::Rock)
		{
			// 貼り付け先岩体の明示指定：面トレース（SurfaceTrace）で岩面を拾う
			FHitResult SurfHit;
			const bool bHit = TraceFromScreen(InViewportClient, InViewport, InMouseX, InMouseY, /*bPickHolds*/ false, SurfHit);
			UPrimitiveComponent* HitComp = (bHit && SurfHit.Component.IsValid()) ? SurfHit.Component.Get() : nullptr;
			if (HitComp)
			{
				AttachOverride = HitComp;
				if (AActor* OwnerActor = HitComp->GetOwner())
				{
					GEditor->SelectActor(OwnerActor, true, true);
				}
				PickMode = LeeMapEditor::ELeePickMode::None;
				if (bPreviewActive)
				{
					LastCheck.bRan = false;
					RefreshMeasurements();
				}
			}
			NotifyChanged();
			return true;
		}

		// 抓点・参照の拾取：Climbable チャンネルでレイキャストし、真の抓点にフィルタする
		FHitResult PickHit;
		const bool bHit = TraceFromScreen(InViewportClient, InViewport, InMouseX, InMouseY, /*bPickHolds*/ true, PickHit);
		AActor* HitActor = bHit ? PickHit.GetActor() : nullptr;
		if (HitActor && IsHoldActor(HitActor))
		{
			if (PickMode == LeeMapEditor::ELeePickMode::Hold)
			{
				StartEditHold(HitActor);
			}
			else // Reference
			{
				SpecifiedRefs.AddUnique(HitActor);
				RefMode = LeeMapEditor::ELeeRefMode::Specified;
				RefreshMeasurements();
			}
			PickMode = LeeMapEditor::ELeePickMode::None;
		}
		NotifyChanged();
		return true;
	}

	// 配置フロー：左ボタン押下でプレビュー開始（既存プレビュー時は面の張り直し）
	FHitResult Hit;
	const bool bHit = TraceFromScreen(InViewportClient, InViewport, InMouseX, InMouseY, /*bPickHolds*/ false, Hit);
	if (bHit && Hit.Component.IsValid())
	{
		// 明示指定した貼り付け先がある場合は、そのコンポーネント以外の表面クリックを拒否する
		UPrimitiveComponent* AttachTarget = AttachOverride.IsValid() ? AttachOverride.Get() : nullptr;
		if (!AttachTarget || Hit.Component.Get() == AttachTarget)
		{
			BeginPreviewFromHit(Hit);
			bDragging = true;
		}
	}
	// 空振り・拒否の場合は既存プレビューを保持する（端部で最後の有効プレビューを維持）
	NotifyChanged();
	return true;
}

bool FLeeMapEditorTool::HandleMouseButtonRelease(FKey InKey)
{
	if (InKey == EKeys::LeftMouseButton && bDragging)
	{
		bDragging = false;
		return true;
	}
	return false;
}

bool FLeeMapEditorTool::HandleEscape()
{
	if (bPreviewActive || bDragging || PickMode != LeeMapEditor::ELeePickMode::None)
	{
		CancelPreview();
		return true;
	}
	return false;
}

bool FLeeMapEditorTool::HandleCapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (!bModeActive || !bDragging || !bPreviewActive)
	{
		return false;
	}

	// 2026.11.25 Lee start
	// 抓点モード時のみ：命中してアンカーを更新した後に合否を比較し、変化した時だけパネルへ通知する
	// （毎フレームのブロードキャスト回避。岩体モードでは比較も通知も行わない）
	// 2026.11.27 李奕勋 start
	/**
	 * @brief 朝向ロック撤回に伴い、ドラッグ中の主攀爬面チェック合否の比較と通知を止めた。
	 *        元のコードは消さない。
	 */
	//const bool bHoldPlacement = (PlacementKind == LeeMapEditor::ELeePlacementKind::Hold);
	//bool bFaceCheckPassedBefore = false;
	//if (bHoldPlacement)
	//{
	//	bFaceCheckPassedBefore = IsMainFaceWithinTolerance(Anchor.Normal);
	//}
	// 2026.11.27 李奕勋 end
	// 2026.11.25 Lee end
	FHitResult Hit;
	if (TraceFromScreen(InViewportClient, InViewport, InMouseX, InMouseY, /*bPickHolds*/ false, Hit)
		&& Hit.Component == Anchor.Component)
	{
		// 同一コンポーネント上のみ追従する。面法線はヒットごとに更新する
		Anchor.Point = Hit.ImpactPoint;
		Anchor.Normal = Hit.ImpactNormal.GetSafeNormal();
		// 2026.11.27 李奕勋 start
		// ドラッグ再貼り付け：現在の配置種別を渡し、抓点モードでは近水平面でも右手系の面内基底を使う
		//Anchor.RecomputeAxes();←元のコードは消さない
		Anchor.RecomputeAxes(PlacementKind == LeeMapEditor::ELeePlacementKind::Hold);
		// 2026.11.27 李奕勋 end
		LastCheck.bRan = false;
		RefreshMeasurements();
		// 2026.11.25 Lee start
		// 2026.11.27 李奕勋 start
		// ドラッグ中の合否比較による通知は撤回。元のコードは消さない
		//if (bHoldPlacement && IsMainFaceWithinTolerance(Anchor.Normal) != bFaceCheckPassedBefore)
		//{
		//	NotifyChanged();
		//}
		// 2026.11.27 李奕勋 end
		// 2026.11.25 Lee end
	}
	// 空振り・別コンポーネントの場合は最後の有効なプレビューを保持する
	return true;
}

void FLeeMapEditorTool::Tick()
{
	RefreshMeasurements();
}

void FLeeMapEditorTool::Render(const FSceneView* InView, FPrimitiveDrawInterface* PDI)
{
	if (PDI == nullptr)
	{
		return;
	}

	FTransform PreviewTransform;
	if (!ComputePreviewTransform(PreviewTransform))
	{
		return;
	}

	const FQuat Rotation = PreviewTransform.GetRotation();
	const FVector Center = PreviewTransform.GetTranslation();
	const FVector AxisX = Rotation.GetAxisX();
	const FVector AxisY = Rotation.GetAxisY();
	const FVector AxisZ = Rotation.GetAxisZ();
	const FVector HalfExtent = GetFocusSize() * 0.5;

	// 盒体プレビュー（前面表示）
	const FLinearColor BoxColor = (PlacementKind == LeeMapEditor::ELeePlacementKind::Rock)
		? FLinearColor(0.30f, 1.0f, 0.40f)
		: ((HoldType == LeeMapEditor::ELeeHoldType::Strong)
			? FLinearColor(1.0f, 0.80f, 0.10f)
			: FLinearColor(0.20f, 0.65f, 1.0f));
	DrawOrientedWireBox(PDI, Center, AxisX, AxisY, AxisZ, HalfExtent, BoxColor, SDPG_Foreground, 2.0f);

	// 面法線矢印
	if (Anchor.bValid)
	{
		const double ArrowLength = FMath::Max(HalfExtent.Y + 40.0, 60.0);
		const FMatrix ArrowToWorld(AxisY, Anchor.AxisU, -Anchor.AxisV, Anchor.Point);
		DrawDirectionalArrow(PDI, ArrowToWorld, FLinearColor(1.0f, 0.4f, 0.1f), static_cast<float>(ArrowLength), 8.0f, SDPG_Foreground, 1.5f);
		PDI->DrawPoint(Anchor.Point, FLinearColor::White, 8.0f, SDPG_Foreground);
	}

	// 編集対象の元姿勢ゴースト
	if (EditTarget.IsValid())
	{
		const FTransform OriginalTransform = EditTargetOriginalTransform;
		const FQuat OrigRot = OriginalTransform.GetRotation();
		DrawOrientedWireBox(PDI, OriginalTransform.GetTranslation(),
			OrigRot.GetAxisX(), OrigRot.GetAxisY(), OrigRot.GetAxisZ(),
			GetFocusSize() * 0.5, FLinearColor(0.6f, 0.6f, 0.6f, 0.4f), SDPG_World, 1.0f, 1.0f);
	}

	// 貼り付け先コンポーネントの境界
	if (Anchor.Component.IsValid())
	{
		UPrimitiveComponent* AnchorComp = Anchor.Component.Get();
		DrawWireBox(PDI, AnchorComp->Bounds.GetBox(), FLinearColor(0.15f, 0.55f, 1.0f, 0.5f), SDPG_World, 1.0f, 0.5f);
	}

	// 測量線
	for (const LeeMapEditor::FLeeMeasureEntry& Entry : Measurements)
	{
		AActor* Target = Entry.Target.Get();
		if (!Target)
		{
			continue;
		}
		const FVector TargetCenter = Target->GetActorLocation();
		const FLinearColor LineColor = Entry.bSpecified ? FLinearColor(1.0f, 0.25f, 1.0f, 0.9f) : FLinearColor(1.0f, 0.40f, 0.40f, 0.9f);
		DrawDashedLine(PDI, Center, TargetCenter, LineColor, 10.0f, SDPG_Foreground);
		PDI->DrawPoint(TargetCenter, LineColor, 6.0f, SDPG_Foreground);
	}

	// 原生配置では座標軸も表示する
	if (PlacementKind == LeeMapEditor::ELeePlacementKind::Rock)
	{
		DrawCoordinateSystem(PDI, Center, PreviewTransform.Rotator(), 25.0f, SDPG_Foreground);
	}
}

void FLeeMapEditorTool::DrawHUD(FEditorViewportClient* InViewportClient, FViewport* InViewport, const FSceneView* InView, FCanvas* Canvas)
{
	if (!bPreviewActive || InView == nullptr || Canvas == nullptr)
	{
		return;
	}

	FVector Center;
	if (!GetFocusCenter(Center))
	{
		return;
	}

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9);
	const float DpiScale = FMath::Max(Canvas->GetDPIScale(), 1.0f);

	for (const LeeMapEditor::FLeeMeasureEntry& Entry : Measurements)
	{
		AActor* Target = Entry.Target.Get();
		if (!Target)
		{
			continue;
		}
		const FVector MidPoint = (Center + Target->GetActorLocation()) * 0.5;
		FVector2D Pixel;
		if (!InView->WorldToPixel(MidPoint, Pixel))
		{
			continue;
		}
		const FLinearColor TextColor = Entry.bSpecified ? FLinearColor(1.0f, 0.35f, 1.0f) : FLinearColor(1.0f, 0.45f, 0.45f);
		FCanvasTextItem TextItem(Pixel / DpiScale,
			FText::FromString(FString::Printf(TEXT("%.1f  ΔZ%+.1f"), Entry.Distance, Entry.DeltaZ)),
			Font, TextColor);
		TextItem.bCentreX = true;
		Canvas->DrawItem(TextItem);
	}
}

void FLeeMapEditorTool::SetPickMode(LeeMapEditor::ELeePickMode InMode)
{
	PickMode = InMode;
	NotifyChanged();
}

void FLeeMapEditorTool::ClearAttachOverride()
{
	AttachOverride = nullptr;
	if (Anchor.bValid)
	{
		// 2026.11.27 李奕勋 start
		// 貼り付け先の明示解除：現在の配置種別に応じて近水平基底を選ぶ
		//Anchor.RecomputeAxes();←元のコードは消さない
		Anchor.RecomputeAxes(PlacementKind == LeeMapEditor::ELeePlacementKind::Hold);
		// 2026.11.27 李奕勋 end
	}
	NotifyChanged();
}

void FLeeMapEditorTool::ClearSpecifiedRefs()
{
	SpecifiedRefs.Reset();
	RefreshMeasurements();
	NotifyChanged();
}

void FLeeMapEditorTool::AddSpecifiedRef(AActor* InHoldActor)
{
	if (InHoldActor && IsHoldActor(InHoldActor))
	{
		SpecifiedRefs.AddUnique(InHoldActor);
		RefMode = LeeMapEditor::ELeeRefMode::Specified;
		RefreshMeasurements();
		NotifyChanged();
	}
}

void FLeeMapEditorTool::RemoveSpecifiedRef(AActor* InHoldActor)
{
	SpecifiedRefs.Remove(TWeakObjectPtr<AActor>(InHoldActor));
	RefreshMeasurements();
	NotifyChanged();
}

TArray<AActor*> FLeeMapEditorTool::GetSpecifiedRefs() const
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<AActor>& WeakRef : SpecifiedRefs)
	{
		if (AActor* Ref = WeakRef.Get())
		{
			Result.Add(Ref);
		}
	}
	return Result;
}

bool FLeeMapEditorTool::ComputePreviewTransform(FTransform& OutTransform) const
{
	if (!bPreviewActive || !Anchor.bValid)
	{
		return false;
	}

	const double AngleRad = FMath::DegreesToRadians(AngleDeg);
	const double CosAngle = FMath::Cos(AngleRad);
	const double SinAngle = FMath::Sin(AngleRad);

	// 法線回りの回転：U' = U cosθ − V sinθ、V' = U sinθ + V cosθ
	const FVector AxisU2 = Anchor.AxisU * CosAngle - Anchor.AxisV * SinAngle;
	const FVector AxisV2 = Anchor.AxisU * SinAngle + Anchor.AxisV * CosAngle;
	const FVector AxisN = Anchor.Normal;

	// 盒体中心 = 検証済み面上基準点 + 法線方向 (厚さ/2 + 面外オフセット) + 検証済み接線オフセット
	const FVector LocalSize = GetFocusSize();
	const FVector Center = ValidatedSurfacePoint
		+ AxisN * (LocalSize.Y * 0.5 + OffsetN)
		+ AxisU2 * ValidOffsetU
		+ AxisV2 * ValidOffsetV;

	// X=横接線, Y=法線, Z=縦接線 の右手系行列
	const FMatrix BoxToWorld(AxisU2, AxisN, AxisV2, Center);
	OutTransform = FTransform(BoxToWorld);
	return true;
}

void FLeeMapEditorTool::CancelPreview()
{
	bPreviewActive = false;
	bDragging = false;
	Anchor.Reset();
	ValidatedSurfacePoint = FVector::ZeroVector;
	ValidOffsetU = 0.0;
	ValidOffsetV = 0.0;
	EditTarget = nullptr;
	PickMode = LeeMapEditor::ELeePickMode::None;
	LastCheck.bRan = false;
	RefreshMeasurements();
	NotifyChanged();
}

FString FLeeMapEditorTool::ApplyHold()
{
	UWorld* World = GetWorldSafe();
	if (!World || !World->GetCurrentLevel())
	{
		return TEXT("没有可编辑的关卡");
	}
	if (!bPreviewActive)
	{
		return TEXT("预览未建立，请先在视口中点击岩面");
	}

	FTransform PreviewTransform;
	if (!ComputePreviewTransform(PreviewTransform))
	{
		return TEXT("预览变换无效");
	}

	// 2026.11.25 Lee start
	// 主攀爬面チェック：プレビュー変換の検証後・トランザクションと一切の変更前に同一規則で拒否する。
	// 拒否時もプレビューを保持するため、パラメータ調整や面の選び直しが可能（新規・既存編集の両方に適用）
	// 2026.11.27 李奕勋 start
	/**
	 * @brief 朝向ロック撤回に伴い、適用時の主攀爬面チェック（30° 拒否）を止めた。
	 *        元のコードは消さない。
	 */
	//if (!IsMainFaceWithinTolerance(Anchor.Normal))
	//{
	//	return FString::Printf(TEXT("拒绝应用：%s"), *MainFaceCheckRejectText());
	//}
	// 2026.11.27 李奕勋 end
	// 2026.11.25 Lee end

	UClass* HoldClass = ResolveHoldClass();
	if (!HoldClass)
	{
		return TEXT("BP_ClimbHold 蓝图缺失，禁止生成替代 Actor；请确认 /Game/Maps/ClimbSample/BP_ClimbHold 存在");
	}

	AActor* ExistingHold = EditTarget.Get();

	FScopedTransaction Transaction(
		TEXT("SplitClimbMapEditor"),
		NSLOCTEXT("SplitClimbMapEditor", "ApplyHoldTransaction", "SplitClimb 抓点 应用"),
		ExistingHold);

	AActor* HoldActor = nullptr;
	if (ExistingHold)
	{
		HoldActor = ExistingHold;
		HoldActor->Modify();
		if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(HoldActor->GetRootComponent()))
		{
			RootComp->Modify();
			RootComp->SetWorldTransform(PreviewTransform);
		}
		else
		{
			HoldActor->SetActorTransform(PreviewTransform);
		}
	}
	else
	{
		HoldActor = GEditor->AddActor(World->GetCurrentLevel(), HoldClass, PreviewTransform, /*bSilent*/ true, RF_Transactional, /*bSelectActor*/ false);
		if (!HoldActor)
		{
			return TEXT("抓点生成失败");
		}
	}

	// 标签更新
	HoldActor->Modify();
	HoldActor->Tags.Remove(LeeMapEditor::HoldStrongTag);
	HoldActor->Tags.Remove(LeeMapEditor::HoldWeakTag);
	HoldActor->Tags.AddUnique(GetCurrentHoldTag());

	// 盒体全尺寸（SetBoxExtent 使用半尺寸）
	if (UBoxComponent* BoxComp = Cast<UBoxComponent>(HoldActor->GetRootComponent()))
	{
		BoxComp->Modify();
		BoxComp->SetBoxExtent(Size * 0.5, true);
	}

	// 附着到命中的岩体组件（或悬空）
	UPrimitiveComponent* AttachTarget = AttachOverride.IsValid() ? AttachOverride.Get() : Anchor.Component.Get();
	if (AttachTarget)
	{
		HoldActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepWorldTransform);
	}
	else if (HoldActor->GetRootComponent() && HoldActor->GetRootComponent()->GetAttachParent())
	{
		HoldActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	const FString ResultMessage = FString::Printf(TEXT("已%s抓点 [%s] %s  尺寸 %.0f x %.0f x %.0f"),
		ExistingHold ? TEXT("修改") : TEXT("生成"),
		*HoldActor->GetName(),
		HoldTagDisplayName(HoldType),
		Size.X, Size.Y, Size.Z);

	CancelPreview();
	return ResultMessage;
}

FString FLeeMapEditorTool::ApplyRock()
{
	UWorld* World = GetWorldSafe();
	if (!World || !World->GetCurrentLevel())
	{
		return TEXT("没有可编辑的关卡");
	}
	if (!bPreviewActive)
	{
		return TEXT("预览未建立，请先在视口中点击岩面");
	}
	UStaticMesh* SelectedMesh = RockMesh;
	if (!SelectedMesh)
	{
		return TEXT("未选择岩体网格");
	}

	FTransform PreviewTransform;
	if (!ComputePreviewTransform(PreviewTransform))
	{
		return TEXT("预览变换无效");
	}

	AActor* ExistingHold = EditTarget.Get();
	FScopedTransaction Transaction(
		TEXT("SplitClimbMapEditor"),
		NSLOCTEXT("SplitClimbMapEditor", "ApplyRockTransaction", "SplitClimb 岩体 应用"),
		ExistingHold);

	AStaticMeshActor* RockActor = Cast<AStaticMeshActor>(
		GEditor->AddActor(World->GetCurrentLevel(), AStaticMeshActor::StaticClass(), PreviewTransform, /*bSilent*/ true, RF_Transactional, /*bSelectActor*/ false));
	if (!RockActor)
	{
		return TEXT("岩体生成失败");
	}

	if (UStaticMeshComponent* MeshComp = RockActor->GetStaticMeshComponent())
	{
		MeshComp->Modify();
		MeshComp->SetStaticMesh(SelectedMesh);
	}
	RockActor->SetMobility(EComponentMobility::Static);

	if (Anchor.Component.IsValid())
	{
		RockActor->AttachToComponent(Anchor.Component.Get(), FAttachmentTransformRules::KeepWorldTransform);
	}

	const FString ResultMessage = FString::Printf(TEXT("已生成岩体 %s（%s）"),
		*RockActor->GetName(),
		*SelectedMesh->GetName());

	CancelPreview();
	return ResultMessage;
}

FString FLeeMapEditorTool::StartEditHold(AActor* InHoldActor)
{
	if (!InHoldActor || !IsHoldActor(InHoldActor))
	{
		return TEXT("选中的 Actor 不是抓点");
	}

	UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(InHoldActor->GetRootComponent());
	UBoxComponent* BoxComp = Cast<UBoxComponent>(RootComp);
	if (!RootComp || !BoxComp)
	{
		return TEXT("抓点缺少盒体组件，无法编辑");
	}

	EditTarget = InHoldActor;

	// タグから強弱を読み込む
	if (InHoldActor->Tags.Contains(LeeMapEditor::HoldWeakTag))
	{
		HoldType = LeeMapEditor::ELeeHoldType::Weak;
	}
	else if (InHoldActor->Tags.Contains(LeeMapEditor::HoldStrongTag))
	{
		HoldType = LeeMapEditor::ELeeHoldType::Strong;
	}

	// 現在の盒体の実寸法（スケール適用後）を引き継ぐ
	Size = BoxComp->GetScaledBoxExtent() * 2.0;

	// 姿勢からアンカー（面・法線・接線・回転角）を復元する。
	// 親のない旧独立抓点を勝手に岩面へ吸附してはいけない：貼り付け先はユーザーが明示選択する
	const FQuat RootQuat = RootComp->GetComponentQuat();
	const FVector AxisN = RootQuat.GetAxisY();
	const FVector Center = RootComp->GetComponentLocation();

	Anchor.Reset();
	Anchor.Normal = AxisN;
	Anchor.Point = Center - AxisN * (Size.Y * 0.5);
	Anchor.Component = Cast<UPrimitiveComponent>(RootComp->GetAttachParent());
	// 2026.11.27 李奕勋 start
	// 既存抓点の編集読み出しは常に抓点基準：近水平面でも右手系の面内基底を使う（明示的に true）
	//Anchor.RecomputeAxes();←元のコードは消さない
	Anchor.RecomputeAxes(true);
	// 2026.11.27 李奕勋 end

	OffsetU = 0.0;
	OffsetV = 0.0;
	OffsetN = 0.0;
	ValidatedSurfacePoint = Anchor.Point;
	ValidOffsetU = 0.0;
	ValidOffsetV = 0.0;
	AttachOverride = Anchor.Component;

	// 既存回転角を接線系へ復元
	const FVector AxisXBox = RootQuat.GetAxisX();
	const double CosAngle = FVector::DotProduct(AxisXBox, Anchor.AxisU);
	// 2026.11.27 李奕勋 start
	// プレビュー側は U' = U cosθ - V sinθ で回すため、盒体ローカル X と V の内積は -sinθ に一致する。
	// 既存インスタンスから AngleDeg を読み戻すときは符号を反転し、再適用しても同じ姿勢に戻るようにする
	//const double SinAngle = FVector::DotProduct(AxisXBox, Anchor.AxisV);
	const double SinAngle = -FVector::DotProduct(AxisXBox, Anchor.AxisV);
	// 2026.11.27 李奕勋 end
	AngleDeg = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(SinAngle, CosAngle)));

	bPreviewActive = true;
	bDragging = false;
	EditTargetOriginalTransform = RootComp->GetComponentTransform();
	LastCheck.bRan = false;
	RefreshMeasurements();
	NotifyChanged();

	return FString::Printf(TEXT("已载入抓点 %s（%s %.0f x %.0f x %.0f），调整后点击应用"),
		*InHoldActor->GetActorLabel(),
		HoldTagDisplayName(HoldType),
		Size.X, Size.Y, Size.Z);
}

void FLeeMapEditorTool::SetRefMode(LeeMapEditor::ELeeRefMode InMode)
{
	RefMode = InMode;
	RefreshMeasurements();
	NotifyChanged();
}

void FLeeMapEditorTool::SetCheckMode(LeeMapEditor::ELeeCheckMode InMode)
{
	CheckMode = InMode;
	LastCheck.bRan = false;
	NotifyChanged();
}

const LeeMapEditor::FLeeCheckResult& FLeeMapEditorTool::RunCheck()
{
	FTransform FocusTransform;
	if (!GetFocusTransform(FocusTransform))
	{
		LastCheck = LeeMapEditor::FLeeCheckResult();
		LastCheck.bRan = false;
		LastCheck.Detail = TEXT("预览未建立，请先点击岩面建立预览或从列表载入抓点");
		return LastCheck;
	}

	UWorld* World = GetWorldSafe();
	if (!World)
	{
		LastCheck = LeeMapEditor::FLeeCheckResult();
		LastCheck.Detail = TEXT("没有可查询的关卡世界");
		return LastCheck;
	}

	const FVector Center = FocusTransform.GetTranslation();
	const FQuat Rotation = FocusTransform.GetRotation();
	const FVector HalfExtent = GetFocusSize() * 0.5;
	const FCollisionShape BoxShape = FCollisionShape::MakeBox(HalfExtent);
	const FCollisionQueryParams QueryParams = MakeTraceParams(/*bTraceComplex*/ false);

	LastCheck = LeeMapEditor::FLeeCheckResult();
	LastCheck.bRan = true;

	if (CheckMode == LeeMapEditor::ELeeCheckMode::IgnoreCollision)
	{
		// 碰撞忽略模式：仅统计包围盒几何相交的组件
		int32 GeomHitCount = 0;
		bool bSelfHit = false;
		FString HitNames;
		if (ULevel* Level = World->GetCurrentLevel())
		{
			// OBB → AABB：按各盒体轴向投影求世界包围盒半尺寸
			const FVector AbsAxisX = Rotation.GetAxisX().GetAbs();
			const FVector AbsAxisY = Rotation.GetAxisY().GetAbs();
			const FVector AbsAxisZ = Rotation.GetAxisZ().GetAbs();
			const FVector AabbHalfExtent = AbsAxisX * HalfExtent.X + AbsAxisY * HalfExtent.Y + AbsAxisZ * HalfExtent.Z;
			const FBox QueryAABB = FBox::BuildAABB(Center, AabbHalfExtent);
			for (AActor* Actor : Level->Actors)
			{
				if (!Actor)
				{
					continue;
				}
				const bool bSelf = IsHoldActor(Actor);
				TInlineComponentArray<UPrimitiveComponent*, 8> Components;
				Actor->GetComponents(Components);
				for (UPrimitiveComponent* Component : Components)
				{
					if (!Component)
					{
						continue;
					}
					if (Component->Bounds.GetBox().Intersect(QueryAABB))
					{
						++GeomHitCount;
						if (bSelf)
						{
							bSelfHit = true;
						}
						else if (GeomHitCount <= 6)
						{
							HitNames += FString::Printf(TEXT("  · %s / %s\n"), *Actor->GetActorLabel(), *Component->GetName());
						}
					}
				}
			}
		}
		LastCheck.HitCount = GeomHitCount;
		LastCheck.bSelfHit = bSelfHit;
		LastCheck.bSuccess = true;
		LastCheck.FirstHitName = TEXT("几何相交检查");
		LastCheck.Detail = FString::Printf(TEXT("忽略碰撞：包围盒相交组件 %d 个\n%s"), GeomHitCount, *HitNames);
		return LastCheck;
	}

	// 編集中の実配置盒体の衝突設定検証：QueryOnly ＋ Climbable Block ＋ Pawn Ignore。
	// 「自身」の判定はコンポーネントポインタの厳密比較で行い、隣接抓点を誤って自身としない
	AActor* FocusActor = EditTarget.Get();
	const UBoxComponent* SelfBox = FocusActor ? Cast<UBoxComponent>(FocusActor->GetRootComponent()) : nullptr;
	FString ConfigLine;
	bool bConfigOk = true;
	if (SelfBox)
	{
		const bool bQueryOnly = (SelfBox->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
		const ECollisionResponse ClimbableResponse = SelfBox->GetCollisionResponseToChannel(ECC_GameTraceChannel1);
		const ECollisionResponse PawnResponse = SelfBox->GetCollisionResponseToChannel(ECC_Pawn);
		bConfigOk = bQueryOnly && (ClimbableResponse == ECR_Block) && (PawnResponse == ECR_Ignore);
		ConfigLine = FString::Printf(TEXT("自身配置：CollisionEnabled=%s，Climbable=%s，Pawn=%s\n"),
			bQueryOnly ? TEXT("QueryOnly") : TEXT("非 QueryOnly"),
			CollisionResponseName(ClimbableResponse),
			CollisionResponseName(PawnResponse));
	}
	else
	{
		ConfigLine = TEXT("自身配置：抓点尚未应用，仅验证预览几何\n");
	}

	TArray<FOverlapResult> Overlaps;
	const ECollisionChannel QueryChannel = (CheckMode == LeeMapEditor::ELeeCheckMode::Climbable)
		? ECC_GameTraceChannel1
		: ECC_Pawn;
	const bool bOverlap = World->OverlapMultiByChannel(Overlaps, Center, Rotation, QueryChannel, BoxShape, QueryParams);

	int32 HitCount = 0;
	bool bSelfHit = false;
	FString HitNames;
	TSet<AActor*> VisitedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* OverlapComp = Overlap.Component.Get();
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapComp || !OverlapActor)
		{
			continue;
		}
		++HitCount;
		if (OverlapComp == SelfBox)
		{
			// 厳密にこの抓点自身の盒体コンポーネントのみを「自身」とする
			bSelfHit = true;
			LastCheck.FirstHitName = OverlapActor->GetActorLabel();
		}
		else if (!VisitedActors.Contains(OverlapActor) && VisitedActors.Num() < 5)
		{
			VisitedActors.Add(OverlapActor);
			HitNames += FString::Printf(TEXT("  · %s / %s（Climbable=%s，Pawn=%s）\n"),
				*OverlapActor->GetActorLabel(),
				*OverlapComp->GetName(),
				CollisionResponseName(OverlapComp->GetCollisionResponseToChannel(ECC_GameTraceChannel1)),
				CollisionResponseName(OverlapComp->GetCollisionResponseToChannel(ECC_Pawn)));
		}
	}

	LastCheck.HitCount = HitCount;
	LastCheck.bSelfHit = bSelfHit;

	if (CheckMode == LeeMapEditor::ELeeCheckMode::Climbable)
	{
		LastCheck.bSuccess = bSelfHit && bConfigOk;
		LastCheck.Detail = FString::Printf(TEXT("Climbable 查询：%s\n命中 %d 个组件\n%s%s"),
			bSelfHit ? TEXT("盒体自身阻挡 Climbable") : TEXT("盒体自身未命中，抓点不阻挡 Climbable，需检查碰撞设置"),
			HitCount, *ConfigLine, *HitNames);
	}
	else
	{
		LastCheck.bSuccess = !bSelfHit && bConfigOk;
		LastCheck.Detail = FString::Printf(TEXT("Pawn 阻挡查询：%s\n命中 %d 个组件\n%s%s"),
			!bSelfHit ? TEXT("盒体自身不阻挡 Pawn，符合抓点配置") : TEXT("盒体自身阻挡了 Pawn，抓点不应阻挡 Pawn"),
			HitCount, *ConfigLine, *HitNames);
	}
	return LastCheck;
}

const LeeMapEditor::FLeeCheckResult& FLeeMapEditorTool::RunReverseProbe()
{
	FTransform FocusTransform;
	if (!GetFocusTransform(FocusTransform))
	{
		LastCheck = LeeMapEditor::FLeeCheckResult();
		LastCheck.bRan = false;
		LastCheck.Detail = TEXT("预览未建立，无法进行外侧反查");
		return LastCheck;
	}

	UWorld* World = GetWorldSafe();
	if (!World)
	{
		LastCheck = LeeMapEditor::FLeeCheckResult();
		LastCheck.Detail = TEXT("没有可查询的关卡世界");
		return LastCheck;
	}

	const FVector Center = FocusTransform.GetTranslation();
	const FQuat Rotation = FocusTransform.GetRotation();
	const FVector HalfExtent = GetFocusSize() * 0.5;
	const FVector AxisOut = Rotation.GetAxisY();

	LastCheck = LeeMapEditor::FLeeCheckResult();
	LastCheck.bRan = true;

	if (CheckMode == LeeMapEditor::ELeeCheckMode::IgnoreCollision)
	{
		LastCheck.Detail = TEXT("外侧反查需要碰撞查询，请切换到 Climbable 或 Pawn 阻挡模式");
		return LastCheck;
	}

	const ECollisionChannel QueryChannel = (CheckMode == LeeMapEditor::ELeeCheckMode::Climbable)
		? ECC_GameTraceChannel1
		: ECC_Pawn;
	const FVector ProbeStart = Center + AxisOut * (HalfExtent.Y + LeeMapEditor::ReverseProbeClearance);
	const FVector ProbeEnd = Center - AxisOut * 1.0;
	const FCollisionQueryParams QueryParams = MakeTraceParams(/*bTraceComplex*/ false);

	FHitResult ProbeHit;
	const bool bHit = World->LineTraceSingleByChannel(ProbeHit, ProbeStart, ProbeEnd, QueryChannel, QueryParams);

	if (!bHit)
	{
		LastCheck.bSuccess = false;
		LastCheck.FirstHitName = TEXT("无命中");
		LastCheck.Detail = TEXT("外侧反查：从盒体外侧发起查询未命中任何组件");
		return LastCheck;
	}

	AActor* HitActor = ProbeHit.GetActor();
	UPrimitiveComponent* HitComp = ProbeHit.GetComponent();
	AActor* FocusActor = EditTarget.Get();
	const UBoxComponent* SelfBox = FocusActor ? Cast<UBoxComponent>(FocusActor->GetRootComponent()) : nullptr;

	// 厳密比較：外→内レイの最初のヒットが「この抓点自身の盒体コンポーネント」と同一であること。
	// 隣接抓点の盒体がヒットしても成功扱いにしない（誤報防止）
	const bool bSelfHit = (HitComp != nullptr) && (HitComp == SelfBox);
	const bool bHitOtherHold = !bSelfHit && HitActor && IsHoldActor(HitActor);

	LastCheck.HitCount = 1;
	LastCheck.bSelfHit = bSelfHit;
	LastCheck.FirstHitName = HitActor ? HitActor->GetActorLabel() : TEXT("无命中");
	LastCheck.bSuccess = bSelfHit;

	if (bSelfHit)
	{
		LastCheck.Detail = FString::Printf(TEXT("外侧反查：首个命中为抓点自身盒体（%s / %s），通过"),
			*LastCheck.FirstHitName, HitComp ? *HitComp->GetName() : TEXT("?"));
	}
	else if (bHitOtherHold)
	{
		LastCheck.Detail = FString::Printf(TEXT("外侧反查：首个命中为旁边抓点 %s / %s，未命中目标抓点自身盒体，不能视为通过"),
			*LastCheck.FirstHitName, HitComp ? *HitComp->GetName() : TEXT("?"));
	}
	else if (FocusActor)
	{
		LastCheck.Detail = FString::Printf(TEXT("外侧反查：首个命中为 %s / %s，未命中抓点自身盒体，请检查碰撞设置"),
			*LastCheck.FirstHitName, HitComp ? *HitComp->GetName() : TEXT("?"));
	}
	else
	{
		LastCheck.Detail = FString::Printf(TEXT("外侧反查：抓点尚未应用，命中为 %s / %s（应用后应为抓点盒体）"),
			*LastCheck.FirstHitName, HitComp ? *HitComp->GetName() : TEXT("?"));
	}
	return LastCheck;
}

TArray<AActor*> FLeeMapEditorTool::CollectHoldActors()
{
	TArray<AActor*> Result;
	UWorld* World = GetWorldSafe();
	ULevel* Level = World ? World->GetCurrentLevel() : nullptr;
	if (!Level)
	{
		return Result;
	}

	for (AActor* Actor : Level->Actors)
	{
		if (!Actor || Actor->IsPendingKillPending() || Actor->HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject))
		{
			continue;
		}
		// タグのみでは装飾岩も混入するため、実際の抓点クラス＋盒体コンポーネントで判定する
		if (IsHoldActor(Actor))
		{
			Result.AddUnique(Actor);
		}
	}
	return Result;
}

void FLeeMapEditorTool::LocateHold(AActor* InHoldActor)
{
	if (!InHoldActor)
	{
		return;
	}
	GEditor->SelectActor(InHoldActor, true, true);
	const FBox Bounds = InHoldActor->GetComponentsBoundingBox(/*bNonColliding*/ true);
	if (!Bounds.IsValid)
	{
		return;
	}
	for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
	{
		if (ViewportClient)
		{
			ViewportClient->FocusViewportOnBox(Bounds, /*bInstant*/ true);
		}
	}
}

UClass* FLeeMapEditorTool::ResolveHoldClass()
{
	if (!HoldBlueprint)
	{
		HoldBlueprint = LoadObject<UBlueprint>(nullptr, *LeeMapEditor::HoldBlueprintPath);
	}

	UClass* GeneratedClass = nullptr;
	if (HoldBlueprint && HoldBlueprint->GeneratedClass)
	{
		UClass* CandidateClass = HoldBlueprint->GeneratedClass.Get();
		if (!CandidateClass->HasAnyClassFlags(CLASS_Abstract)
			&& CandidateClass->IsChildOf(AActor::StaticClass()))
		{
			GeneratedClass = CandidateClass;
		}
	}
	bUsingFallbackHoldClass = (GeneratedClass == nullptr);
	return GeneratedClass;
}

FString FLeeMapEditorTool::BuildStatusText()
{
	ResolveHoldClass();

	FString StatusText;
	StatusText += bModeActive ? TEXT("工具状态：已启用（视口放置模式）\n") : TEXT("工具状态：已停用\n");
	StatusText += (PlacementKind == LeeMapEditor::ELeePlacementKind::Hold)
		? FString::Printf(TEXT("放置类型：抓点 %s\n当前尺寸（宽×厚×高）：%.0f × %.0f × %.0f cm\n"),
			HoldTagDisplayName(HoldType), Size.X, Size.Y, Size.Z)
		: FString::Printf(TEXT("放置类型：岩体 %s\n"), RockMesh ? *RockMesh->GetName() : TEXT("（未选择网格）"));

	FString AttachName = TEXT("无（悬空）");
	UPrimitiveComponent* AttachComp = AttachOverride.IsValid() ? AttachOverride.Get() : (Anchor.bValid ? Anchor.Component.Get() : nullptr);
	if (AttachComp)
	{
		AttachName = AttachComp->GetOwner()
			? FString::Printf(TEXT("%s / %s"), *AttachComp->GetOwner()->GetActorLabel(), *AttachComp->GetName())
			: AttachComp->GetName();
	}
	StatusText += FString::Printf(TEXT("贴附岩体：%s\n"), *AttachName);

	StatusText += bPreviewActive ? TEXT("预览：已建立（左键拖动贴面滑动，Esc 取消）\n") : TEXT("预览：无（左键点击岩面建立）\n");
	if (EditTarget.IsValid())
	{
		StatusText += FString::Printf(TEXT("编辑已有实例：%s\n"), *EditTarget->GetActorLabel());
	}
	if (PickMode != LeeMapEditor::ELeePickMode::None)
	{
		const TCHAR* PickHint = TEXT("");
		switch (PickMode)
		{
			case LeeMapEditor::ELeePickMode::Rock: PickHint = TEXT("请点击视口中的岩体表面"); break;
			case LeeMapEditor::ELeePickMode::Hold: PickHint = TEXT("请点击视口中的抓点"); break;
			case LeeMapEditor::ELeePickMode::Reference: PickHint = TEXT("请点击视口中的参照抓点"); break;
			default: break;
		}
		StatusText += FString::Printf(TEXT("拾取模式：%s\n"), PickHint);
	}

	// 2026.11.25 Lee start
	// 主攀爬面チェック：抓点モードかつプレビュー表示中（アンカー有効）に合否を表示する。
	// 不合格文は MainFaceCheckRejectText() の単一ソースから生成する
	// 2026.11.27 李奕勋 start
	/**
	 * @brief 朝向ロック撤回に伴い、パネル状態表示への「抓点朝向检查」行の出力を止めた。
	 *        元のコードは消さない。
	 */
	//if (PlacementKind == LeeMapEditor::ELeePlacementKind::Hold && bPreviewActive && Anchor.bValid)
	//{
	//	if (IsMainFaceWithinTolerance(Anchor.Normal))
	//	{
	//		StatusText += TEXT("抓点朝向检查：通过\n");
	//	}
	//	else
	//	{
	//		StatusText += FString::Printf(TEXT("抓点朝向检查：未通过——%s\n"), *MainFaceCheckRejectText());
	//	}
	//}
	// 2026.11.27 李奕勋 end
	// 2026.11.25 Lee end

	if (bUsingFallbackHoldClass)
	{
		StatusText += TEXT("抓点蓝图 BP_ClimbHold 缺失：禁止生成替代 Actor，应用不可用；原生岩体放置不受影响\n");
	}
	return StatusText;
}

UWorld* FLeeMapEditorTool::GetWorldSafe() const
{
	UWorld* World = GLevelEditorModeTools().GetWorld();
	if (World && World->IsEditorWorld())
	{
		return World;
	}
	return nullptr;
}

bool FLeeMapEditorTool::SurfaceTrace(UWorld* InWorld, const FVector& InStart, const FVector& InDir, FHitResult& OutHit)
{
	if (!InWorld)
	{
		return false;
	}
	FCollisionQueryParams Params = MakeTraceParams(/*bTraceComplex*/ true);
	TArray<AActor*> HoldActors = CollectHoldActors();
	for (AActor* HoldActor : HoldActors)
	{
		Params.AddIgnoredActor(HoldActor);
	}
	if (AActor* EditActor = EditTarget.Get())
	{
		Params.AddIgnoredActor(EditActor);
	}
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	return InWorld->LineTraceSingleByObjectType(OutHit, InStart, InStart + InDir * LEE_TRACE_RANGE, ObjectParams, Params);
}

bool FLeeMapEditorTool::HoldPickTrace(UWorld* InWorld, const FVector& InStart, const FVector& InDir, FHitResult& OutHit)
{
	if (!InWorld)
	{
		return false;
	}
	// 抓点拾取は Climbable チャンネル（ECC_GameTraceChannel1）で行う（抓点の盒体が Block するチャンネル）
	FCollisionQueryParams Params = MakeTraceParams(/*bTraceComplex*/ false);
	return InWorld->LineTraceSingleByChannel(OutHit, InStart, InStart + InDir * LEE_TRACE_RANGE, ECC_GameTraceChannel1, Params);
}

bool FLeeMapEditorTool::TraceFromScreen(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InX, int32 InY, bool bPickHolds, FHitResult& OutHit)
{
	UWorld* World = GetWorldSafe();
	if (!World || !InViewportClient || !InViewport)
	{
		return false;
	}

	// スクリーン座標からワールドレイを生成する（エンジン標準のフォリージュ方式と同じ経路）
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		InViewport,
		InViewportClient->GetScene(),
		InViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(InViewportClient->IsRealtime()));

	FSceneView* View = InViewportClient->CalcSceneView(&ViewFamily);
	if (!View)
	{
		return false;
	}

	FViewportCursorLocation MouseRay(View, InViewportClient, InX, InY);
	FVector RayStart = MouseRay.GetOrigin();
	const FVector RayDirection = MouseRay.GetDirection();
	if (InViewportClient->IsOrtho())
	{
		RayStart -= LEE_ORTHO_BACKOFF * RayDirection;
	}
	return bPickHolds
		? HoldPickTrace(World, RayStart, RayDirection, OutHit)
		: SurfaceTrace(World, RayStart, RayDirection, OutHit);
}

void FLeeMapEditorTool::BeginPreviewFromHit(const FHitResult& InHit)
{
	Anchor.Component = InHit.Component;
	Anchor.Point = InHit.ImpactPoint;
	Anchor.Normal = InHit.ImpactNormal.GetSafeNormal();
	// 2026.11.27 李奕勋 start
	// 配置フロー：現在の配置種別を渡し、抓点モードでは近水平面でも右手系の面内基底を使う
	//Anchor.RecomputeAxes();←元のコードは消さない
	Anchor.RecomputeAxes(PlacementKind == LeeMapEditor::ELeePlacementKind::Hold);
	// 2026.11.27 李奕勋 end
	ValidatedSurfacePoint = InHit.ImpactPoint;
	ValidOffsetU = 0.0;
	ValidOffsetV = 0.0;
	OffsetU = 0.0;
	OffsetV = 0.0;
	OffsetN = 0.0;
	bPreviewActive = true;
	LastCheck.bRan = false;
	RefreshMeasurements();
}

bool FLeeMapEditorTool::IsHoldActor(AActor* InActor)
{
	if (!InActor || InActor->IsPendingKillPending() || InActor->HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject))
	{
		return false;
	}

	// 1) 実際の抓点クラスであること：BP_ClimbHold の生成クラス（またはそのクラスパス）で判定する。
	//    BP アセットのロードに失敗しても、インスタンスのクラスパスで判定できる。
	const FString ClassPath = InActor->GetClass()->GetPathName();
	if (!ClassPath.StartsWith(LeeMapEditor::HoldClassPathPrefix))
	{
		return false;
	}

	// 2) 盒体コンポーネントを持つこと（TriggerBox 由来の実配置盒体）。
	return Cast<UBoxComponent>(InActor->GetRootComponent()) != nullptr;
}

void FLeeMapEditorTool::RefreshMeasurements()
{
	Measurements.Reset();

	FVector FocusCenter;
	if (!GetFocusCenter(FocusCenter))
	{
		return;
	}

	TArray<AActor*> Candidates;
	AActor* SelfActor = EditTarget.Get();
	if (RefMode == LeeMapEditor::ELeeRefMode::Specified)
	{
		Candidates = GetSpecifiedRefs();
	}
	else
	{
		Candidates = CollectHoldActors();
		Candidates.Remove(SelfActor);
		Candidates.Sort([FocusCenter](const AActor& A, const AActor& B)
		{
			const double DistA = FVector::DistSquared(FocusCenter, A.GetActorLocation());
			const double DistB = FVector::DistSquared(FocusCenter, B.GetActorLocation());
			return DistA < DistB;
		});
		if (Candidates.Num() > 3)
		{
			Candidates.SetNum(3);
		}
	}

	for (AActor* Target : Candidates)
	{
		if (!Target || Target == SelfActor)
		{
			continue;
		}
		LeeMapEditor::FLeeMeasureEntry Entry;
		Entry.Target = Target;
		Entry.Distance = FVector::Distance(FocusCenter, Target->GetActorLocation());
		Entry.DeltaZ = FocusCenter.Z - Target->GetActorLocation().Z;
		Entry.bSpecified = (RefMode == LeeMapEditor::ELeeRefMode::Specified);
		Measurements.Add(Entry);
	}
}

bool FLeeMapEditorTool::GetFocusCenter(FVector& OutCenter) const
{
	FTransform FocusTransform;
	if (!GetFocusTransform(FocusTransform))
	{
		return false;
	}
	OutCenter = FocusTransform.GetTranslation();
	return true;
}

FVector FLeeMapEditorTool::GetFocusSize() const
{
	if (PlacementKind == LeeMapEditor::ELeePlacementKind::Rock && RockMesh)
	{
		return RockMesh->GetBounds().GetBox().GetSize();
	}
	return Size;
}

bool FLeeMapEditorTool::GetFocusTransform(FTransform& OutTransform) const
{
	if (bPreviewActive && ComputePreviewTransform(OutTransform))
	{
		return true;
	}
	if (AActor* EditActor = EditTarget.Get())
	{
		if (const UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(EditActor->GetRootComponent()))
		{
			OutTransform = RootComp->GetComponentTransform();
			return true;
		}
		OutTransform = EditActor->GetActorTransform();
		return true;
	}
	return false;
}

void FLeeMapEditorTool::NotifyChanged()
{
	OnStateChanged.Broadcast();
}

FName FLeeMapEditorTool::GetCurrentHoldTag() const
{
	return (HoldType == LeeMapEditor::ELeeHoldType::Strong)
		? LeeMapEditor::HoldStrongTag
		: LeeMapEditor::HoldWeakTag;
}
