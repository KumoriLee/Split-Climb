#include "Lee/LeeSplitClimbMapEditorModule.h"

#include "Lee/LeeMapEditorMode.h"
#include "Lee/LeeMapEditorTool.h"
#include "Lee/SLeeMapEditorPanel.h"

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
	UnregisterNativePlacement();

	if (UToolMenus::TryGet())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
	}
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
