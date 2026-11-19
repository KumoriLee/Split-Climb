#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Templates/SharedPointer.h"

#include "Lee/LeeMapEditorTypes.h"

class FCanvas;
class FEditorViewportClient;
class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class HHitProxy;
class UBoxComponent;
class UBlueprint;
class UStaticMesh;
struct FViewportClick;
struct FLeeRockAssetEntry;

/**
 * @brief SplitClimb マップエディタツールの本体。配置状態・パラメータ・測量・検査・適用を管理する。
 *
 *        プレビューはアクタを一切レベルへ生成せず、モードの Render / DrawHUD による描画のみで行う。
 *        レベルへの生成・変更は Apply 実行時のみ、単一のトランザクションとして行う。
 */
class FLeeMapEditorTool : public FGCObject, public TSharedFromThis<FLeeMapEditorTool>
{
public:
	/** @brief コンストラクタ。切図・PIE イベントを購読する。 */
	FLeeMapEditorTool();

	/** @brief デストラクタ。イベント購読を解除する。 */
	virtual ~FLeeMapEditorTool() override;

	//~ FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FLeeMapEditorTool");
	}

	/** @brief 状態変化通知（パネル UI の更新用）。 */
	FLeeToolStateChanged OnStateChanged;

	// ---------------------------------------------------------------
	// モード連携
	// ---------------------------------------------------------------

	/** @brief モードが有効になったときに呼ばれる。 */
	void OnModeEntered();

	/** @brief モードが無効になったときに呼ばれる。 */
	void OnModeExited();

	/** @brief 現在エディタモードが有効か。 */
	bool IsModeActive() const { return bModeActive; }

	/** @brief ツールパネルから配置ツールの有効/無効を切り替える。 */
	void SetArmed(bool bArmed);

	/** @brief モード有効中にクリックやドラッグをツールが処理中か（マーキー選択抑止用）。 */
	bool IsEngaged() const;

	// ---------------------------------------------------------------
	// パラメータ
	// ---------------------------------------------------------------

	/** @brief 配置種別（抓点 / 岩体）を設定する。 */
	void SetPlacementKind(LeeMapEditor::ELeePlacementKind InKind);

	LeeMapEditor::ELeePlacementKind GetPlacementKind() const { return PlacementKind; }

	/** @brief 抓点の強弱を切り替える。現在の寸法は保持する。 */
	void SetHoldType(LeeMapEditor::ELeeHoldType InType);

	LeeMapEditor::ELeeHoldType GetHoldType() const { return HoldType; }

	/** @brief 現在の強弱に対応する既定寸法へ戻す。 */
	void RestorePresetSize();

	/** @brief 接線ステップ（cm）を設定する。 */
	void SetStepCm(double InStep);

	double GetStepCm() const { return StepCm; }

	/** @brief 接線横方向オフセット（cm）。 */
	void SetOffsetU(double InValue);

	double GetOffsetU() const { return OffsetU; }

	/** @brief 接線縦方向オフセット（cm）。 */
	void SetOffsetV(double InValue);

	double GetOffsetV() const { return OffsetV; }

	/** @brief 法線回り回転角（度）。 */
	void SetAngleDeg(double InValue);

	double GetAngleDeg() const { return AngleDeg; }

	/** @brief 面から外へのオフセット（cm）。 */
	void SetOffsetN(double InValue);

	double GetOffsetN() const { return OffsetN; }

	/** @brief 全寸法（幅 X × 厚さ Y × 高さ Z、cm）を設定する。 */
	void SetSize(const FVector& InSize);

	const FVector& GetSize() const { return Size; }

	/** @brief 幅（X）を設定する。 */
	void SetSizeX(double InValue);

	/** @brief 厚さ（Y）を設定する。 */
	void SetSizeY(double InValue);

	/** @brief 高さ（Z）を設定する。 */
	void SetSizeZ(double InValue);

	// ---------------------------------------------------------------
	// 岩体ネイティブ配置
	// ---------------------------------------------------------------

	/** @brief 原生配置カテゴリ一覧を返す（アセット欠損でもカテゴリは常に表示される）。 */
	static const TArray<FLeeRockAssetEntry>& GetRockEntries();

	/** @brief 原生配置するメッシュを選択する。 */
	void SetRockMesh(const FLeeRockAssetEntry& Entry);

	/** @brief 現在選択中の原生配置メッシュ（未選択なら nullptr）。 */
	UStaticMesh* GetRockMesh() const;

	/** @brief 選択中の原生配置エントリのインデックス（未選択なら -1）。 */
	int32 GetRockEntryIndex() const { return RockEntryIndex; }

	// ---------------------------------------------------------------
	// ビューポート操作（モードから呼ばれる）
	// ---------------------------------------------------------------

	/**
	 * @brief ビューポート左クリックを処理する（5.9 は押下/解放の双方で HandleClick が来る）。
	 *        実処理はすべて InputKey → ExecuteViewportPlacementRay で行うため、
	 *        ここではモード有効時の当該交互の二重派发を遮蔽するだけにする。
	 * @return true でクリックを消費（エディタ標準の選択処理を抑止）。
	 */
	bool HandleViewportClick(FEditorViewportClient* InViewportClient, HHitProxy* InHitProxy, const FViewportClick& InClick);

	/**
	 * @brief 指定スクリーン座標で実際のレイ検出を行う（拾取・プレビュー生成・面張り直しの中身）。
	 *        Mode::InputKey の左ボタン押下から呼ばれる（HandleClick 経由に依存しない）。
	 * @return 命中して処理した場合 true。
	 */
	bool ExecuteViewportPlacementRay(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY);

	/** @brief ドラッグ終了（左ボタン解放）。 */
	bool HandleMouseButtonRelease(FKey InKey);

	/** @brief Esc キーでプレビュー・拾取モードを取り消す。 */
	bool HandleEscape();

	/** @brief マウスキャプチャ中の移動。同一コンポーネント上を滑らせプレビューを追従させる。 */
	bool HandleCapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY);

	/** @brief 毎フレームの更新（測量の再計算など）。 */
	void Tick();

	/** @brief ビューポートへのプレビュー描画。 */
	void Render(const FSceneView* InView, FPrimitiveDrawInterface* PDI);

	/** @brief ビューポート HUD への測量ラベル描画。 */
	void DrawHUD(FEditorViewportClient* InViewportClient, FViewport* InViewport, const FSceneView* InView, FCanvas* Canvas);

	// ---------------------------------------------------------------
	// 拾取（明示選択）
	// ---------------------------------------------------------------

	/** @brief 拾取モードを設定する（None で解除）。 */
	void SetPickMode(LeeMapEditor::ELeePickMode InMode);

	LeeMapEditor::ELeePickMode GetPickMode() const { return PickMode; }

	/** @brief 明示指定した貼り付け先岩体コンポーネント。 */
	UPrimitiveComponent* GetAttachOverride() const { return AttachOverride.Get(); }

	/** @brief 明示指定した貼り付け先を解除する。 */
	void ClearAttachOverride();

	/** @brief 指定参照をすべて解除する。 */
	void ClearSpecifiedRefs();

	/** @brief 指定参照を追加する（参照モードを指定参照へ切り替える）。 */
	void AddSpecifiedRef(AActor* InHoldActor);

	/** @brief 指定参照を 1 件削除する。 */
	void RemoveSpecifiedRef(AActor* InHoldActor);

	/** @brief 指定参照アクタ配列を返す。 */
	TArray<AActor*> GetSpecifiedRefs() const;

	// ---------------------------------------------------------------
	// プレビュー・適用
	// ---------------------------------------------------------------

	/** @brief プレビューが有効か。 */
	bool IsPreviewActive() const { return bPreviewActive; }

	/** @brief 既存抓点を編集中か。 */
	bool IsEditingExisting() const { return EditTarget.IsValid(); }

	/** @brief 編集中の既存抓点アクタ。 */
	AActor* GetEditTarget() const { return EditTarget.Get(); }

	/**
	 * @brief 現在のパラメータから盒体のワールド変換を計算する。
	 * @return プレビューが有効な場合 true。
	 */
	bool ComputePreviewTransform(FTransform& OutTransform) const;

	/** @brief プレビュー・編集状態を取り消す（パラメータは保持）。 */
	void CancelPreview();

	/** @brief 抓点を生成または変更する（単一トランザクション＝一度の undo）。 */
	FString ApplyHold();

	/** @brief 岩体メッシュを生成する（単一トランザクション）。 */
	FString ApplyRock();

	// ---------------------------------------------------------------
	// 既存抓点編集
	// ---------------------------------------------------------------

	/** @brief 既存抓点を編集対象として読み込む。 */
	FString StartEditHold(AActor* InHoldActor);

	// ---------------------------------------------------------------
	// 測量
	// ---------------------------------------------------------------

	/** @brief 参照モードを設定する。 */
	void SetRefMode(LeeMapEditor::ELeeRefMode InMode);

	LeeMapEditor::ELeeRefMode GetRefMode() const { return RefMode; }

	/** @brief 最新の測量結果を返す。 */
	const TArray<LeeMapEditor::FLeeMeasureEntry>& GetMeasurements() const { return Measurements; }

	// ---------------------------------------------------------------
	// 検査
	// ---------------------------------------------------------------

	/** @brief 検査クエリモードを設定する。 */
	void SetCheckMode(LeeMapEditor::ELeeCheckMode InMode);

	LeeMapEditor::ELeeCheckMode GetCheckMode() const { return CheckMode; }

	/** @brief 検査ボックスのクエリを実行する（読み取り専用）。 */
	const LeeMapEditor::FLeeCheckResult& RunCheck();

	/** @brief 盒体の外側から逆方向クエリを行い、最初の命中を確認する。 */
	const LeeMapEditor::FLeeCheckResult& RunReverseProbe();

	/** @brief 直近の検査結果（未実行時は未実行状態）。 */
	const LeeMapEditor::FLeeCheckResult& GetLastCheckResult() const { return LastCheck; }

	// ---------------------------------------------------------------
	// 抓点リスト
	// ---------------------------------------------------------------

	/** @brief 現在のレベルの抓点一覧を取得する。 */
	TArray<AActor*> CollectHoldActors();

	/** @brief 対象アクタにビューポートを合わせて選択する。 */
	void LocateHold(AActor* InHoldActor);

	// ---------------------------------------------------------------
	// アセット解決・状態テキスト
	// ---------------------------------------------------------------

	/** @brief 抓点クラスを解決する（BP_ClimbHold 生成クラス、欠損時は nullptr）。 */
	UClass* ResolveHoldClass();

	/** @brief BP_ClimbHold が欠損しているか。 */
	bool IsUsingFallbackHoldClass() const { return bUsingFallbackHoldClass; }

	/** @brief パネル表示用の状態テキスト（複数行）。 */
	FString BuildStatusText();

