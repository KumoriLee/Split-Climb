#pragma once

#include "Templates/SharedPointer.h"

class FLeeMapEditorTool;
class SWidget;

/** @brief 岩壁素材の一覧と編集操作を持つウィジェットを生成する。 */
TSharedRef<SWidget> MakeLeeRockLibrary(const TSharedRef<FLeeMapEditorTool>& Tool);
