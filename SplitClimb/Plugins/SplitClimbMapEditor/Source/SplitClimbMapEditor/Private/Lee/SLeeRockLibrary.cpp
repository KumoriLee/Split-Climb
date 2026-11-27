#include "Lee/SLeeRockLibrary.h"

#include "Lee/LeeMapEditorTool.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetThumbnail.h"
#include "ContentBrowserModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "IContentBrowserSingleton.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "LeeRockLibrary"

namespace
{
	const TCHAR* const LibrarySection = TEXT("RockLibrary");
	const TCHAR* const AssetPathsKey = TEXT("AssetPaths");
	const TCHAR* const InitializedKey = TEXT("Initialized");

	struct FRockRow
	{
		FString Path;
		FString Name;
		FAssetData Asset;
		TSharedPtr<FAssetThumbnail> Thumbnail;
	};

	using FRockRowPtr = TSharedPtr<FRockRow>;

	/** @brief 保存済みのメッシュ参照と一覧操作を管理する。 */
	class SLeeRockLibrary : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLeeRockLibrary) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<FLeeMapEditorTool>& InTool)
		{
			Tool = InTool;
			ThumbnailPool = MakeShared<FAssetThumbnailPool>(128);
			ConfigPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("SplitClimbRockLibrary.ini"));

			FConfigFile Config;
			if (IFileManager::Get().FileExists(*ConfigPath))
			{
				Config.Read(ConfigPath);
			}
			TArray<FString> Paths;
			bool bInitialized = false;
			Config.GetBool(LibrarySection, InitializedKey, bInitialized);
			if (bInitialized)
			{
				Config.GetArray(LibrarySection, AssetPathsKey, Paths);
			}
			else
			{
				for (const FLeeRockAssetEntry& Entry : FLeeMapEditorTool::GetRockEntries())
				{
					Paths.Add(Entry.AssetPath);
				}
			}
			for (const FString& Path : Paths)
			{
				if (!Path.IsEmpty())
				{
					Rows.Add(MakeRow(Path));
				}
			}

			ChildSlot
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 8)
				[
					SNew(SSearchBox)
					.HintText(LOCTEXT("Search", "搜索素材名称或路径"))
					.OnTextChanged(this, &SLeeRockLibrary::FilterChanged)
				]
				+ SVerticalBox::Slot().FillHeight(1).Padding(10, 0)
				[
					SNew(SBox).MinDesiredHeight(160)
					[
						SAssignNew(List, SListView<FRockRowPtr>)
						.ListItemsSource(&VisibleRows)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SLeeRockLibrary::GenerateRow)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 8, 10, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UStaticMesh::StaticClass())
						.ObjectPath_Lambda([this] { return Candidate.IsValid() ? Candidate.GetObjectPathString() : FString(); })
						.OnObjectChanged(this, &SLeeRockLibrary::CandidateChanged)
						.DisplayUseSelected(false)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0)
					[
						SNew(SButton).Text(LOCTEXT("UseBrowserSelection", "取内容浏览器选中项"))
						.OnClicked(this, &SLeeRockLibrary::PickBrowserSelection)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 0)
				[
					SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 4))
					+ SWrapBox::Slot()
					[
						SNew(SButton).Text(LOCTEXT("Add", "添加到列表"))
						.IsEnabled_Lambda([this] { return Candidate.IsValid(); })
						.OnClicked(this, &SLeeRockLibrary::AddMesh)
					]
					+ SWrapBox::Slot()
					[
						SNew(SButton).Text(LOCTEXT("Replace", "替换所选"))
						.IsEnabled_Lambda([this] { return Candidate.IsValid() && Selected().IsValid(); })
						.OnClicked(this, &SLeeRockLibrary::ReplaceMesh)
					]
					+ SWrapBox::Slot()
					[
						SNew(SButton).Text(LOCTEXT("Remove", "移出列表"))
						.ToolTipText(LOCTEXT("RemoveTip", "移除这条素材引用"))
						.IsEnabled_Lambda([this] { return Selected().IsValid(); })
						.OnClicked(this, &SLeeRockLibrary::RemoveMesh)
					]
					+ SWrapBox::Slot()
					[
						SNew(SButton).Text(LOCTEXT("Edit", "编辑原资产"))
						.IsEnabled_Lambda([this] { return CanUseSelection(); })
						.OnClicked(this, &SLeeRockLibrary::OpenMesh)
					]
					+ SWrapBox::Slot()
					[
						SNew(SButton).Text(LOCTEXT("Use", "用于摆放"))
						.IsEnabled_Lambda([this] { return CanUseSelection(); })
						.OnClicked(this, &SLeeRockLibrary::UseMesh)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 6, 10, 8)
				[
					SNew(STextBlock).AutoWrapText(true)
					.Text_Lambda([this] { return Status; })
				]
			];
			RefreshList();
			IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
			if (Registry.IsLoadingAssets())
			{
				Registry.OnFilesLoaded().AddSP(this, &SLeeRockLibrary::ResolveMissingRows);
			}
		}

	private:
		/** @brief 起動中に復元されたタブの未解決参照を再確認する。 */
		void ResolveMissingRows()
		{
			for (FRockRowPtr& Row : Rows)
			{
				if (!Row->Thumbnail)
				{
					Row = MakeRow(Row->Path);
				}
			}
			RefreshList();
		}

		/** @brief パスを解決し、未解決の場合は再指定できる行を生成する。 */
		FRockRowPtr MakeRow(const FString& Path) const
		{
			FRockRowPtr Row = MakeShared<FRockRow>();
			Row->Path = Path;
			const FSoftObjectPath SoftPath(Path);
			Row->Name = SoftPath.GetAssetName();
			const FAssetRegistryModule& Registry =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			Row->Asset = Registry.Get().GetAssetByObjectPath(SoftPath);
			if (Row->Asset.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
			{
				Row->Thumbnail = MakeShared<FAssetThumbnail>(Row->Asset, 64, 64, ThumbnailPool);
			}
			return Row;
		}

		TSharedRef<ITableRow> GenerateRow(FRockRowPtr Row, const TSharedRef<STableViewBase>& Owner)
		{
			const FText Name = FText::FromString(Row->Name.IsEmpty() ? Row->Path : Row->Name);
			return SNew(STableRow<FRockRowPtr>, Owner)
				.ToolTipText(FText::FromString(Row->Path))
				.OnDragDetected_Lambda([Row](const FGeometry&, const FPointerEvent& Event)
				{
					if (Row->Thumbnail && Event.IsMouseButtonDown(EKeys::LeftMouseButton))
					{
						return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(Row->Asset));
					}
					return FReply::Unhandled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(4)
					[
						SNew(SBox).WidthOverride(64).HeightOverride(64)
						[
							Row->Thumbnail ? Row->Thumbnail->MakeThumbnailWidget() : SNullWidget::NullWidget
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(6, 2)
					[
						SNew(STextBlock)
						.Text(Row->Thumbnail ? Name : FText::Format(LOCTEXT("Missing", "{0}（素材不可用）"), Name))
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
				];
		}

		void CandidateChanged(const FAssetData& Asset)
		{
			Candidate = Asset;
		}

		/** @brief コンテンツブラウザで選択した単一メッシュを候補にする。 */
		FReply PickBrowserSelection()
		{
			TArray<FAssetData> Assets;
			FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"))
				.Get().GetSelectedAssets(Assets);
			if (Assets.Num() == 1 && Assets[0].AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
			{
				CandidateChanged(Assets[0]);
			}
			else
			{
				Status = LOCTEXT("BrowserSelectionRequired", "请在内容浏览器中选中一个静态网格");
			}
			return FReply::Handled();
		}

		void FilterChanged(const FText& Text)
		{
			Filter = Text.ToString();
			RefreshList();
		}

		void RefreshList()
		{
			VisibleRows = Rows.FilterByPredicate([this](const FRockRowPtr& Row)
			{
				return Row->Path.Contains(Filter, ESearchCase::IgnoreCase);
			});
			if (const FRockRowPtr Row = Selected(); Row && !VisibleRows.Contains(Row))
			{
				List->ClearSelection();
			}
			List->RequestListRefresh();
			Status = FText::Format(LOCTEXT("Count", "显示 {0} / {1} 项"), VisibleRows.Num(), Rows.Num());
		}

		FRockRowPtr Selected() const
		{
			const TArray<FRockRowPtr> SelectedItems = List->GetSelectedItems();
			return SelectedItems.IsEmpty() ? nullptr : SelectedItems[0];
		}

		bool CanUseSelection() const
		{
			const FRockRowPtr Row = Selected();
			return Row && Row->Thumbnail.IsValid();
		}

		/** @brief 成功した保存だけを一覧へ反映する。 */
		void SaveRows(TArray<FRockRowPtr> NextRows, const FRockRowPtr& SelectAfterSave = nullptr)
		{
			TArray<FString> Paths;
			for (const FRockRowPtr& Row : NextRows)
			{
				Paths.Add(Row->Path);
			}
			FConfigFile Config;
			Config.SetBool(LibrarySection, InitializedKey, true);
			Config.SetArray(LibrarySection, AssetPathsKey, Paths);
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(ConfigPath), true);
			if (!Config.Write(ConfigPath, false))
			{
				Status = LOCTEXT("SaveFailed", "无法保存本地素材列表");
				return;
			}

			Rows = MoveTemp(NextRows);
			RefreshList();
			if (SelectAfterSave && VisibleRows.Contains(SelectAfterSave))
			{
				List->SetSelection(SelectAfterSave);
				List->RequestScrollIntoView(SelectAfterSave);
			}
		}

		FReply AddMesh() { return StoreCandidate(INDEX_NONE); }
		FReply ReplaceMesh() { return StoreCandidate(Rows.IndexOfByKey(Selected())); }

		FReply StoreCandidate(int32 ReplaceIndex)
		{
			const FString Path = Candidate.GetObjectPathString();
			if (!Candidate.IsValid() || Candidate.AssetClassPath != UStaticMesh::StaticClass()->GetClassPathName())
			{
				Status = LOCTEXT("PickMesh", "请选择一个静态网格");
				return FReply::Handled();
			}
			if (Rows.ContainsByPredicate([&Path](const FRockRowPtr& Row) { return Row->Path.Equals(Path, ESearchCase::IgnoreCase); }))
			{
				Status = LOCTEXT("Duplicate", "该网格已在素材列表中");
				return FReply::Handled();
			}

			TArray<FRockRowPtr> NextRows = Rows;
			const FRockRowPtr NewRow = MakeRow(Path);
			if (NextRows.IsValidIndex(ReplaceIndex))
			{
				NextRows[ReplaceIndex] = NewRow;
			}
			else
			{
				NextRows.Add(NewRow);
			}
			SaveRows(MoveTemp(NextRows), NewRow);
			return FReply::Handled();
		}

		FReply RemoveMesh()
		{
			if (const FRockRowPtr Row = Selected())
			{
				TArray<FRockRowPtr> NextRows = Rows;
				NextRows.RemoveSingle(Row);
				SaveRows(MoveTemp(NextRows));
			}
			return FReply::Handled();
		}

		FReply OpenMesh()
		{
			if (const FRockRowPtr Row = Selected(); Row && GEditor)
			{
				if (UAssetEditorSubsystem* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
				{
					if (UObject* Asset = Row->Asset.GetAsset(); Asset && AssetEditor->OpenEditorForAsset(Asset))
					{
						return FReply::Handled();
					}
				}
			}
			Status = LOCTEXT("OpenFailed", "无法打开该静态网格");
			return FReply::Handled();
		}

		FReply UseMesh()
		{
			if (const FRockRowPtr Row = Selected(); Row && Cast<UStaticMesh>(Row->Asset.GetAsset()))
			{
				Tool->SetPlacementKind(LeeMapEditor::ELeePlacementKind::Rock);
				Tool->SetRockMesh({ FText::GetEmpty(), Row->Path, Row->Name, false });
				Status = FText::Format(LOCTEXT("Using", "已选用 {0}。在抓点布置面板启用视口工具后即可摆放。"),
					FText::FromString(Row->Name));
			}
			else
			{
				Status = LOCTEXT("UseFailed", "无法加载该静态网格");
			}
			return FReply::Handled();
		}

		TSharedPtr<FLeeMapEditorTool> Tool;
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
		TArray<FRockRowPtr> Rows;
		TArray<FRockRowPtr> VisibleRows;
		TSharedPtr<SListView<FRockRowPtr>> List;
		FString ConfigPath;
		FString Filter;
		FAssetData Candidate;
		FText Status;
	};
}

TSharedRef<SWidget> MakeLeeRockLibrary(const TSharedRef<FLeeMapEditorTool>& Tool)
{
	return SNew(SLeeRockLibrary, Tool);
}

#undef LOCTEXT_NAMESPACE
