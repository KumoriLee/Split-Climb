#include "Lee/LeeSplitClimbMapEditorModule.h"

#include "Lee/LeeMapEditorMode.h"
#include "Lee/LeeMapEditorTool.h"
#include "Lee/SLeeMapEditorPanel.h"
// 2026.11.25 Lee start
#include "Lee/SLeeRockLibrary.h"
// 2026.11.25 Lee end
// 2026.11.25 Lee start
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
// 2026.11.25 Lee end

#include "Subsystems/PlacementSubsystem.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "EditorModeRegistry.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "IPlacementModeModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/StaticMesh.h"
#include "ToolMenu.h"
#include "ToolMenuOwner.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"
#include "Widgets/Docking/SDockTab.h"

DEFINE_LOG_CATEGORY(LogLeeSplitClimbMapEditor);

IMPLEMENT_MODULE(FLeeSplitClimbMapEditorModule, SplitClimbMapEditor)

namespace
{
	/** @brief 原生配置カテゴリのハンドル名。 */
	const FName NativePlacementCategoryName(TEXT("LeeSplitClimbRocks"));

	/** @brief 登録する実在メッシュ（ディスク上のアセット名に対応）。 */
	struct FLeeNativeRockEntry
	{
		const TCHAR* AssetPath;
		const TCHAR* DisplayName;
		int32 SortOrder;
	};

	const FLeeNativeRockEntry NativeRockEntries[] = {
		{ TEXT("/Game/LP_RocksandCliffs/Meshes/sm_cliff_11.sm_cliff_11"), TEXT("崖壁 · sm_cliff_11"), 10 },
		{ TEXT("/Game/LP_RocksandCliffs/Meshes/sm_rock_flat_01.sm_rock_flat_01"), TEXT("平台 · sm_rock_flat_01"), 20 },
		{ TEXT("/Game/LP_RocksandCliffs/Meshes/sm_rock_small_02.sm_rock_small_02"), TEXT("小岩 · sm_rock_small_02"), 30 },
		{ TEXT("/Game/LP_RocksandCliffs/Meshes/sm_rock_slab.sm_rock_slab"), TEXT("岩板 · sm_rock_slab"), 40 },
	};

	// 2026.11.25 Lee start
	/** @brief 抓点デバッグ補助輪の ON/OFF（1=表示, 0=非表示）。既定は表示。 */
	TAutoConsoleVariable<int32> CVarDebugGrabHolds(
		TEXT("splitclimb.DebugGrabHolds"),
		1,
		TEXT("PIE 中にクエリ可能な抓点の正面へ補助輪を描画する（1=表示, 0=非表示）"));

	/** @brief 補助輪の色（全抓点共通の単一色・明るい黄）。 */
	const FColor GrabHoldDebugColor(255, 220, 0);

	/** @brief 補助輪の線幅（cm）。 */
	const float GrabHoldDebugThickness = 2.0f;

	/** @brief 楕円近似の円周分割数。 */
	constexpr int32 GrabHoldDebugSegments = 48;

	/** @brief 正面を外法線方向へ軽く持ち上げる距離（cm）。同面重なり防止用。 */
	constexpr double GrabHoldDebugLiftCm = 2.0;

	/** @brief 四隅の直角マークの長さ上限（cm）。 */
	constexpr double GrabHoldDebugMarkLenCm = 12.0;
	// 2026.11.25 Lee end
}

TSharedRef<FLeeMapEditorTool> GetLeeSplitClimbMapEditorTool()
{
	return FModuleManager::LoadModuleChecked<FLeeSplitClimbMapEditorModule>(TEXT("SplitClimbMapEditor")).GetTool();
}