private:
	/** @brief 現在編集対象のエディタワールドを取得する。 */
	UWorld* GetWorldSafe() const;

	/** @brief 面への可視性レイキャスト（抓点アクタと編集対象を無視）。 */
	bool SurfaceTrace(UWorld* InWorld, const FVector& InStart, const FVector& InDir, FHitResult& OutHit);

	/** @brief Climbable チャンネルでのレイキャスト（抓点拾取用、呼び出し側で真の抓点にフィルタする）。 */
	bool HoldPickTrace(UWorld* InWorld, const FVector& InStart, const FVector& InDir, FHitResult& OutHit);

	/** @brief スクリーン座標からワールドレイを生成して面トレースする。 */
	bool TraceFromScreen(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InX, int32 InY, bool bPickHolds, FHitResult& OutHit);

	/** @brief ヒットからアンカーを設定しプレビューを開始する。 */
	void BeginPreviewFromHit(const FHitResult& InHit);

	/** @brief アクタが抓点（BP_ClimbHold クラス＋盒体コンポーネント）かどうか。タグのみでは判定しない。 */
	bool IsHoldActor(AActor* InActor);

	/** @brief 測量を再計算する。 */
	void RefreshMeasurements();

	/** @brief 現在の盒体中心（プレビュー or 編集対象）。 */
	bool GetFocusCenter(FVector& OutCenter) const;

	/** @brief 現在の盒体の寸法（プレビュー or 編集対象）。 */
	FVector GetFocusSize() const;

	/** @brief 現在の盒体の変換（プレビュー or 編集対象）。 */
	bool GetFocusTransform(FTransform& OutTransform) const;

	/**
	 * @brief 数値 UV オフセットを同一コンポーネント上へ再レイ投影して検証する。
	 * @return 同一コンポーネント上に射影できた場合 true（OutPoint を更新）。
	 */
	bool ProjectSurfacePoint(const FVector& InTangentPoint, FVector& OutPoint);

	/** @brief 数値変更後にプレビューの有効性を再検証する。 */
	void ValidatePreviewAfterParameterChange();

	/** @brief 切図・PIE 開始時にプレビューと古い参照ポインタを破棄する。 */
	void InvalidateWorldReferences();

	/** @brief 切図イベントハンドラ。 */
	void HandleMapChanged(uint32 InFlags);

	/** @brief PIE 開始前イベントハンドラ。 */
	void HandlePreBeginPIE(bool bInIsSimulating);

	/** @brief PIE 終了前イベントハンドラ。 */
	void HandlePrePIEEnded(bool bInIsSimulating);

	/** @brief 状態変化を通知する。 */
	void NotifyChanged();

	/** @brief 現在の強弱タグ名を返す。 */
	FName GetCurrentHoldTag() const;

