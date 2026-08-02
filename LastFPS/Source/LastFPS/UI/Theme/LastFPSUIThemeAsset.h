#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "LastFPSUIThemeAsset.generated.h"

/**
 * 아웃게임 화면이 공유하는 색 팔레트.
 *
 * 등급색은 여기에 넣지 않는다. LastFPSGetRarityColor() 가 이미 UI·드랍 발광의 단일 출처이고,
 * 같은 값을 테마에도 두면 두 곳이 어긋난다. 테마는 등급 표현의 세기(RarityGlowIntensity)만 정한다.
 */
USTRUCT(BlueprintType)
struct FLastFPSUIPalette
{
	GENERATED_BODY()

	/** 패널 본체 채움색 (알파 포함 — 배경 위에 겹쳐 쓴다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor PanelFill = FLinearColor(0.016f, 0.024f, 0.035f, 0.86f);

	/** 패널 테두리·구분선 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor PanelEdge = FLinearColor(0.09f, 0.16f, 0.20f, 1.f);

	/** 주 강조색 — 선택 상태, 활성 탭, 게이지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor AccentPrimary = FLinearColor(0.16f, 0.78f, 0.92f, 1.f);

	/** 보조 강조색 — 경고, 소모 자원, 하이라이트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor AccentWarning = FLinearColor(0.95f, 0.35f, 0.15f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor TextPrimary = FLinearColor(0.93f, 0.96f, 0.98f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor TextSecondary = FLinearColor(0.62f, 0.72f, 0.78f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor TextMuted = FLinearColor(0.38f, 0.45f, 0.50f, 1.f);

	/** 증가 보정 표시색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor ValuePositive = FLinearColor(0.25f, 0.90f, 0.35f, 1.f);

	/** 감소 보정 표시색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Palette")
	FLinearColor ValueNegative = FLinearColor(0.90f, 0.30f, 0.30f, 1.f);
};

/** 화면 전체가 공유하는 글꼴 단계. 크기를 여기서만 바꾸면 모든 화면이 함께 따라온다. */
USTRUCT(BlueprintType)
struct FLastFPSUITypography
{
	GENERATED_BODY()

	/** 화면 제목·탭 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FSlateFontInfo Header;

	/** 항목 이름·본문 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FSlateFontInfo Label;

	/** 수치 강조 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FSlateFontInfo Value;

	/** 보조 설명·키 힌트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Typography")
	FSlateFontInfo Caption;
};

/** 간격 규칙. 개별 위젯에서 매직 넘버로 흩어지지 않게 한 곳에 모은다. */
USTRUCT(BlueprintType)
struct FLastFPSUIMetrics
{
	GENERATED_BODY()

	/** 패널 바깥 여백 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metrics")
	FMargin PanelPadding = FMargin(14.f, 12.f);

	/** 패널 안 항목 여백 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metrics")
	FMargin ContentPadding = FMargin(8.f, 4.f);

	/** 목록 항목 사이 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metrics", meta=(ClampMin="0.0"))
	float RowSpacing = 4.f;

	/** 구획 사이 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metrics", meta=(ClampMin="0.0"))
	float SectionSpacing = 16.f;
};

/** 패널 표면 연출. 실제 브러시는 머티리얼을 물릴 수 있게 FSlateBrush 로 노출한다. */
USTRUCT(BlueprintType)
struct FLastFPSUISurface
{
	GENERATED_BODY()

	/** 패널 배경 — 각진 프레임 머티리얼을 물린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface")
	FSlateBrush PanelBrush;

	/** 활성 탭 밑줄·강조 라인 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface")
	FSlateBrush TabActiveBrush;

	/** 슬롯 테두리 — 등급색을 틴트로 받는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface")
	FSlateBrush SlotBorderBrush;

	/** 등급 발광 세기. 등급 '색'은 LastFPSGetRarityColor() 가 정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface", meta=(ClampMin="0.0"))
	float RarityGlowIntensity = 1.6f;

	/** 선택 상태 발광 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface", meta=(ClampMin="0.0"))
	float SelectedGlowIntensity = 2.4f;

	/** 스캔라인 세기. 0 이면 스캔라인을 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ScanIntensity = 0.18f;
};

/**
 * 아웃게임 UI 테마 — 색·글꼴·간격·표면 연출의 단일 출처.
 *
 * 화면 위젯은 이 에셋만 알고 서로를 알지 않는다. 톤을 바꿀 때 위젯 블루프린트를 하나씩 여는 대신
 * 이 에셋 하나만 고치면 되도록, 표시 값은 전부 데이터로 뺀다.
 *
 * 활성 테마는 ULastFPSUISettings 가 가리킨다.
 */
UCLASS(BlueprintType)
class LASTFPS_API ULastFPSUIThemeAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Theme")
	FLastFPSUIPalette Palette;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Theme")
	FLastFPSUITypography Typography;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Theme")
	FLastFPSUIMetrics Metrics;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Theme")
	FLastFPSUISurface Surface;
};
