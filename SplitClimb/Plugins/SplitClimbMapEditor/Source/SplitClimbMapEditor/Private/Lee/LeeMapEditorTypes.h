#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Tools/Modes.h"

class UPrimitiveComponent;
class AActor;

/** @brief ツール状態変化通知（パネル UI 同期用）。 */
DECLARE_MULTICAST_DELEGATE(FLeeToolStateChanged);

/**
 * @brief SplitClimb マップエディタ用の共通定数・列挙・構造体。
 */
namespace LeeMapEditor
{
	/** @brief 抓点の強弱種別。 */
	enum class ELeeHoldType : uint8
	{
		Strong, //!< 強（GripStrong）
		Weak    //!< 弱（GripWeak）
	};

	/** @brief ビューポートクリックで拾う対象の種別。 */
	enum class ELeePickMode : uint8
	{
		None,      //!< 拾取なし（通常の配置フロー）
		Rock,      //!< 貼り付け先の岩体コンポーネントを明示指定
		Hold,      //!< 編集対象の既存抓点を選択
		Reference  //!< 測量の指定参照抓点を選択
	};

	/** @brief 検査ボックスのクエリモード。 */
	enum class ELeeCheckMode : uint8
	{
		Climbable,       //!< Climbable チャンネル（ECC_GameTraceChannel1）でクエリ
		BlockPawn,       //!< Pawn 阻挡応答でクエリ
		IgnoreCollision  //!< 衝突を無視しバウンディング箱の幾何交差のみ確認
	};

	/** @brief 測量参照の選び方。 */
	enum class ELeeRefMode : uint8
	{
		Nearest3,  //!< 自身を除く最も近い 3 つの抓点
		Specified  //!< ユーザー指定の参照抓点
	};

	/** @brief 配置対象の種別（抓点 / 岩体ネイティブ配置）。 */
	enum class ELeePlacementKind : uint8
	{
		Hold, //!< 抓点（BP_ClimbHold）
		Rock  //!< 岩体（LP_RocksandCliffs のメッシュ）
	};

	/** @brief エディタモード ID。 */
	static const FEditorModeID ModeID = FName(TEXT("EM_LeeSplitClimbMapEditor"));

	/** @brief パネルのタブ ID。 */
	static const FName TabID = FName(TEXT("LeeSplitClimbMapEditorPanel"));
	// 2026.11.25 Lee start
	/** @brief 岩壁素材タブを識別する。 */
	static const FName RockLibraryTabID(TEXT("LeeSplitClimbRockLibrary"));
	// 2026.11.25 Lee end

	/** @brief 強抓点のタグ名。 */
	static const FName HoldStrongTag = FName(TEXT("GripStrong"));

	/** @brief 弱抓点のタグ名。 */
	static const FName HoldWeakTag = FName(TEXT("GripWeak"));

	/** @brief 強抓点の既定全寸法（幅 X × 厚さ Y × 高さ Z、cm）。 */
	static const FVector StrongPresetSize(44.0, 26.0, 56.0);

	/** @brief 弱抓点の既定全寸法（幅 X × 厚さ Y × 高さ Z、cm）。 */
	static const FVector WeakPresetSize(300.0, 40.0, 36.0);

	/** @brief BP_ClimbHold ブループリントのアセットパス。 */
	static const FString HoldBlueprintPath = TEXT("/Game/Maps/ClimbSample/BP_ClimbHold.BP_ClimbHold");

	/** @brief 実抓点クラス（BP_ClimbHold 生成クラス）のパス接頭辞。 */
	static const FString HoldClassPathPrefix = TEXT("/Game/Maps/ClimbSample/BP_ClimbHold");

	/** @brief 接線方向オフセットの既定ステップ（cm）。 */
	static const double DefaultStepCm = 1.0;

	/** @brief 法線回り回転角の既定ステップ（度）。 */
	static const double DefaultAngleStepDeg = 5.0;

	/** @brief 外側逆引きプローブを盒体から離して開始する距離（cm）。 */
	static const double ReverseProbeClearance = 100.0;

	/**
	 * @brief 岩面の錨（アンカー）情報。ヒットコンポーネント上の点・法線と接線座標系を持つ。
	 */
	struct FLeeSurfaceAnchor
	{
		TWeakObjectPtr<UPrimitiveComponent> Component; //!< 貼り付け先のヒットコンポーネント
		FVector Point = FVector::ZeroVector;           //!< 面上の基準点（ワールド）
		FVector Normal = FVector::ZAxisVector;         //!< 面法線（ワールド・外向き）
		FVector AxisU = FVector::XAxisVector;          //!< 接線・横方向（面内）
		FVector AxisV = FVector::YAxisVector;          //!< 接線・縦方向（面内・上向き基準）
		bool bValid = false;                           //!< 有効フラグ

		/** @brief 法線から接線座標系（AxisU / AxisV）を再計算する。 */
		void RecomputeAxes()
		{
			const FVector Up = FVector::ZAxisVector;
			const double DotNUp = FVector::DotProduct(Normal, Up);
			if (FMath::Abs(DotNUp) < 0.999)
			{
				// 鉛直成分を含む面：縦軸を世界アップの面内射影から採る
				AxisV = (Up - Normal * DotNUp).GetSafeNormal();
				AxisU = FVector::CrossProduct(Normal, AxisV);
			}
			else
			{
				// 床・天井面：横軸を世界 X から採る
				AxisU = FVector::XAxisVector;
				AxisV = FVector::CrossProduct(Normal, AxisU).GetSafeNormal();
			}
			// Component は任意（懸空抓点も面データだけでプレビュー可能）
			bValid = !Normal.IsNearlyZero() && !AxisU.IsNearlyZero() && !AxisV.IsNearlyZero();
		}

		/** @brief アンカーを無効化する。 */
		void Reset()
		{
			Component = nullptr;
			Point = FVector::ZeroVector;
			Normal = FVector::ZAxisVector;
			AxisU = FVector::XAxisVector;
			AxisV = FVector::YAxisVector;
			bValid = false;
		}
	};

	/**
	 * @brief 測量結果 1 件分（距離と符号付き高低差）。
	 */
	struct FLeeMeasureEntry
	{
		TWeakObjectPtr<AActor> Target; //!< 参照先アクタ
		double Distance = 0.0;         //!< 盒体中心間距離（cm）
		double DeltaZ = 0.0;           //!< 符号付き高低差（プレビュー基準、cm）
		bool bSpecified = false;       //!< ユーザー指定参照かどうか
	};

	/**
	 * @brief 検査ボックスの実行結果。
	 */
	struct FLeeCheckResult
	{
		bool bRan = false;             //!< 実行済みか
		bool bSuccess = false;         //!< 意図した設定と一致したか
		int32 HitCount = 0;            //!< ヒット件数
		FString FirstHitName;          //!< 先頭ヒットの表示名
		bool bSelfHit = false;         //!< 自身（抓点の箱）が命中したか
		FString Detail;                //!< 複数行の詳細テキスト
	};

}

/**
 * @brief 原生配置カテゴリのエントリ（岩体アセット）。グローバル名前空間で定義する。
 */
struct FLeeRockAssetEntry
{
	FText CategoryLabel; //!< カテゴリ表示名（崖壁・平台・小岩・岩板）
	FString AssetPath;   //!< メッシュのソフト参照パス
	FString MeshName;    //!< 表示用メッシュ名（アセット名）
	bool bMissing = false; //!< アセット欠損フラグ（読み込み時点で未判定の可能性あり）
};
