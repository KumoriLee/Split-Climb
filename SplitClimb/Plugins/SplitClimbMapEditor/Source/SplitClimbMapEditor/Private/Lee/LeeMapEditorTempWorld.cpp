#include "Lee/LeeMapEditorTempWorld.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

/**
 * @brief EditorPreview 型の一時ワールドを生成する。
 */
FLeeMapEditorTempWorldScope::FLeeMapEditorTempWorldScope()
{
	World = UWorld::CreateWorld(EWorldType::EditorPreview, /*bInformEngineOfWorld*/ false, TEXT("LeeSplitClimbMapEditorTemp"));
}

/**
 * @brief 一時ワールドを確実に破棄する。
 */
FLeeMapEditorTempWorldScope::~FLeeMapEditorTempWorldScope()
{
	if (World)
	{
		if (GEngine)
		{
			GEngine->DestroyWorldContext(World);
		}
		World->MarkAsGarbage();
		World = nullptr;
	}
}
