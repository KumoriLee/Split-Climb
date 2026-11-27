#pragma once

#include "CoreMinimal.h"

/**
 * @brief 分離された一時ワールドのスコープガード。
 *
 *        プレビューや計測のために BP インスタンスを一時的に生成して実寸法を取得する
 *        などの目的で使用し、永続レベルには一切のオブジェクトを残さない。
 *        生成されたワールドは RF_Transient で保存対象にならない。
 */
class FLeeMapEditorTempWorldScope
{
public:
	/** @brief 一時ワールド（EditorPreview）を生成する。 */
	FLeeMapEditorTempWorldScope();

	/** @brief 一時ワールドをエンジンから切り離して破棄する。 */
	~FLeeMapEditorTempWorldScope();

	FLeeMapEditorTempWorldScope(const FLeeMapEditorTempWorldScope&) = delete;
	FLeeMapEditorTempWorldScope& operator=(const FLeeMapEditorTempWorldScope&) = delete;

	/** @brief 生成した一時ワールドを返す（失敗時は nullptr）。 */
	UWorld* GetWorld() const { return World; }

private:
	/** @brief 保持する一時ワールド。 */
	UWorld* World = nullptr;
};