void FLeeSplitClimbMapEditorModule::StartupModule()
{
	UE_LOG(LogLeeSplitClimbMapEditor, Log, TEXT("StartupModule: SplitClimbMapEditor を起動します"));

	// ツール実体を生成する
	ToolInstance = MakeShared<FLeeMapEditorTool>();

	// エディタモードを登録する（モード一覧に表示する）
	FEditorModeRegistry::Get().RegisterMode<FLeeMapEditorMode>(
		LeeMapEditor::ModeID,
		NSLOCTEXT("SplitClimbMapEditor", "ModeDisplayName", "SplitClimb 抓点布置"),
		FSlateIcon(),
		/*bVisible*/ true,
		/*PriorityOrder*/ 100);

	// ドッキング可能なパネルタブを登録する
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		LeeMapEditor::TabID,
		FOnSpawnTab::CreateRaw(this, &FLeeSplitClimbMapEditorModule::SpawnPanelTab))
		.SetDisplayName(NSLOCTEXT("SplitClimbMapEditor", "TabDisplayName", "SplitClimb 布置"))
		.SetTooltipText(NSLOCTEXT("SplitClimbMapEditor", "TabTooltip", "SplitClimb 抓点与岩体布置面板"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);

	// 2026.11.25 Lee start
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		LeeMapEditor::RockLibraryTabID,
		FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
		{
			return SNew(SDockTab).TabRole(ETabRole::NomadTab)
			[
				MakeLeeRockLibrary(GetTool())
			];
		}))
		.SetDisplayName(NSLOCTEXT("SplitClimbMapEditor", "RockLibraryTab", "SplitClimb 岩壁素材"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	// 2026.11.25 Lee end

	// 2026.11.25 Lee start
	// PIE ワールド毎のポストアクタ tick で抓点デバッグ補助輪を描画する（解除は ShutdownModule で対称に行う）
	WorldPostActorTickHandle = FWorldDelegates::OnWorldPostActorTick.AddRaw(
		this, &FLeeSplitClimbMapEditorModule::HandleWorldPostActorTick);
	// 2026.11.25 Lee end

	// メニュー拡張と原生配置の登録は ToolMenus の起動後（エディタ起動完了後）に行う
	ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLeeSplitClimbMapEditorModule::RegisterMenus)); 
	// 2026.11.19 Lee start
	/** @brief 初期化済みのファクトリを使って配置項目を登録する。 */
	if (GIsRunning)
	{
		RegisterNativePlacement();
	}
	else
	{
		EditorInitializedHandle = FEditorDelegates::OnEditorInitialized.AddLambda( [this](double)
		{
			RegisterNativePlacement();
		});
	}
	// 2026.11.19 Lee end
}

void FLeeSplitClimbMapEditorModule::ShutdownModule()
{
	UE_LOG(LogLeeSplitClimbMapEditor, Log, TEXT("ShutdownModule: SplitClimbMapEditor を終了します"));

	// 2026.11.19 Lee start
	FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedHandle);
	// 2026.11.19 Lee end
	// 2026.11.25 Lee start
	// StartupModule で登録したポストアクタ tick 通知を対称に解除する
	FWorldDelegates::OnWorldPostActorTick.Remove(WorldPostActorTickHandle);
	// 2026.11.25 Lee end
	UnregisterNativePlacement();

	if (UToolMenus::TryGet())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
		// 2026.11.25 Lee start
		UToolMenus::UnregisterOwner(this);
		// 2026.11.25 Lee end
	}
	// 2026.11.25 Lee start
	if (const TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(LeeMapEditor::RockLibraryTabID))
	{
		Tab->RequestCloseTab();
	}
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LeeMapEditor::RockLibraryTabID);
	// 2026.11.25 Lee end
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LeeMapEditor::TabID);
	FEditorModeRegistry::Get().UnregisterMode(LeeMapEditor::ModeID);

	ToolInstance.Reset();
}

TSharedRef<FLeeMapEditorTool> FLeeSplitClimbMapEditorModule::GetTool() const
{
	return ToolInstance.ToSharedRef();
}

TSharedRef<SDockTab> FLeeSplitClimbMapEditorModule::SpawnPanelTab(const FSpawnTabArgs& InSpawnArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SLeeMapEditorPanel, GetTool())
		];
}

void FLeeSplitClimbMapEditorModule::OpenPanelTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(LeeMapEditor::TabID);
}

void FLeeSplitClimbMapEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window"))
	{
		FToolMenuSection& Section = Menu->AddSection("LeeSplitClimbMapEditorSection");
		Section.AddMenuEntry(
			"LeeSplitClimbMapEditorOpenPanel",
			NSLOCTEXT("SplitClimbMapEditor", "MenuOpenPanel", "SplitClimb 抓点布置"),
			NSLOCTEXT("SplitClimbMapEditor", "MenuOpenPanelTip", "打开 SplitClimb 抓点与岩体布置面板"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FLeeSplitClimbMapEditorModule::OpenPanelTab)));
		// 2026.11.25 Lee start
		Section.AddMenuEntry(
			"LeeSplitClimbRockLibrary",
			NSLOCTEXT("SplitClimbMapEditor", "RockLibraryMenu", "SplitClimb 岩壁素材"),
			NSLOCTEXT("SplitClimbMapEditor", "RockLibraryMenuTip", "查看和管理可用于摆放的岩壁网格"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]
			{
				FGlobalTabmanager::Get()->TryInvokeTab(LeeMapEditor::RockLibraryTabID);
			})));
		// 2026.11.25 Lee end
	}

	// 2026.11.19 Lee start
	// RegisterNativePlacement();
	// 2026.11.19 Lee end
}

void FLeeSplitClimbMapEditorModule::RegisterNativePlacement()
{
	if (PlacementCategoryHandle != NAME_None)
	{
		return;
	}

	// GEditor とアクタファクトリの準備を、PlacementMode モジュール取得および
	// カテゴリ生成よりも必ず先に確認する（起動早期の null 参照クラッシュ防止）。
	// いずれかが未準備の場合はカテゴリ自体を生成せずに中断する。
	if (GEditor == nullptr)
	{
		UE_LOG(LogLeeSplitClimbMapEditor, Warning, TEXT("RegisterNativePlacement skipped: GEditor is not ready yet"));
		return;
	}
	// 2026.11.19 Lee start
	/*	UActorFactory* MeshFactory = GEditor->FindActorFactoryByClass(UActorFactoryStaticMesh::StaticClass());
	if (MeshFactory == nullptr)
	{
		UE_LOG(LogLeeSplitClimbMapEditor, Warning, TEXT("RegisterNativePlacement skipped: UActorFactoryStaticMesh not found"));
		return;
	}

	*/
	UPlacementSubsystem* PlacementSubsystem = GEditor->GetEditorSubsystem<UPlacementSubsystem>();
	if (!PlacementSubsystem) { return; }
	// 2026.11.19 Lee end
	IPlacementModeModule& PlacementMode = IPlacementModeModule::Get();


	// カテゴリ登録（アセット欠損時でもカテゴリ自体は表示される）
	FPlacementCategoryInfo CategoryInfo(
		NSLOCTEXT("SplitClimbMapEditor", "PlacementCategory", "SplitClimb 岩石"),
		NativePlacementCategoryName,
		TEXT("SplitClimbRocks"),
		/*SortOrder*/ 90,
		/*bSortable*/ true);
	if (!PlacementMode.RegisterPlacementCategory(CategoryInfo))
	{
		UE_LOG(LogLeeSplitClimbMapEditor, Warning, TEXT("RegisterPlacementCategory failed: %s"), *NativePlacementCategoryName.ToString());
		return;
	}
	PlacementCategoryHandle = NativePlacementCategoryName;

	for (const FLeeNativeRockEntry& Entry : NativeRockEntries)
	{
		UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, Entry.AssetPath);
		if (!LoadedMesh)
		{
			UE_LOG(LogLeeSplitClimbMapEditor, Warning, TEXT("Rock asset missing, skip: %s"), Entry.AssetPath);
			continue;
		}
		// 2026.11.19 Lee start
		const auto MeshFactory = PlacementSubsystem->FindAssetFactoryFromAssetData(FAssetData(LoadedMesh));
		if (!MeshFactory) { continue; }
		// 2026.11.19 Lee end
		TSharedRef<FPlaceableItem> Item = MakeShared<FPlaceableItem>(
			MeshFactory,
			FAssetData(LoadedMesh),
			TOptional<int32>(Entry.SortOrder));
		TOptional<FPlacementModeID> ItemID = PlacementMode.RegisterPlaceableItem(NativePlacementCategoryName, Item);
		if (ItemID.IsSet())
		{
			PlacementItemIDs.Add(ItemID.GetValue());
		}
	}
	// 2026.11.19 Lee start
	UE_LOG(LogLeeSplitClimbMapEditor, Log, TEXT("Registered %d rock placement items"), PlacementItemIDs.Num());
	// 2026.11.19 Lee end
}

