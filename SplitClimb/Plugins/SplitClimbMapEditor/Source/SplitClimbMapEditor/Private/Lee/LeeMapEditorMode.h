#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"

#include "Lee/LeeMapEditorTypes.h"

class FCanvas;
class FEditorViewportClient;
class FLeeMapEditorTool;
class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class HHitProxy;
struct FViewportClick;

/**
 * @brief SplitClimb マップエディタ用のレベルエディタモード。
 *
 *        モード有効時のみ左クリックとドラッグをツールへ委譲して消費し、
 *        プレビュー・測量線の描画と Esc 取消を担う。アイドル時は標準の選択操作を妨げない。
 *        ツール実体はモジュールのシングルトンから取得する（モードは既定コンストラクタで生成される）。
 */
class FLeeMapEditorMode : public FEdMode
{
public:
	/** @brief コンストラクタ（モジュールのツール実体を取得する）。 */
	FLeeMapEditorMode();

	//~ FEdMode
	virtual void Enter() override;
	virtual void Exit() override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* InHitProxy, const FViewportClick& InClick) override;
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;
	virtual bool MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual void Tick(FEditorViewportClient* InViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* InView, FViewport* InViewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* InViewportClient, FViewport* InViewport, const FSceneView* InView, FCanvas* Canvas) override;
	virtual bool DisallowMouseDeltaTracking() const override;
	virtual bool UsesTransformWidget() const override;
	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override;

private:
	/** @brief モジュールの共有ツール実体。 */
	TSharedPtr<FLeeMapEditorTool> Tool;
};
