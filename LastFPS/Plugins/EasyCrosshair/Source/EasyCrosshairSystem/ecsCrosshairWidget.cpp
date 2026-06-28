// Copyright (c) 2025 Dodge Theory. All Rights Reserved.
//
// This software and associated documentation files (the "Software") are the
// proprietary information of Dodge Theory and may not be used, copied,
// modified, merged, published, distributed, sublicensed, or sold without
// express written permission from Dodge Theory.
//
// For more information, please contact: dodgetheory@gmail.com

#include "ecsCrosshairWidget.h"
#include "ecsCrosshairBehavior.h"
#include "Components/Overlay.h"
#include "Widgets/Images/SImage.h"


void SecsCrosshairWidget::Construct(const FArguments& InArgs)
{
	Overlay = SNew(SOverlay);
	
	ChildSlot
	[
		Overlay.ToSharedRef()
	];

	Brushes.Empty();
	Images.Empty();
	Overlay->ClearChildren();
	
	
	SetCrosshairLayers(Layers);
}

bool SecsCrosshairWidget::IsLayersExist(TArray<FName> InLayerNames)
{
    for (const FName& Name : InLayerNames)
    {
        bool bFound = false;
        for (const TPair<FString, FecsCrosshairLayerData>& Pair : Layers)
        {
            if (Pair.Value.LayerName == Name)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            return false;
        }
    }
    return true;
}

FecsCrosshairLayerData& SecsCrosshairWidget::GetLayerByName(FName LayerName)
{
	for (auto& Elem : Layers)
	{
		if (Elem.Value.LayerName == LayerName)
		{
			return Elem.Value;
		}
	}
	// Fallback: return the first element (unsafe if map is empty)
	auto It = Layers.CreateIterator();
	check(It); // Ensure map is not empty
	return It->Value;
}

TSharedPtr<SImage> SecsCrosshairWidget::GetLayerImageByName(FName LayerName) const
{
	int32 Index = 0;
	for (const auto& Elem : Layers)
	{
		if (Elem.Value.LayerName == LayerName)
		{
			return Images.IsValidIndex(Index) ? Images[Index] : nullptr;
		}
		++Index;
	}
	return nullptr;
}

TSharedPtr<FSlateBrush> SecsCrosshairWidget::GetLayerBrushByName(FName LayerName) const
{
	int32 Index = 0;
	for (const auto& Elem : Layers)
	{
		if (Elem.Value.LayerName == LayerName)
		{
			return Brushes.IsValidIndex(Index) ? Brushes[Index] : nullptr;
		}
		++Index;
	}
	return nullptr;
}




void SecsCrosshairWidget::SetCrosshairLayers(const TMap<FString, FecsCrosshairLayerData>& InLayers)
{
	if (!Overlay.IsValid())
	{
		return;
	}

	Layers = InLayers;
	Brushes.Empty();
	Images.Empty();
	Overlay->ClearChildren();

	
	for (TPair<FString, FecsCrosshairLayerData>& Layer : Layers)
	{
		if (!Layer.Value.IsValid())
			continue;

		Layer.Value.LayerName = FName(Layer.Key);
		
		// Set Brush Size
		TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
		Brush->SetResourceObject(Layer.Value.LayerTexture);
		Brush->DrawAs = ESlateBrushDrawType::Type::Image;
		Brush->Tiling = ESlateBrushTileType::Type::NoTile;
		Brushes.Add(Brush);
		
		// Create Image Widget
		TSharedPtr<SImage> Image = SNew(SImage)
			.Image(Brush.Get())
			.ColorAndOpacity(Layer.Value.LayerColor);
		Images.Add(Image);

		SetLayerTransform(
			Layer.Value.LayerName,
			FVector2D(Layer.Value.Transform.X, Layer.Value.Transform.Y),
			Layer.Value.Rotation,
			Layer.Value.Scale
			);


		Overlay->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			Image.ToSharedRef()
		];
	}
}

void SecsCrosshairWidget::SetCrosshairBehaviors(const TArray<UecsCrosshairBehavior*>& InBehaviors, UecsCrosshairWidget* Widget)
{
	Behaviors = InBehaviors;

	BehaviorTickTimers.SetNum(Behaviors.Num());
	for (int32 i = 0; i < Behaviors.Num(); ++i)
	{
		BehaviorTickTimers[i] = 0.0f;
		if (Behaviors[i])
		{
			Behaviors[i]->CrosshairWidget = Widget;
			Behaviors[i]->BeginBehavior();
		}
	}
}