void FLeeSplitClimbMapEditorModule::UnregisterNativePlacement()
{
	if (PlacementCategoryHandle == NAME_None)
	{
		return;
	}

	// シャットダウン順序によって PlacementMode が既に unload されている場合は触らない
	if (!IPlacementModeModule::IsAvailable())
	{
		PlacementItemIDs.Reset();
		PlacementCategoryHandle = NAME_None;
		return;
	}

	IPlacementModeModule& PlacementMode = IPlacementModeModule::Get();
	for (int32 Index = PlacementItemIDs.Num() - 1; Index >= 0; --Index)
	{
		PlacementMode.UnregisterPlaceableItem(PlacementItemIDs[Index]);
	}
	PlacementItemIDs.Reset();
	PlacementMode.UnregisterPlacementCategory(PlacementCategoryHandle);
	PlacementCategoryHandle = NAME_None;
}

// 2026.11.25 Lee start
/** @brief PIE ワールドのポストアクタ tick 通知を受け、抓点デバッグ補助輪の描画を行う。
 *
 *  @param World        tick を終えたワールド
 *  @param TickType     ワールドの tick 種別（使用しない）
 *  @param DeltaSeconds 前フレームからの経過時間（使用しない）
 */
void FLeeSplitClimbMapEditorModule::HandleWorldPostActorTick(UWorld* World, ELevelTick /*TickType*/, float /*DeltaSeconds*/)
{
	// PIE ワールドのみ対象（エディタ編集ワールドには描画しない。
	// 攀爬キャラクター不在の PIE も対象に含める）
	if (!World || World->WorldType != EWorldType::PIE)
	{
		return;
	}

	// 専用サーバーでは描画しない（描画済みのローカルクライアントへの転送を避ける）
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// コンソール変数 splitclimb.DebugGrabHolds が 0 のときは何も描かない
	if (CVarDebugGrabHolds.GetValueOnGameThread() <= 0)
	{
		return;
	}

	DrawGrabHoldDebug(World);
}

/** @brief クエリ可能な抓点（BP_ClimbHold 生成クラスとその子クラス）の正面へ補助輪と四隅マークを描画する。
 *
 *        毎フレームの非永続描画で、停止後に痕を残さない。深度テストは通常の
 *        ワールド優先度（SDPG_World）で、裏面や他の岩体に隠れた部分は隠れる。
 *
 *  @param World 描画対象の PIE ワールド
 */
