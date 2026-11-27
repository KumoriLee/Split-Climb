#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Views/SListView.h"

#include "Lee/LeeMapEditorTypes.h"

class FLeeMapEditorTool;
class STextBlock;

/** @brief 抓点リストの 1 行分のデータ。 */
struct FLeeHoldListRow
{
	TWeakObjectPtr<AActor> Actor; //!< 対応する抓点アクタ
	FString Label;                //!< アクタラベル
	FString Tags;                 //!< 強弱タグ
	FString Position;             //!< 位置（XYZ）
};
typedef TSharedPtr<FLeeHoldListRow> FLeeHoldListRowPtr;

/** @brief 原生配置コンボボックス用のラップ。 */
typedef TSharedPtr<FLeeRockAssetEntry> FLeeRockEntryPtr;

/** @brief 参照モードコンボボックス用のラップ。 */
typedef TSharedPtr<LeeMapEditor::ELeeRefMode> FLeeRefModePtr;

/** @brief 検査モードコンボボックス用のラップ。 */
typedef TSharedPtr<LeeMapEditor::ELeeCheckMode> FLeeCheckModePtr;

/**
 * @brief SplitClimb マップエディタの中国語パネル（ドッキング可能なタブ）。
 *
 *        配置パラメータ・拾取・適用・測量・検査・抓点リストを
 *        一つのスクロールパネルにまとめる。状態表示は 0.25 秒間隔で更新する。
 */
class SLeeMapEditorPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLeeMapEditorPanel) {}
	SLATE_END_ARGS()

	/**
	 * @brief パネルを構築する。
	 * @param InTool 共有ツール実体
	 */
	void Construct(const FArguments& InArgs, TSharedRef<FLeeMapEditorTool> InTool);

	/** @brief デストラクタ。状態通知の購読を解除する。 */
	virtual ~SLeeMapEditorPanel() override;

private:
	/** @brief 各種表示テキストとチェック状態をツールの現在値へ同期する。 */
	void UpdateDynamicText();

	/** @brief 抓点リストを再構築する。 */
	void RefreshHoldList();

	/** @brief 刷新ボタン（FReply 版）。 */
	FReply HandleRefreshList();

	/** @brief アクティブタイマーのコールバック。 */
	EActiveTimerReturnType HandlePanelTick(double InCurrentTime, float InDeltaTime);

	// UI ハンドラ
	void HandleArmToggled(ECheckBoxState NewState);
	void HandlePlacementKindToggled(ECheckBoxState NewState, LeeMapEditor::ELeePlacementKind InKind);
	void HandleHoldTypeToggled(ECheckBoxState NewState, LeeMapEditor::ELeeHoldType InType);
	FReply HandleRestorePreset();
	void HandleOffsetUChanged(double InValue);
	void HandleOffsetVChanged(double InValue);
	void HandleAngleChanged(double InValue);
	void HandleOffsetNChanged(double InValue);
	void HandleSizeXChanged(double InValue);
	void HandleSizeYChanged(double InValue);
	void HandleSizeZChanged(double InValue);
	void HandleStepChanged(double InValue);
	FReply HandleApply();
	FReply HandleCancel();
	FReply HandlePickRock();
	FReply HandlePickHold();
	FReply HandlePickReference();
	FReply HandleClearAttach();
	FReply HandleClearRefs();
	FReply HandleRunCheck();
	FReply HandleRunReverseProbe();
	void HandleRockSelected(FLeeRockEntryPtr InEntry, ESelectInfo::Type InSelectInfo);
	void HandleRefModeSelected(FLeeRefModePtr InMode, ESelectInfo::Type InSelectInfo);
	void HandleCheckModeSelected(FLeeCheckModePtr InMode, ESelectInfo::Type InSelectInfo);
	TSharedRef<SWidget> MakeRockOptionWidget(FLeeRockEntryPtr InEntry);
	TSharedRef<SWidget> MakeRefModeOptionWidget(FLeeRefModePtr InMode);
	TSharedRef<SWidget> MakeCheckModeOptionWidget(FLeeCheckModePtr InMode);
	FReply HandleLocateHoldClicked(FLeeHoldListRowPtr InRow);
	FReply HandleEditHoldClicked(FLeeHoldListRowPtr InRow);
	void HandleRowReferenceToggled(ECheckBoxState NewState, FLeeHoldListRowPtr InRow);
	TSharedRef<ITableRow> OnGenerateHoldRow(FLeeHoldListRowPtr InRow, const TSharedRef<STableViewBase>& OwnerTable);

	/** @brief 共有ツール実体。 */
	TSharedPtr<FLeeMapEditorTool> Tool;

	/** @brief 状態変化購読ハンドル。 */
	FDelegateHandle StateChangedHandle;

	// 動的ウィジェット参照（UpdateDynamicText で更新する）
	TSharedPtr<SCheckBox> ArmCheckBox;          //!< 工具有効チェック
	TSharedPtr<SCheckBox> HoldKindCheckBox;     //!< 抓点配置チェック
	TSharedPtr<SCheckBox> RockKindCheckBox;     //!< 岩体配置チェック
	TSharedPtr<SCheckBox> StrongTypeCheckBox;   //!< 強チェック
	TSharedPtr<SCheckBox> WeakTypeCheckBox;     //!< 弱チェック
	TSharedPtr<SSpinBox<double>> OffsetUSpin;   //!< 横方向オフセット
	TSharedPtr<SSpinBox<double>> OffsetVSpin;   //!< 縦方向オフセット
	TSharedPtr<SSpinBox<double>> AngleSpin;     //!< 法線回り回転角
	TSharedPtr<SSpinBox<double>> OffsetNSpin;   //!< 面外オフセット
	TSharedPtr<SSpinBox<double>> SizeXSpin;     //!< 幅
	TSharedPtr<SSpinBox<double>> SizeYSpin;     //!< 厚さ
	TSharedPtr<SSpinBox<double>> SizeZSpin;     //!< 高さ
	TSharedPtr<SSpinBox<double>> StepSpin;      //!< 接線ステップ
	TSharedPtr<STextBlock> SizeText;            //!< 現在寸法表示
	TSharedPtr<STextBlock> ResultText;          //!< 適用・操作結果表示
	TSharedPtr<STextBlock> MeasureText;         //!< 測量結果表示
	TSharedPtr<STextBlock> CheckText;           //!< 検査結果表示
	TSharedPtr<STextBlock> RockComboText;       //!< 原生配置の選択表示
	TSharedPtr<STextBlock> RefModeComboText;    //!< 参照モードの選択表示
	TSharedPtr<STextBlock> CheckModeComboText;  //!< 検査モードの選択表示
	TSharedPtr<SListView<FLeeHoldListRowPtr>> HoldListView; //!< 抓点リスト
	TArray<FLeeHoldListRowPtr> HoldRows;        //!< 抓点リストの行データ
	TArray<FLeeRockEntryPtr> RockOptions;       //!< 原生配置オプション
	TArray<FLeeRefModePtr> RefModeOptions;      //!< 参照モードオプション
	TArray<FLeeCheckModePtr> CheckModeOptions;  //!< 検査モードオプション
};
