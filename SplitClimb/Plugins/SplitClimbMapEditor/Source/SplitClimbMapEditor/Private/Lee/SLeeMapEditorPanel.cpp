#include "Lee/SLeeMapEditorPanel.h"

#include "Lee/LeeSplitClimbMapEditorModule.h"
#include "Lee/LeeMapEditorTool.h"
// 2026.11.25 Lee start
#include "Engine/StaticMesh.h"
// 2026.11.25 Lee end

#include "Types/SlateEnums.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "LeeSplitClimbMapEditorPanel"

namespace
{
	/** @brief 区切り見出しテキストを生成する。 */
	TSharedRef<SWidget> MakeSectionHeader(const FText& InText)
	{
		return SNew(STextBlock)
			.Text(InText)
			.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
	}

	/** @brief ラベル付きスピンボックス行を生成する。 */
	TSharedRef<SWidget> MakeSpinBoxRow(const FText& InLabel, const TSharedRef<SSpinBox<double>>& InSpinBox, const FText& InUnit)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(InLabel)
				.MinDesiredWidth(110.0f)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				InSpinBox
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(InUnit)
			];
	}

	/** @brief チェックボックス（テキストラベル付き）を生成する。 */
	TSharedRef<SCheckBox> MakeLabelCheckBox(const TSharedRef<STextBlock>& LabelWidget)
	{
		return SNew(SCheckBox)
			[
				LabelWidget
			];
	}

	/** @brief 汎用ボタンを生成する。 */
	TSharedRef<SButton> MakePanelButton(const FText& InText, const FOnClicked& InOnClicked)
	{
		return SNew(SButton)
			.Text(InText)
			.OnClicked(InOnClicked);
	}
}