void SecsCrosshairWidget::SetLayerTransform(FName LayerName, FVector2D Transform, float Rotation, float Scale)
{
	FecsCrosshairLayerData& Layer = GetLayerByName(LayerName);
	TSharedPtr<SImage>		Image = GetLayerImageByName(LayerName);

	if (!Layer.LayerTexture || !Image.IsValid())
		return;

	// Layer.CurrentTransform = Transform;
	// Layer.CurrentRotation = Rotation;
	// Layer.CurrentScale = Scale;
	
	float ScaleMultiplier = 1.0f;
	FVector2D ImportedSize = FVector2D(Layer.LayerTexture->GetImportedSize());
	float FinalScale = (1.0f + Layer.Scale * ScaleMultiplier) * (1.0f + Scale);
	FVector2D ScaledSize = ImportedSize * FinalScale * GlobalScale;
	FVector2D Pivot = ScaledSize * 0.5f;
	
	FVector2D FinalTransform = Transform * GlobalScale;
	FSlateRenderTransform FullTransform = BuildLayerTransform(
		Pivot,
		FinalScale,
		Rotation,
		FinalTransform);

	
	GetLayerBrushByName(LayerName)->SetImageSize(ScaledSize);
	Image->SetRenderTransformPivot(FVector2f(0.5f , 0.5f)); // Pivot at center
	Image->SetRenderTransform(FullTransform);
}

void SecsCrosshairWidget::ResetLayerTransform(FName LayerName)
{
	SetLayerTransform(
		LayerName,
		GetLayerByName(LayerName).Transform,
		GetLayerByName(LayerName).Rotation,
		GetLayerByName(LayerName).Scale
	);
}

FSlateRenderTransform SecsCrosshairWidget::BuildLayerTransform(
	FVector2D Pivot, float Scale, float Rotation, FVector2D Translation)
{
	return Concatenate(
		FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(Rotation))),
		FSlateRenderTransform(FVector2D(Scale, Scale)),
		FSlateRenderTransform(Translation)
	);
}


void SecsCrosshairWidget::SetLayerColor(FName LayerName, const FLinearColor& InColor)
{
	TSharedPtr<SImage> Image = GetLayerImageByName(LayerName);
	TSharedPtr<FSlateBrush> Brush = GetLayerBrushByName(LayerName);

	if (!Image.IsValid() || !Brush.IsValid())
	{
		return;
	}

	// Get current color
	FLinearColor CurrentColor = GetLayerByName(LayerName).LayerColor;

	// Add colors together (clamp each component)
	FLinearColor NewColor;
	NewColor.R = FMath::Clamp(CurrentColor.R + InColor.R, 0.f, 1.f);
	NewColor.G = FMath::Clamp(CurrentColor.G + InColor.G, 0.f, 1.f);
	NewColor.B = FMath::Clamp(CurrentColor.B + InColor.B, 0.f, 1.f);
	NewColor.A = FMath::Clamp(CurrentColor.A + InColor.A, 0.f, 1.f); // Optional: could keep alpha unchanged

	Image->SetColorAndOpacity(NewColor);
}

void SecsCrosshairWidget::SetLayerColorDirect(FName LayerName, const FLinearColor& InColor)
{
	TSharedPtr<SImage> Image = GetLayerImageByName(LayerName);
	TSharedPtr<FSlateBrush> Brush = GetLayerBrushByName(LayerName);

	if (!Image.IsValid() || !Brush.IsValid())
	{
		return;
	}

	Image->SetColorAndOpacity(InColor);
}

void SecsCrosshairWidget::ResetLayerColor(FName LayerName)
{
	TSharedPtr<SImage> Image = GetLayerImageByName(LayerName);

	if (!Image.IsValid())
	{
		return;
	}

	Image->SetColorAndOpacity(GetLayerByName(LayerName).LayerColor);
}

// Only reset layers not targeted by the next animation
void SecsCrosshairWidget::StopAllAnimationsExcept(const TArray<FName>& ExcludedLayers, bool bGracefully)
{
	if (bGracefully)
	{
		for (auto AnimLayer : ActiveAnimationLayers)
		{
			if (AnimLayer)
			{
				AnimLayer->bStopRequested = true;
			}
		}
	}
	else
	{
		for (auto Element : Layers)
		{
			if (!ExcludedLayers.Contains(FName(Element.Key)))
			{
				ResetLayerColor(FName(Element.Key));
				ResetLayerTransform(FName(Element.Key));
			}
		}
		for (auto AnimLayer : ActiveAnimationLayers)
		{
			if (AnimLayer)
			{
				AnimLayer->ElapsedTime = 0.0f;
				AnimLayer->PreviousCurveValue = 0.0f;
			}
		}
		ActiveAnimationLayers.Empty();
	}
}

