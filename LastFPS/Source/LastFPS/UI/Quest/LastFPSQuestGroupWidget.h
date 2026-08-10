#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastFPSQuestGroupWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UExpandableArea;
class ULastFPSQuestEntryWidget;

/**
 * 퀘스트 화면의 아코디언 메뉴(그룹)를 담당하는 위젯입니다.
 */
UCLASS()
class LASTFPS_API ULastFPSQuestGroupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupGroup(const FText& InGroupTitle, const FText& InGroupSubtitle);
	
	void AddQuestEntry(ULastFPSQuestEntryWidget* EntryWidget);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UExpandableArea> ExpandableArea;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> TB_GroupTitle;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TB_GroupSubtitle;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UVerticalBox> Box_Entries;
};