void SLeeMapEditorPanel::Construct(const FArguments& InArgs, TSharedRef<FLeeMapEditorTool> InTool)
{
	Tool = InTool;

	// コンボボックス用のオプションを作成する
	for (const FLeeRockAssetEntry& Entry : FLeeMapEditorTool::GetRockEntries())
	{
		RockOptions.Add(MakeShared<FLeeRockAssetEntry>(Entry));
	}
	RefModeOptions.Add(MakeShared<LeeMapEditor::ELeeRefMode>(LeeMapEditor::ELeeRefMode::Nearest3));
	RefModeOptions.Add(MakeShared<LeeMapEditor::ELeeRefMode>(LeeMapEditor::ELeeRefMode::Specified));
	CheckModeOptions.Add(MakeShared<LeeMapEditor::ELeeCheckMode>(LeeMapEditor::ELeeCheckMode::Climbable));
	CheckModeOptions.Add(MakeShared<LeeMapEditor::ELeeCheckMode>(LeeMapEditor::ELeeCheckMode::BlockPawn));
	CheckModeOptions.Add(MakeShared<LeeMapEditor::ELeeCheckMode>(LeeMapEditor::ELeeCheckMode::IgnoreCollision));

	// スピンボックス（ステップはツールの現在値に連動）
	SAssignNew(OffsetUSpin, SSpinBox<double>)
		.MinValue(-100000.0)
		.MaxValue(100000.0)
		.Delta_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee start
		// .Value(Tool->GetOffsetU())
		.Value_Lambda([this]() { return Tool->GetOffsetU(); })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleOffsetUChanged);
	SAssignNew(OffsetVSpin, SSpinBox<double>)
		.MinValue(-100000.0)
		.MaxValue(100000.0)
		.Delta_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee start
		// .Value(Tool->GetOffsetV())
		.Value_Lambda([this]() { return Tool->GetOffsetV(); })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleOffsetVChanged);
	SAssignNew(AngleSpin, SSpinBox<double>)
		.MinValue(-360.0)
		.MaxValue(360.0)
		.Delta_Lambda([]() { return LeeMapEditor::DefaultAngleStepDeg; })
		// 2026.11.19 Lee start
		// .Value(Tool->GetAngleDeg())
		.Value_Lambda([this]() { return Tool->GetAngleDeg(); })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleAngleChanged);
	SAssignNew(OffsetNSpin, SSpinBox<double>)
		.MinValue(-10000.0)
		.MaxValue(10000.0)
		.Delta_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee start
		// .Value(Tool->GetOffsetN())
		.Value_Lambda([this]() { return Tool->GetOffsetN(); })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleOffsetNChanged);
	SAssignNew(SizeXSpin, SSpinBox<double>)
		.MinValue(1.0)
		.MaxValue(100000.0)
		.Delta_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee start
		// .Value(Tool->GetSize().X)
		.Value_Lambda([this]() { return Tool->GetSize().X; })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleSizeXChanged);
	SAssignNew(SizeYSpin, SSpinBox<double>)
		.MinValue(1.0)
		.MaxValue(100000.0)
		.Delta_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee start
		// .Value(Tool->GetSize().Y)
		.Value_Lambda([this]() { return Tool->GetSize().Y; })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleSizeYChanged);
	SAssignNew(SizeZSpin, SSpinBox<double>)
		.MinValue(1.0)
		.MaxValue(100000.0)
		.Delta_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee start
		// .Value(Tool->GetSize().Z)
		.Value_Lambda([this]() { return Tool->GetSize().Z; })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleSizeZChanged);
	SAssignNew(StepSpin, SSpinBox<double>)
		.MinValue(0.1)
		.MaxValue(100.0)
		.Delta(1.0)
		// 2026.11.19 Lee start
		// .Value(Tool->GetStepCm())
		.Value_Lambda([this]() { return Tool->GetStepCm(); })
		// 2026.11.19 Lee end
		.OnValueChanged(this, &SLeeMapEditorPanel::HandleStepChanged);

	ChildSlot
	[
		SNew(SScrollBox)
		// 工具有効化
		+ SScrollBox::Slot().Padding(6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SAssignNew(ArmCheckBox, SCheckBox)
				.OnCheckStateChanged(this, &SLeeMapEditorPanel::HandleArmToggled)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "ArmTool", "启用视口放置工具"))
				]
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
			.Text(NSLOCTEXT("SplitClimbMapEditor", "ArmHint", "启用后：左键点击岩面建立盒预览，按住左键贴面滑动，点击其他表面重新选面，Esc 取消预览"))
			.AutoWrapText(true)
		]

		// 配置対象
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionPlacement", "放置对象"))
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 12.0f, 2.0f)
			[
				SAssignNew(HoldKindCheckBox, SCheckBox)
				.OnCheckStateChanged(this, &SLeeMapEditorPanel::HandlePlacementKindToggled, LeeMapEditor::ELeePlacementKind::Hold)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "KindHold", "抓点"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f)
			[
				SAssignNew(RockKindCheckBox, SCheckBox)
				.OnCheckStateChanged(this, &SLeeMapEditorPanel::HandlePlacementKindToggled, LeeMapEditor::ELeePlacementKind::Rock)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "KindRock", "岩体（原生放置）"))
				]
			]
		]

		// 抓点強弱
		+ SScrollBox::Slot().Padding(6.0f, 4.0f, 6.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 12.0f, 2.0f)
			[
				SAssignNew(StrongTypeCheckBox, SCheckBox)
				.OnCheckStateChanged(this, &SLeeMapEditorPanel::HandleHoldTypeToggled, LeeMapEditor::ELeeHoldType::Strong)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "HoldStrong", "强（44×26×56）"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 12.0f, 2.0f)
			[
				SAssignNew(WeakTypeCheckBox, SCheckBox)
				.OnCheckStateChanged(this, &SLeeMapEditorPanel::HandleHoldTypeToggled, LeeMapEditor::ELeeHoldType::Weak)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "HoldWeak", "弱（300×40×36）"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "RestorePreset", "恢复预设尺寸"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleRestorePreset)
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SAssignNew(SizeText, STextBlock)
			.Text(NSLOCTEXT("SplitClimbMapEditor", "SizePlaceholder", "当前尺寸：-"))
		]

		// 数値パラメータ
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionParams", "调整参数"))
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "OffsetULabel", "横向偏移"), OffsetUSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm", "cm"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "OffsetVLabel", "纵向偏移"), OffsetVSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm2", "cm"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "AngleLabel", "绕法线角"), AngleSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitDeg", "°"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "OffsetNLabel", "离面偏移"), OffsetNSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm3", "cm"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "SizeXLabel", "宽"), SizeXSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm4", "cm"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "SizeYLabel", "厚"), SizeYSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm5", "cm"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "SizeZLabel", "高"), SizeZSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm6", "cm"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSpinBoxRow(NSLOCTEXT("SplitClimbMapEditor", "StepLabel", "步进"), StepSpin.ToSharedRef(), NSLOCTEXT("SplitClimbMapEditor", "UnitCm7", "cm"))
			]
		]

		// 適用
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionApply", "应用"))
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "Apply", "应用（生成 / 修改）"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleApply)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "CancelPreview", "取消（Esc）"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleCancel)
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SAssignNew(ResultText, STextBlock)
			.AutoWrapText(true)
			.Text(NSLOCTEXT("SplitClimbMapEditor", "ResultPlaceholder", " "))
		]

		// 拾取
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionPick", "拾取"))
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(SWrapBox)
			.UseAllottedSize(true)
			+ SWrapBox::Slot().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "PickRock", "拾取贴附岩体"))
				.ToolTipText(NSLOCTEXT("SplitClimbMapEditor", "PickRockTip", "点击后在视口中点击岩体表面，作为独立抓点的显式贴附目标"))
				.OnClicked(this, &SLeeMapEditorPanel::HandlePickRock)
			]
			+ SWrapBox::Slot().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "PickHold", "拾取已有抓点编辑"))
				.ToolTipText(NSLOCTEXT("SplitClimbMapEditor", "PickHoldTip", "点击后在视口中点击已有抓点载入编辑"))
				.OnClicked(this, &SLeeMapEditorPanel::HandlePickHold)
			]
			+ SWrapBox::Slot().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "PickRef", "拾取参照抓点"))
				.ToolTipText(NSLOCTEXT("SplitClimbMapEditor", "PickRefTip", "点击后在视口中点击抓点作为测量参照"))
				.OnClicked(this, &SLeeMapEditorPanel::HandlePickReference)
			]
			+ SWrapBox::Slot().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "ClearAttach", "清除贴附指定"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleClearAttach)
			]
			+ SWrapBox::Slot().Padding(0.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "ClearRefs", "清除参照"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleClearRefs)
			]
		]

		// 測量
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionMeasure", "测量"))
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f, 6.0f, 4.0f)
		[
			SNew(SComboBox<FLeeRefModePtr>)
			.OptionsSource(&RefModeOptions)
			.OnSelectionChanged(this, &SLeeMapEditorPanel::HandleRefModeSelected)
			.OnGenerateWidget(this, &SLeeMapEditorPanel::MakeRefModeOptionWidget)
			[
				SAssignNew(RefModeComboText, STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "RefModeCurrent", "最近三个抓点（忽略自身）"))
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SAssignNew(MeasureText, STextBlock)
			.AutoWrapText(true)
			.Text(NSLOCTEXT("SplitClimbMapEditor", "MeasurePlaceholder", "建立预览后显示到最近抓点的距离与高差"))
		]

		// 検査
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionCheck", "检查盒"))
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f, 6.0f, 4.0f)
		[
			SNew(SComboBox<FLeeCheckModePtr>)
			.OptionsSource(&CheckModeOptions)
			.OnSelectionChanged(this, &SLeeMapEditorPanel::HandleCheckModeSelected)
			.OnGenerateWidget(this, &SLeeMapEditorPanel::MakeCheckModeOptionWidget)
			[
				SAssignNew(CheckModeComboText, STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "CheckModeCurrent", "Climbable 通道查询"))
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "RunCheck", "运行检查"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleRunCheck)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "ReverseProbe", "外侧反查命中"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleRunReverseProbe)
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SAssignNew(CheckText, STextBlock)
			.AutoWrapText(true)
			.Text(NSLOCTEXT("SplitClimbMapEditor", "CheckPlaceholder", "尚未运行检查"))
		]

		// 抓点リスト
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionHoldList", "当前关卡抓点"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "RefreshList", "刷新"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleRefreshList)
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SAssignNew(HoldListView, SListView<FLeeHoldListRowPtr>)
			.ListItemsSource(&HoldRows)
			.OnGenerateRow(this, &SLeeMapEditorPanel::OnGenerateHoldRow)
			.SelectionMode(ESelectionMode::None)
		]

		// 原生配置
		+ SScrollBox::Slot().Padding(6.0f, 8.0f, 6.0f, 2.0f)
		[
			MakeSectionHeader(NSLOCTEXT("SplitClimbMapEditor", "SectionRock", "原生放置（LP_RocksandCliffs）"))
		]
		// 2026.11.25 Lee start
		+ SScrollBox::Slot().Padding(6.0f, 0.0f, 6.0f, 4.0f)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("SplitClimbMapEditor", "OpenRockLibrary", "打开岩壁素材窗口"))
			.OnClicked_Lambda([]
			{
				FGlobalTabmanager::Get()->TryInvokeTab(LeeMapEditor::RockLibraryTabID);
				return FReply::Handled();
			})
		]
		// 2026.11.25 Lee end
		+ SScrollBox::Slot().Padding(6.0f, 0.0f, 6.0f, 4.0f)
		[
			SNew(SComboBox<FLeeRockEntryPtr>)
			.OptionsSource(&RockOptions)
			.OnSelectionChanged(this, &SLeeMapEditorPanel::HandleRockSelected)
			.OnGenerateWidget(this, &SLeeMapEditorPanel::MakeRockOptionWidget)
			[
				SAssignNew(RockComboText, STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "RockCurrent", "选择岩体资产"))
			]
		]
		+ SScrollBox::Slot().Padding(6.0f, 0.0f)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
			.AutoWrapText(true)
			.Text(NSLOCTEXT("SplitClimbMapEditor", "RockHint", "选择后切换到岩体模式，点击岩面生成网格包围盒预览，应用后挂到命中组件；资产缺失时类别仍会显示"))
		]
	];

	// 状態変化を購読して UI を同期する
	StateChangedHandle = Tool->OnStateChanged.Add(
		FLeeToolStateChanged::FDelegate::CreateSP(this, &SLeeMapEditorPanel::UpdateDynamicText));

	// 状態表示は 0.25 秒間隔で更新する
	RegisterActiveTimer(0.25f, FWidgetActiveTimerDelegate::CreateSP(this, &SLeeMapEditorPanel::HandlePanelTick));

	UpdateDynamicText();
	RefreshHoldList();
}