// In PlayAnimation, collect all target layers and use the new function
void SecsCrosshairWidget::PlayAnimation(FecsCrosshairAnimation Animation)
{
	// Gather all target layers for the new animation
	TArray<FName> TargetLayers;
	for (auto AnimLayer : Animation.AnimationLayers)
	{
		if (AnimLayer)
		{
			TargetLayers.Append(AnimLayer->TargetLayers);
		}
	}

	// Stop all current animations, but do not reset layers that will be animated
	StopAllAnimationsExcept(TargetLayers, false);

	// Add new animation layers
	for (auto AnimLayer : Animation.AnimationLayers)
	{
		if (AnimLayer && IsLayersExist(AnimLayer->TargetLayers))
		{
			AnimLayer->AnimationDuration = Animation.AnimationDuration;
			AnimLayer->ElapsedTime = 0.0f;
			AnimLayer->PreviousCurveValue = 0.0f;
			AnimLayer->bStopRequested = false;
			ActiveAnimationLayers.Add(AnimLayer);
		}
	}
}
// StopAnimationLayer: Always remove from ActiveAnimationLayers and reset state
void SecsCrosshairWidget::StopAnimationLayer(UecsCrosshairAnimationLayer* AnimLayer, bool bGracefully)
{
	if (!AnimLayer) return;

	if (bGracefully)
	{
		AnimLayer->bStopRequested = true;
	}
	else
	{
		ActiveAnimationLayers.Remove(AnimLayer);
		AnimLayer->ElapsedTime = 0.0f;
		AnimLayer->PreviousCurveValue = 0.0f;
	}
}

// StopAllAnimations: Remove all and reset state
void SecsCrosshairWidget::StopAllAnimations(bool bGracefully)
{
	if (bGracefully)
	{
		for (auto AnimLayer : ActiveAnimationLayers)
		{
			if (AnimLayer)
			{
				AnimLayer->bStopRequested = true;
			}
		}
	}
	else
	{
		for (auto Element : Layers)
		{
			ResetLayerColor(FName(Element.Key));
			ResetLayerTransform(FName(Element.Key));
		}
		for (auto AnimLayer : ActiveAnimationLayers)
		{
			if (AnimLayer)
			{
				AnimLayer->ElapsedTime = 0.0f;
				AnimLayer->PreviousCurveValue = 0.0f;
			}
		}
		ActiveAnimationLayers.Empty();
	}
}

// StopAnimation: Only stop layers belonging to the given animation
void SecsCrosshairWidget::StopAnimation(FecsCrosshairAnimation Animation)
{
	for (auto AnimLayer : Animation.AnimationLayers)
	{
		StopAnimationLayer(AnimLayer, false);
	}
}

void SecsCrosshairWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TArray<TObjectPtr<UecsCrosshairAnimationLayer>>	LayerCopies = ActiveAnimationLayers;
	for (UecsCrosshairAnimationLayer* AnimationLayer : LayerCopies)
	{
		if (AnimationLayer)
		{
			AnimationLayer->TickLayer(InDeltaTime, this);
		}
	}

	// In Tick
	for (int32 i = 0; i < Behaviors.Num(); ++i)
	{
		if (Behaviors[i])
		{
			BehaviorTickTimers[i] += InDeltaTime;
			if (BehaviorTickTimers[i] >= Behaviors[i]->TickRate)
			{
				Behaviors[i]->Tick();
				BehaviorTickTimers[i] = 0.0f;
			}
		}
	}
}




void UecsCrosshairWidget::ConfigureCrosshairWidget(UecsCrosshairEditorAsset* InEditorAsset)
{
	if (!InEditorAsset)
		return;
	
	if (!CrosshairWidget)
		CrosshairWidget = SNew(SecsCrosshairWidget);
	
	CrosshairEditorAsset = InEditorAsset;
	
	CrosshairWidget->GlobalScale = InEditorAsset->GlobalScale;

	for (const auto& Animation : InEditorAsset->Animations)
	{
		for (const auto& AnimationLayer : Animation.AnimationLayers)
		{
			AnimationLayer->bLoop = false;
		}
	}
	
	CrosshairWidget->SetCrosshairLayers(InEditorAsset->Layers);
	
	CrosshairWidget->SetCrosshairBehaviors(InEditorAsset->Behaviors, this);
}

TSharedRef<SWidget> UecsCrosshairWidget::RebuildWidget()
{
	Super::RebuildWidget();

	if (!CrosshairWidget)
		CrosshairWidget = SNew(SecsCrosshairWidget);
	
	return CrosshairWidget.ToSharedRef();
	
}
