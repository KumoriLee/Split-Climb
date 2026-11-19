#include "Lee/LeeMapEditorMode.h"

#include "Lee/LeeSplitClimbMapEditorModule.h"
#include "Lee/LeeMapEditorTool.h"

#include "EditorViewportClient.h"
#include "InputCoreTypes.h"

FLeeMapEditorMode::FLeeMapEditorMode()
	: FEdMode()
{
	Tool = GetLeeSplitClimbMapEditorTool();
}

void FLeeMapEditorMode::Enter()
{
	FEdMode::Enter();
	if (Tool.IsValid())
	{
		Tool->OnModeEntered();
	}
}

void FLeeMapEditorMode::Exit()
{
	// モード退出時はプレビューと拾取状態を取り消す（パラメータは保持される）
	if (Tool.IsValid())
	{
		Tool->OnModeExited();
	}
	FEdMode::Exit();
}

bool FLeeMapEditorMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* InHitProxy, const FViewportClick& InClick)
{
	if (Tool.IsValid() && Tool->HandleViewportClick(InViewportClient, InHitProxy, InClick))
	{
		return true;
	}
	return FEdMode::HandleClick(InViewportClient, InHitProxy, InClick);
}

bool FLeeMapEditorMode::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	if (Tool.IsValid() && InViewportClient != nullptr && InViewport != nullptr)
	{
		if (InKey == EKeys::LeftMouseButton)
		{
			// Alt+左ドラッグはカメラ操作（オービット）なので、その間はツールに割り込まない
			const bool bAltHeld = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);
			if (!bAltHeld)
			{
				if (InEvent == IE_Pressed)
				{
					// 押下：ここで実際のレイ検出（拾取・プレビュー生成・面張り直し）を行う。
					// 5.9 の ProcessClickInViewport は解放時に IE_Released で HandleClick を呼ぶため、
					// HandleClick 経由には依存しない
					Tool->ExecuteViewportPlacementRay(InViewportClient, InViewport, InViewport->GetMouseX(), InViewport->GetMouseY());
					return true;
				}
				if (InEvent == IE_Released)
				{
					// 解放：貼面ドラッグを終了し、解放側 HandleClick の選択処理も併せて遮蔽する
					Tool->HandleMouseButtonRelease(InKey);
					return true;
				}
			}
		}

		if (InKey == EKeys::Escape && InEvent == IE_Pressed && Tool->HandleEscape())
		{
			// Esc でプレビュー・拾取を取り消した場合はイベントを消費し、モードは維持する
			return true;
		}
	}
	return FEdMode::InputKey(InViewportClient, InViewport, InKey, InEvent);
}

bool FLeeMapEditorMode::MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (Tool.IsValid())
	{
		// 捕獲の有無に関わらず、ドラッグ中は同一コンポーネント面上の滑りを更新する
		return Tool->HandleCapturedMouseMove(InViewportClient, InViewport, InMouseX, InMouseY);
	}
	return FEdMode::MouseMove(InViewportClient, InViewport, InMouseX, InMouseY);
}

bool FLeeMapEditorMode::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (Tool.IsValid())
	{
		return Tool->HandleCapturedMouseMove(InViewportClient, InViewport, InMouseX, InMouseY);
	}
	return false;
}

void FLeeMapEditorMode::Tick(FEditorViewportClient* InViewportClient, float DeltaTime)
{
	FEdMode::Tick(InViewportClient, DeltaTime);
	if (Tool.IsValid())
	{
		Tool->Tick();
	}
}

void FLeeMapEditorMode::Render(const FSceneView* InView, FViewport* InViewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(InView, InViewport, PDI);
	if (Tool.IsValid())
	{
		Tool->Render(InView, PDI);
	}
}

void FLeeMapEditorMode::DrawHUD(FEditorViewportClient* InViewportClient, FViewport* InViewport, const FSceneView* InView, FCanvas* Canvas)
{
	FEdMode::DrawHUD(InViewportClient, InViewport, InView, Canvas);
	if (Tool.IsValid())
	{
		Tool->DrawHUD(InViewportClient, InViewport, InView, Canvas);
	}
}

bool FLeeMapEditorMode::DisallowMouseDeltaTracking() const
{
	// プレビュー中・拾取中はマーキー選択を抑止し、左ドラッグを貼面スライドに使う
	return Tool.IsValid() && Tool->IsEngaged();
}

bool FLeeMapEditorMode::UsesTransformWidget() const
{
	// ギズモは使用しない（数値パネルで操作する）
	return false;
}

bool FLeeMapEditorMode::UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const
{
	return false;
}