SLeeMapEditorPanel::~SLeeMapEditorPanel()
{
	if (Tool.IsValid())
	{
		Tool->OnStateChanged.Remove(StateChangedHandle);
	}
}

EActiveTimerReturnType SLeeMapEditorPanel::HandlePanelTick(double InCurrentTime, float InDeltaTime)
{
	UpdateDynamicText();
	return EActiveTimerReturnType::Continue;
}

void SLeeMapEditorPanel::UpdateDynamicText()
{
	if (!Tool.IsValid())
	{
		return;
	}

	ArmCheckBox->SetIsChecked(Tool->IsModeActive() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	HoldKindCheckBox->SetIsChecked(Tool->GetPlacementKind() == LeeMapEditor::ELeePlacementKind::Hold ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	RockKindCheckBox->SetIsChecked(Tool->GetPlacementKind() == LeeMapEditor::ELeePlacementKind::Rock ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	StrongTypeCheckBox->SetIsChecked(Tool->GetHoldType() == LeeMapEditor::ELeeHoldType::Strong ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	WeakTypeCheckBox->SetIsChecked(Tool->GetHoldType() == LeeMapEditor::ELeeHoldType::Weak ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

	// 2026.11.19 Lee start
	// OffsetUSpin->SetValue(Tool->GetOffsetU());
	// 2026.11.19 Lee end
	// 2026.11.19 Lee start
	// OffsetVSpin->SetValue(Tool->GetOffsetV());
	// 2026.11.19 Lee end
	// 2026.11.19 Lee start
	// AngleSpin->SetValue(Tool->GetAngleDeg());
	// 2026.11.19 Lee end
	// 2026.11.19 Lee start
	// OffsetNSpin->SetValue(Tool->GetOffsetN());
	// 2026.11.19 Lee end
	const FVector CurrentSize = Tool->GetSize();
	// 2026.11.19 Lee start
	// SizeXSpin->SetValue(CurrentSize.X);
	// 2026.11.19 Lee end
	// 2026.11.19 Lee start
	// SizeYSpin->SetValue(CurrentSize.Y);
	// 2026.11.19 Lee end
	// 2026.11.19 Lee start
	// SizeZSpin->SetValue(CurrentSize.Z);
	// 2026.11.19 Lee end
	// 2026.11.19 Lee start
	// StepSpin->SetValue(Tool->GetStepCm());
	// 2026.11.19 Lee end

	SizeText->SetText(FText::FromString(FString::Printf(TEXT("当前尺寸（宽×厚×高）：%.0f × %.0f × %.0f cm"), CurrentSize.X, CurrentSize.Y, CurrentSize.Z)));
	// 2026.11.25 Lee start
	const UStaticMesh* CurrentRockMesh = Tool->GetRockMesh();
	RockComboText->SetText(CurrentRockMesh
		? FText::FromName(CurrentRockMesh->GetFName())
		: NSLOCTEXT("SplitClimbMapEditor", "RockCurrent", "选择岩体资产"));
	// 2026.11.25 Lee end
	ResultText->SetText(FText::FromString(Tool->BuildStatusText()));

	// 測量表示
	FString MeasureLines;
	for (const LeeMapEditor::FLeeMeasureEntry& Entry : Tool->GetMeasurements())
	{
		const AActor* Target = Entry.Target.Get();
		if (!Target)
		{
			continue;
		}
		MeasureLines += FString::Printf(TEXT("→ %s%s：距离 %.1f cm，高差 %+.1f cm\n"),
			*Target->GetActorLabel(),
			Entry.bSpecified ? TEXT("（指定参照）") : TEXT(""),
			Entry.Distance, Entry.DeltaZ);
	}
	MeasureText->SetText(MeasureLines.IsEmpty()
		? NSLOCTEXT("SplitClimbMapEditor", "MeasureEmpty", "建立预览后显示到最近抓点的距离与高差")
		: FText::FromString(MeasureLines));

	// 検査表示
	const LeeMapEditor::FLeeCheckResult& CheckResult = Tool->GetLastCheckResult();
	if (!CheckResult.bRan)
	{
		CheckText->SetText(NSLOCTEXT("SplitClimbMapEditor", "CheckEmpty", "尚未运行检查"));
	}
	else
	{
		CheckText->SetText(FText::FromString(FString::Printf(TEXT("%s%s\n%s"),
			CheckResult.bSuccess ? TEXT("[通过] ") : TEXT("[注意] "),
			*CheckResult.FirstHitName,
			*CheckResult.Detail)));
	}
}

void SLeeMapEditorPanel::RefreshHoldList()
{
	HoldRows.Reset();
	for (AActor* HoldActor : Tool->CollectHoldActors())
	{
		const FLeeHoldListRowPtr Row = MakeShared<FLeeHoldListRow>();
		Row->Actor = HoldActor;
		Row->Label = HoldActor->GetActorLabel();
		Row->Tags = HoldActor->Tags.Contains(LeeMapEditor::HoldStrongTag) ? TEXT("GripStrong") : TEXT("GripWeak");
		const FVector Location = HoldActor->GetActorLocation();
		Row->Position = FString::Printf(TEXT("(%.0f, %.0f, %.0f)"), Location.X, Location.Y, Location.Z);
		HoldRows.Add(Row);
	}
	if (HoldListView.IsValid())
	{
		HoldListView->RequestListRefresh();
	}
}

FReply SLeeMapEditorPanel::HandleRefreshList()
{
	RefreshHoldList();
	return FReply::Handled();
}

void SLeeMapEditorPanel::HandleArmToggled(ECheckBoxState NewState)
{
	Tool->SetArmed(NewState == ECheckBoxState::Checked);
}

void SLeeMapEditorPanel::HandlePlacementKindToggled(ECheckBoxState NewState, LeeMapEditor::ELeePlacementKind InKind)
{
	if (NewState == ECheckBoxState::Checked)
	{
		Tool->SetPlacementKind(InKind);
	}
	UpdateDynamicText();
}

void SLeeMapEditorPanel::HandleHoldTypeToggled(ECheckBoxState NewState, LeeMapEditor::ELeeHoldType InType)
{
	if (NewState == ECheckBoxState::Checked)
	{
		Tool->SetHoldType(InType);
	}
	UpdateDynamicText();
}

FReply SLeeMapEditorPanel::HandleRestorePreset()
{
	Tool->RestorePresetSize();
	return FReply::Handled();
}

void SLeeMapEditorPanel::HandleOffsetUChanged(double InValue)
{
	Tool->SetOffsetU(InValue);
}

void SLeeMapEditorPanel::HandleOffsetVChanged(double InValue)
{
	Tool->SetOffsetV(InValue);
}

void SLeeMapEditorPanel::HandleAngleChanged(double InValue)
{
	Tool->SetAngleDeg(InValue);
}

void SLeeMapEditorPanel::HandleOffsetNChanged(double InValue)
{
	Tool->SetOffsetN(InValue);
}

void SLeeMapEditorPanel::HandleSizeXChanged(double InValue)
{
	Tool->SetSizeX(InValue);
}

void SLeeMapEditorPanel::HandleSizeYChanged(double InValue)
{
	Tool->SetSizeY(InValue);
}

void SLeeMapEditorPanel::HandleSizeZChanged(double InValue)
{
	Tool->SetSizeZ(InValue);
}

void SLeeMapEditorPanel::HandleStepChanged(double InValue)
{
	Tool->SetStepCm(InValue);
}

FReply SLeeMapEditorPanel::HandleApply()
{
	const FString ResultMessage = (Tool->GetPlacementKind() == LeeMapEditor::ELeePlacementKind::Rock)
		? Tool->ApplyRock()
		: Tool->ApplyHold();
	UE_LOG(LogLeeSplitClimbMapEditor, Log, TEXT("%s"), *ResultMessage);
	RefreshHoldList();
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandleCancel()
{
	Tool->CancelPreview();
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandlePickRock()
{
	Tool->SetPickMode(LeeMapEditor::ELeePickMode::Rock);
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandlePickHold()
{
	Tool->SetPickMode(LeeMapEditor::ELeePickMode::Hold);
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandlePickReference()
{
	Tool->SetPickMode(LeeMapEditor::ELeePickMode::Reference);
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandleClearAttach()
{
	Tool->ClearAttachOverride();
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandleClearRefs()
{
	Tool->ClearSpecifiedRefs();
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandleRunCheck()
{
	Tool->RunCheck();
	UpdateDynamicText();
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandleRunReverseProbe()
{
	Tool->RunReverseProbe();
	UpdateDynamicText();
	return FReply::Handled();
}

void SLeeMapEditorPanel::HandleRockSelected(FLeeRockEntryPtr InEntry, ESelectInfo::Type InSelectInfo)
{
	if (InEntry.IsValid())
	{
		Tool->SetRockMesh(*InEntry);
		// 2026.11.25 Lee start
		// RockComboText->SetText(FText::FromString(FString::Printf(TEXT("%s · %s"), *InEntry->CategoryLabel.ToString(), *InEntry->MeshName)));
		// 2026.11.25 Lee end
	}
	UpdateDynamicText();
}

void SLeeMapEditorPanel::HandleRefModeSelected(FLeeRefModePtr InMode, ESelectInfo::Type InSelectInfo)
{
	if (InMode.IsValid())
	{
		Tool->SetRefMode(*InMode);
		RefModeComboText->SetText((*InMode == LeeMapEditor::ELeeRefMode::Nearest3)
			? NSLOCTEXT("SplitClimbMapEditor", "RefNearest3", "最近三个抓点（忽略自身）")
			: NSLOCTEXT("SplitClimbMapEditor", "RefSpecified", "指定参照"));
	}
	UpdateDynamicText();
}

void SLeeMapEditorPanel::HandleCheckModeSelected(FLeeCheckModePtr InMode, ESelectInfo::Type InSelectInfo)
{
	if (InMode.IsValid())
	{
		Tool->SetCheckMode(*InMode);
		switch (*InMode)
		{
			case LeeMapEditor::ELeeCheckMode::Climbable:
				CheckModeComboText->SetText(NSLOCTEXT("SplitClimbMapEditor", "CheckClimbable", "Climbable 通道查询"));
				break;
			case LeeMapEditor::ELeeCheckMode::BlockPawn:
				CheckModeComboText->SetText(NSLOCTEXT("SplitClimbMapEditor", "CheckBlockPawn", "Pawn 阻挡检查"));
				break;
			case LeeMapEditor::ELeeCheckMode::IgnoreCollision:
				CheckModeComboText->SetText(NSLOCTEXT("SplitClimbMapEditor", "CheckIgnore", "忽略碰撞（几何相交）"));
				break;
		}
	}
	UpdateDynamicText();
}

TSharedRef<SWidget> SLeeMapEditorPanel::MakeRockOptionWidget(FLeeRockEntryPtr InEntry)
{
	if (!InEntry.IsValid())
	{
		return SNullWidget::NullWidget;
	}
	return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s · %s"), *InEntry->CategoryLabel.ToString(), *InEntry->MeshName)));
}

TSharedRef<SWidget> SLeeMapEditorPanel::MakeRefModeOptionWidget(FLeeRefModePtr InMode)
{
	if (!InMode.IsValid())
	{
		return SNullWidget::NullWidget;
	}
	return SNew(STextBlock).Text((*InMode == LeeMapEditor::ELeeRefMode::Nearest3)
		? NSLOCTEXT("SplitClimbMapEditor", "RefNearest3Opt", "最近三个抓点（忽略自身）")
		: NSLOCTEXT("SplitClimbMapEditor", "RefSpecifiedOpt", "指定参照"));
}

TSharedRef<SWidget> SLeeMapEditorPanel::MakeCheckModeOptionWidget(FLeeCheckModePtr InMode)
{
	if (!InMode.IsValid())
	{
		return SNullWidget::NullWidget;
	}
	switch (*InMode)
	{
		case LeeMapEditor::ELeeCheckMode::Climbable:
			return SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "CheckClimbableOpt", "Climbable 通道查询"));
		case LeeMapEditor::ELeeCheckMode::BlockPawn:
			return SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "CheckBlockPawnOpt", "Pawn 阻挡检查"));
		case LeeMapEditor::ELeeCheckMode::IgnoreCollision:
			return SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "CheckIgnoreOpt", "忽略碰撞（几何相交）"));
		default:
			return SNullWidget::NullWidget;
	}
}

FReply SLeeMapEditorPanel::HandleLocateHoldClicked(FLeeHoldListRowPtr InRow)
{
	if (InRow.IsValid() && InRow->Actor.IsValid())
	{
		Tool->LocateHold(InRow->Actor.Get());
	}
	return FReply::Handled();
}

FReply SLeeMapEditorPanel::HandleEditHoldClicked(FLeeHoldListRowPtr InRow)
{
	if (InRow.IsValid() && InRow->Actor.IsValid())
	{
		Tool->StartEditHold(InRow->Actor.Get());
	}
	UpdateDynamicText();
	return FReply::Handled();
}

void SLeeMapEditorPanel::HandleRowReferenceToggled(ECheckBoxState NewState, FLeeHoldListRowPtr InRow)
{
	if (!InRow.IsValid() || !InRow->Actor.IsValid())
	{
		return;
	}
	if (NewState == ECheckBoxState::Checked)
	{
		Tool->AddSpecifiedRef(InRow->Actor.Get());
	}
	else
	{
		Tool->RemoveSpecifiedRef(InRow->Actor.Get());
	}
	UpdateDynamicText();
}

TSharedRef<ITableRow> SLeeMapEditorPanel::OnGenerateHoldRow(FLeeHoldListRowPtr InRow, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsReference = Tool->GetSpecifiedRefs().Contains(InRow->Actor.Get());
	return SNew(STableRow<FLeeHoldListRowPtr>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(STextBlock).Text(FText::FromString(InRow->Label))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
				.Text(FText::FromString(InRow->Tags))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 2.0f, 8.0f, 2.0f)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
				.Text(FText::FromString(InRow->Position))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 2.0f, 4.0f, 2.0f)
			[
				SNew(SCheckBox)
				.IsChecked(bIsReference ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged(this, &SLeeMapEditorPanel::HandleRowReferenceToggled, InRow)
				[
					SNew(STextBlock).Text(NSLOCTEXT("SplitClimbMapEditor", "RowRef", "参照"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 2.0f, 4.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "RowLocate", "定位"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleLocateHoldClicked, InRow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 2.0f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("SplitClimbMapEditor", "RowEdit", "编辑"))
				.OnClicked(this, &SLeeMapEditorPanel::HandleEditHoldClicked, InRow)
			]
		];
}

#undef LOCTEXT_NAMESPACE