void FLeeSplitClimbMapEditorModule::DrawGrabHoldDebug(UWorld* World)
{
	// 毎フレームのアセットロードや全 Actor 走査を避ける：BP 生成クラスの
	// オブジェクトパス（LeeMapEditor::HoldBlueprintPath + "_C"）を FindObject でのみ解決する。
	// PIE の Actor は外部生成クラスを参照するため、インスタンスが存在すれば
	// クラスは必ずロード済み。見つからない（抓点未配置・BP 未ロード）場合は描画せず終わる
	const FString GeneratedClassPath = LeeMapEditor::HoldBlueprintPath + TEXT("_C");
	UClass* HoldClass = FindObject<UClass>(nullptr, *GeneratedClassPath);
	if (!HoldClass
		|| HoldClass->HasAnyClassFlags(CLASS_Abstract)
		|| !HoldClass->IsChildOf(AActor::StaticClass()))
	{
		return;
	}

	// 生成クラスで限定したイテレータ（子クラスも含む）
	for (TActorIterator<AActor> It(World, HoldClass); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor->IsPendingKillPending() || Actor->HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject))
		{
			continue;
		}

		// 真のルート UBoxComponent を持つこと（TriggerBox 由来の実配置盒体）
		UBoxComponent* BoxComp = Cast<UBoxComponent>(Actor->GetRootComponent());
		if (!BoxComp)
		{
			continue;
		}

		// 有効な抓点のみ描く：Actor のコリジョン有効・コンポーネント登録済み・
		// クエリ衝突有効・Climbable（ECC_GameTraceChannel1）を Block する。
		// GripStrong / GripWeak タグは要求しない（無タグの有効抓点を落とさない）
		if (!Actor->GetActorEnableCollision()
			|| !BoxComp->IsRegistered()
			|| !BoxComp->IsQueryCollisionEnabled()
			|| BoxComp->GetCollisionResponseToChannel(ECC_GameTraceChannel1) != ECR_Block)
		{
			continue;
		}

		// 盒体の実変換・実寸法を使う。半寸法はスケール込みで X=幅, Y=厚さ（+Y が外向き法線）, Z=高さ
		const FTransform BoxTransform = BoxComp->GetComponentTransform();
		const FVector HalfExtent = BoxComp->GetScaledBoxExtent();
		const double RadiusX = FMath::Abs(HalfExtent.X);
		const double RadiusZ = FMath::Abs(HalfExtent.Z);
		if (RadiusX <= 0.0 || RadiusZ <= 0.0)
		{
			continue;
		}

		const FVector AxisX = BoxTransform.GetUnitAxis(EAxis::X);
		const FVector AxisY = BoxTransform.GetUnitAxis(EAxis::Y);
		const FVector AxisZ = BoxTransform.GetUnitAxis(EAxis::Z);

		// 正面（ローカル +Y 面）の中心：盒体中心 + 法線方向へ厚み半分 + 軽微なリフト
		// （岩面や盒体面との同面重なりを避ける）
		const FVector FaceCenter = BoxTransform.GetTranslation()
			+ AxisY * (FMath::Abs(HalfExtent.Y) + GrabHoldDebugLiftCm);

		// 正面の X/Z 半寸法に沿った楕円を短線分の連なりで描く
		FVector PrevPoint = FaceCenter + AxisX * RadiusX;
		for (int32 Segment = 1; Segment <= GrabHoldDebugSegments; ++Segment)
		{
			const double Theta = static_cast<double>(Segment) / GrabHoldDebugSegments * (2.0 * PI);
			const FVector Point = FaceCenter
				+ AxisX * (RadiusX * FMath::Cos(Theta))
				+ AxisZ * (RadiusZ * FMath::Sin(Theta));
			DrawDebugLine(World, PrevPoint, Point, GrabHoldDebugColor,
				/*bPersistent*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ SDPG_World, /*Thickness*/ GrabHoldDebugThickness);
			PrevPoint = Point;
		}

		// 矩形の実四隅に内側向きの短い直角マークを描く（内外の角範囲を明確にする）
		const double MarkLenX = FMath::Min(RadiusX * 0.35, GrabHoldDebugMarkLenCm);
		const double MarkLenZ = FMath::Min(RadiusZ * 0.35, GrabHoldDebugMarkLenCm);
		for (int32 SignX = -1; SignX <= 1; SignX += 2)
		{
			for (int32 SignZ = -1; SignZ <= 1; SignZ += 2)
			{
				const FVector Corner = FaceCenter
					+ AxisX * (RadiusX * SignX)
					+ AxisZ * (RadiusZ * SignZ);
				const FVector InnerX = Corner - AxisX * (MarkLenX * SignX);
				const FVector InnerZ = Corner - AxisZ * (MarkLenZ * SignZ);
				DrawDebugLine(World, Corner, InnerX, GrabHoldDebugColor,
					/*bPersistent*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ SDPG_World, /*Thickness*/ GrabHoldDebugThickness);
				DrawDebugLine(World, Corner, InnerZ, GrabHoldDebugColor,
					/*bPersistent*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ SDPG_World, /*Thickness*/ GrabHoldDebugThickness);
			}
		}
	}
}
// 2026.11.25 Lee end