public:
	// ---------------------------------------------------------------
	// 描画補助（モードから参照）
	// ---------------------------------------------------------------

	/** @brief 編集対象の元姿勢（編集時のゴースト表示用）。 */
	bool GetEditTargetOriginalTransform(FTransform& OutTransform) const { OutTransform = EditTargetOriginalTransform; return EditTarget.IsValid(); }

private:
	// モード・拾取状態
	bool bModeActive = false;                              //!< エディタモード有効フラグ
	LeeMapEditor::ELeePickMode PickMode = LeeMapEditor::ELeePickMode::None; //!< 拾取モード
	LeeMapEditor::ELeePlacementKind PlacementKind = LeeMapEditor::ELeePlacementKind::Hold; //!< 配置種別

	// パラメータ
	LeeMapEditor::ELeeHoldType HoldType = LeeMapEditor::ELeeHoldType::Strong; //!< 強弱種別
	FVector Size = LeeMapEditor::StrongPresetSize;         //!< 全寸法（幅・厚さ・高さ、cm）
	double AngleDeg = 0.0;                                 //!< 法線回り回転角（度）
	double OffsetU = 0.0;                                  //!< 接線横方向オフセット（cm）
	double OffsetV = 0.0;                                  //!< 接線縦方向オフセット（cm）
	double OffsetN = 0.0;                                  //!< 面外オフセット（cm）
	double StepCm = LeeMapEditor::DefaultStepCm;           //!< 接線ステップ（cm）

	// プレビュー状態
	LeeMapEditor::FLeeSurfaceAnchor Anchor;                //!< 面アンカー
	FVector ValidatedSurfacePoint = FVector::ZeroVector;   //!< 検証済みの面上基準点（数値オフセット投影結果）
	double ValidOffsetU = 0.0;                             //!< 検証済み横方向オフセット（cm）
	double ValidOffsetV = 0.0;                             //!< 検証済み縦方向オフセット（cm）
	bool bPreviewActive = false;                           //!< プレビュー表示中
	bool bDragging = false;                                //!< 左ボタン押下で貼面ドラッグ中
	TWeakObjectPtr<UPrimitiveComponent> AttachOverride;    //!< 明示指定の貼り付け先
	TWeakObjectPtr<AActor> EditTarget;                     //!< 編集中の既存抓点
	FTransform EditTargetOriginalTransform = FTransform::Identity; //!< 編集対象の元姿勢

	// 測量
	LeeMapEditor::ELeeRefMode RefMode = LeeMapEditor::ELeeRefMode::Nearest3; //!< 参照モード
	TArray<TWeakObjectPtr<AActor>> SpecifiedRefs;          //!< 指定参照
	TArray<LeeMapEditor::FLeeMeasureEntry> Measurements;   //!< 最新測量結果

	// 検査
	LeeMapEditor::ELeeCheckMode CheckMode = LeeMapEditor::ELeeCheckMode::Climbable; //!< 検査モード
	LeeMapEditor::FLeeCheckResult LastCheck;               //!< 直近の検査結果

	// アセット（GCObject で TObjectPtr により保護する）
	TObjectPtr<UBlueprint> HoldBlueprint = nullptr;        //!< BP_ClimbHold
	bool bUsingFallbackHoldClass = false;                  //!< BP 欠損フラグ
	int32 RockEntryIndex = INDEX_NONE;                     //!< 選択中の原生配置エントリ

	TObjectPtr<UStaticMesh> RockMesh = nullptr;            //!< 選択中の原生配置メッシュ

	// イベント購読ハンドル
	FDelegateHandle MapChangeHandle;                       //!< 切図イベント
	FDelegateHandle PreBeginPIEHandle;                     //!< PIE 開始前イベント
	FDelegateHandle PrePIEEndedHandle;                     //!< PIE 終了前イベント
};
