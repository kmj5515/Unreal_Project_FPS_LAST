#include "UI/Framework/LastFPSScreenRegistry.h"

const FLastFPSScreenDef* ULastFPSScreenRegistry::FindScreen(const FGameplayTag& ScreenTag) const
{
	return Screens.Find(ScreenTag);
}

void ULastFPSScreenRegistry::GetTabScreensForHost(
	const FGameplayTag& HostScreenTag, TArray<FGameplayTag>& OutTabTags) const
{
	OutTabTags.Reset();
	if (!HostScreenTag.IsValid())
	{
		return;
	}

	// TMap 순회 순서는 보장되지 않으므로 TabOrder 를 함께 모아 정렬한다.
	TArray<TPair<int32, FGameplayTag>> Ordered;
	for (const TPair<FGameplayTag, FLastFPSScreenDef>& Pair : Screens)
	{
		const FLastFPSScreenDef& Def = Pair.Value;
		if (Def.Presentation == ELastFPSScreenPresentation::ShellTab
			&& Def.HostScreenTag == HostScreenTag)
		{
			Ordered.Emplace(Def.TabOrder, Pair.Key);
		}
	}

	Ordered.Sort([](const TPair<int32, FGameplayTag>& A, const TPair<int32, FGameplayTag>& B)
	{
		// 순서 값이 같으면 태그 이름으로 갈라 실행마다 목록이 뒤바뀌지 않게 한다.
		return A.Key != B.Key ? A.Key < B.Key : A.Value.ToString() < B.Value.ToString();
	});

	OutTabTags.Reserve(Ordered.Num());
	for (const TPair<int32, FGameplayTag>& Entry : Ordered)
	{
		OutTabTags.Add(Entry.Value);
	}
}
