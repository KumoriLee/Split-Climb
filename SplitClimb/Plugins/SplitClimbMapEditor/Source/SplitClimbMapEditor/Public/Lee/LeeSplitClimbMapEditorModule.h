#pragma once

#include "CoreMinimal.h"
// 2026.11.25 Lee start
#include "Engine/World.h"
// 2026.11.25 Lee end
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Docking/SDockTab.h"

class FLeeMapEditorTool;
class FPlacementModeID;

/** @brief SplitClimb マップエディタプラグインのログカテゴリ。 */
DECLARE_LOG_CATEGORY_EXTERN(LogLeeSplitClimbMapEditor, Log, All);

/**
 * @brief SplitClimb 用マップエディタプラグインモジュール（エディタ専用）。
 *
 *        エディタモードの登録・ドッキングパネル（タブ）の提供・メニュー項目の拡張・
 *        原生配置（Place Actors パネル）カテゴリとアイテムの登録/解除を行う。
 *        ゲームモジュール（SplitClimb）への依存は持たない。
 */
class FLeeSplitClimbMapEditorModule : public IModuleInterface
{
public:
	//~ IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** @brief ツール実体（配置状態・パラメータの保持者）を返す。 */
	TSharedRef<FLeeMapEditorTool> GetTool() const;

private:
	/** @brief パネルタブを生成する。 */
	TSharedRef<SDockTab> SpawnPanelTab(const FSpawnTabArgs& InSpawnArgs);

	/** @brief パネルタブを前面に開く。 */
	void OpenPanelTab();

	/** @brief ToolMenus の起動後にメニュー項目の拡張と原生配置登録を行う。 */
	void RegisterMenus();

	/** @brief Place Actors パネルへ「SplitClimb 岩石」カテゴリと実在 4 アセットを登録する。 */
	void RegisterNativePlacement();

	/** @brief 原生配置カテゴリとアイテムの登録を解除する。 */
	void UnregisterNativePlacement();

	// 2026.11.25 Lee start
	/** @brief PIE ワールドのポストアクタ tick で抓点デバッグ補助輪の描画を行う。 */
	void HandleWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);

	/** @brief クエリ可能な抓点（BP_ClimbHold 生成クラスとその子クラス）の正面へ補助輪を描画する。 */
	void DrawGrabHoldDebug(UWorld* World);
	// 2026.11.25 Lee end

	/** @brief ツール実体。 */
	TSharedPtr<FLeeMapEditorTool> ToolInstance;

	/** @brief ToolMenus 起動コールバックのハンドル。 */
	FDelegateHandle ToolMenusStartupHandle;
	// 2026.11.19 Lee start
	/** @brief エディタ初期化完了通知の解除用ハンドル。 */
	FDelegateHandle EditorInitializedHandle;
	// 2026.11.19 Lee end
	// 2026.11.25 Lee start
	/** @brief OnWorldPostActorTick 通知の解除用ハンドル。 */
	FDelegateHandle WorldPostActorTickHandle;
	// 2026.11.25 Lee end

	/** @brief 登録した原生配置アイテムの ID（登録解除用）。 */
	TArray<FPlacementModeID> PlacementItemIDs;

	/** @brief 原生配置カテゴリのハンドル名。 */
	FName PlacementCategoryHandle;
};

/** @brief モジュールの共有ツール実体を取得する（モード生成時に使用）。 */
SPLITCLIMBMAPEDITOR_API TSharedRef<FLeeMapEditorTool> GetLeeSplitClimbMapEditorTool();
