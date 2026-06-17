// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetTreeGenerator.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"

// UMG runtime widgets
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/SlateWrapperTypes.h"
#include "Layout/Margin.h"

// Editor-only
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EditorAssetLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogWidgetTreeGen, Log, All);

namespace
{
	/** Parsed representation of a single JSON node, engine-type free until build time. */
	struct FWidgetGenNode
	{
		FString Type;
		FString Name;

		bool bHasText = false;
		FString Text;

		bool bHasPosition = false;
		FVector2D Position = FVector2D::ZeroVector;

		bool bHasSize = false;
		FVector2D Size = FVector2D::ZeroVector;

		bool bAutoSize = false;

		bool bHasPadding = false;
		FMargin Padding = FMargin(0.f);

		FString HAlign;
		FString VAlign;

		bool bHasFill = false;
		float Fill = 1.f;

		TArray<TSharedPtr<FWidgetGenNode>> Children;
	};

	/**
	 * Friendly short-name -> fully-qualified class path map. Easy to extend:
	 * add a line here, or just pass a full "/Script/..." / "/Game/..." path as the
	 * node "type" and it will be resolved by ResolveWidgetClass's fallback.
	 */
	const TMap<FString, FString>& GetWidgetTypeAliasMap()
	{
		static const TMap<FString, FString> Map =
		{
			{ TEXT("CanvasPanel"),            TEXT("/Script/UMG.CanvasPanel") },
			{ TEXT("HorizontalBox"),          TEXT("/Script/UMG.HorizontalBox") },
			{ TEXT("VerticalBox"),            TEXT("/Script/UMG.VerticalBox") },
			{ TEXT("Overlay"),                TEXT("/Script/UMG.Overlay") },
			{ TEXT("GridPanel"),              TEXT("/Script/UMG.GridPanel") },
			{ TEXT("UniformGridPanel"),       TEXT("/Script/UMG.UniformGridPanel") },
			{ TEXT("ScrollBox"),              TEXT("/Script/UMG.ScrollBox") },
			{ TEXT("WrapBox"),                TEXT("/Script/UMG.WrapBox") },
			{ TEXT("SizeBox"),                TEXT("/Script/UMG.SizeBox") },
			{ TEXT("ScaleBox"),               TEXT("/Script/UMG.ScaleBox") },
			{ TEXT("Border"),                 TEXT("/Script/UMG.Border") },
			{ TEXT("WidgetSwitcher"),         TEXT("/Script/UMG.WidgetSwitcher") },
			{ TEXT("Button"),                 TEXT("/Script/UMG.Button") },
			{ TEXT("TextBlock"),              TEXT("/Script/UMG.TextBlock") },
			{ TEXT("RichTextBlock"),          TEXT("/Script/UMG.RichTextBlock") },
			{ TEXT("Image"),                  TEXT("/Script/UMG.Image") },
			{ TEXT("CheckBox"),               TEXT("/Script/UMG.CheckBox") },
			{ TEXT("Slider"),                 TEXT("/Script/UMG.Slider") },
			{ TEXT("ProgressBar"),            TEXT("/Script/UMG.ProgressBar") },
			{ TEXT("Spacer"),                 TEXT("/Script/UMG.Spacer") },
			{ TEXT("EditableText"),           TEXT("/Script/UMG.EditableText") },
			{ TEXT("EditableTextBox"),        TEXT("/Script/UMG.EditableTextBox") },
			{ TEXT("MultiLineEditableText"),  TEXT("/Script/UMG.MultiLineEditableText") },
			{ TEXT("ComboBoxString"),         TEXT("/Script/UMG.ComboBoxString") },
			{ TEXT("Throbber"),               TEXT("/Script/UMG.Throbber") },
			{ TEXT("CircularThrobber"),       TEXT("/Script/UMG.CircularThrobber") },
			{ TEXT("NamedSlot"),              TEXT("/Script/UMG.NamedSlot") },
		};
		return Map;
	}

	EHorizontalAlignment ParseHAlign(const FString& In, EHorizontalAlignment Default)
	{
		if (In.Equals(TEXT("Left"),   ESearchCase::IgnoreCase)) return HAlign_Left;
		if (In.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) return HAlign_Center;
		if (In.Equals(TEXT("Right"),  ESearchCase::IgnoreCase)) return HAlign_Right;
		if (In.Equals(TEXT("Fill"),   ESearchCase::IgnoreCase)) return HAlign_Fill;
		return Default;
	}

	EVerticalAlignment ParseVAlign(const FString& In, EVerticalAlignment Default)
	{
		if (In.Equals(TEXT("Top"),    ESearchCase::IgnoreCase)) return VAlign_Top;
		if (In.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) return VAlign_Center;
		if (In.Equals(TEXT("Bottom"), ESearchCase::IgnoreCase)) return VAlign_Bottom;
		if (In.Equals(TEXT("Fill"),   ESearchCase::IgnoreCase)) return VAlign_Fill;
		return Default;
	}

	bool ParseVector2D(const TSharedPtr<FJsonValue>& Value, FVector2D& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (!Value.IsValid() || !Value->TryGetArray(Arr) || Arr->Num() < 2)
		{
			return false;
		}
		Out.X = (*Arr)[0]->AsNumber();
		Out.Y = (*Arr)[1]->AsNumber();
		return true;
	}

	bool ParsePadding(const TSharedPtr<FJsonValue>& Value, FMargin& Out)
	{
		if (!Value.IsValid())
		{
			return false;
		}

		// Single uniform number.
		double Uniform = 0.0;
		if (Value->TryGetNumber(Uniform))
		{
			Out = FMargin((float)Uniform);
			return true;
		}

		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Value->TryGetArray(Arr))
		{
			if (Arr->Num() == 2)
			{
				Out = FMargin((float)(*Arr)[0]->AsNumber(), (float)(*Arr)[1]->AsNumber());
				return true;
			}
			if (Arr->Num() >= 4)
			{
				Out = FMargin(
					(float)(*Arr)[0]->AsNumber(),
					(float)(*Arr)[1]->AsNumber(),
					(float)(*Arr)[2]->AsNumber(),
					(float)(*Arr)[3]->AsNumber());
				return true;
			}
		}
		return false;
	}

	/** Recursively parse a JSON node object into an FWidgetGenNode. Returns false + error on bad input. */
	bool ParseNode(const TSharedPtr<FJsonObject>& Obj, TSharedPtr<FWidgetGenNode>& OutNode, FString& OutError)
	{
		if (!Obj.IsValid())
		{
			OutError = TEXT("Encountered an invalid (null) node object.");
			return false;
		}

		TSharedPtr<FWidgetGenNode> Node = MakeShared<FWidgetGenNode>();

		if (!Obj->TryGetStringField(TEXT("type"), Node->Type) || Node->Type.IsEmpty())
		{
			OutError = TEXT("A node is missing the required \"type\" field.");
			return false;
		}
		if (!Obj->TryGetStringField(TEXT("name"), Node->Name) || Node->Name.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Node of type \"%s\" is missing the required \"name\" field."), *Node->Type);
			return false;
		}

		Node->bHasText = Obj->TryGetStringField(TEXT("text"), Node->Text);

		if (Obj->HasField(TEXT("position")))
		{
			Node->bHasPosition = ParseVector2D(Obj->TryGetField(TEXT("position")), Node->Position);
		}
		if (Obj->HasField(TEXT("size")))
		{
			Node->bHasSize = ParseVector2D(Obj->TryGetField(TEXT("size")), Node->Size);
		}
		Obj->TryGetBoolField(TEXT("autoSize"), Node->bAutoSize);

		if (Obj->HasField(TEXT("padding")))
		{
			Node->bHasPadding = ParsePadding(Obj->TryGetField(TEXT("padding")), Node->Padding);
		}
		Obj->TryGetStringField(TEXT("halign"), Node->HAlign);
		Obj->TryGetStringField(TEXT("valign"), Node->VAlign);

		double FillValue = 0.0;
		if (Obj->TryGetNumberField(TEXT("fill"), FillValue))
		{
			Node->bHasFill = true;
			Node->Fill = (float)FillValue;
		}
		else
		{
			bool bFillFlag = false;
			if (Obj->TryGetBoolField(TEXT("fill"), bFillFlag) && bFillFlag)
			{
				Node->bHasFill = true;
				Node->Fill = 1.f;
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* ChildArr = nullptr;
		if (Obj->TryGetArrayField(TEXT("children"), ChildArr))
		{
			for (const TSharedPtr<FJsonValue>& ChildVal : *ChildArr)
			{
				const TSharedPtr<FJsonObject>* ChildObj = nullptr;
				if (!ChildVal->TryGetObject(ChildObj))
				{
					OutError = FString::Printf(TEXT("Node \"%s\" has a non-object entry in its \"children\" array."), *Node->Name);
					return false;
				}
				TSharedPtr<FWidgetGenNode> ChildNode;
				if (!ParseNode(*ChildObj, ChildNode, OutError))
				{
					return false;
				}
				Node->Children.Add(ChildNode);
			}
		}

		OutNode = Node;
		return true;
	}

	/** Sanitize an arbitrary name to a valid, unique FName within the tree. */
	FName MakeUniqueWidgetName(const FString& Raw, TSet<FName>& UsedNames)
	{
		FString Clean;
		Clean.Reserve(Raw.Len());
		for (TCHAR Ch : Raw)
		{
			if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
			{
				Clean.AppendChar(Ch);
			}
			else
			{
				Clean.AppendChar(TEXT('_'));
			}
		}
		if (Clean.IsEmpty() || FChar::IsDigit(Clean[0]))
		{
			Clean = TEXT("W_") + Clean;
		}

		FString Candidate = Clean;
		int32 Suffix = 1;
		while (UsedNames.Contains(FName(*Candidate)))
		{
			Candidate = FString::Printf(TEXT("%s_%d"), *Clean, Suffix++);
		}

		const FName Result(*Candidate);
		UsedNames.Add(Result);
		return Result;
	}

	/** Apply slot properties appropriate to the parent panel type. */
	void ApplySlot(UPanelSlot* Slot, const FWidgetGenNode& Node)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			if (Node.bHasPosition)
			{
				CanvasSlot->SetPosition(Node.Position);
			}
			if (Node.bHasSize)
			{
				CanvasSlot->SetAutoSize(false);
				CanvasSlot->SetSize(Node.Size);
			}
			else if (Node.bAutoSize)
			{
				CanvasSlot->SetAutoSize(true);
			}
			return;
		}

		if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
		{
			if (Node.bHasPadding) HBoxSlot->SetPadding(Node.Padding);
			if (!Node.HAlign.IsEmpty()) HBoxSlot->SetHorizontalAlignment(ParseHAlign(Node.HAlign, HAlign_Fill));
			if (!Node.VAlign.IsEmpty()) HBoxSlot->SetVerticalAlignment(ParseVAlign(Node.VAlign, VAlign_Fill));
			if (Node.bHasFill)  HBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			return;
		}

		if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Slot))
		{
			if (Node.bHasPadding) VBoxSlot->SetPadding(Node.Padding);
			if (!Node.HAlign.IsEmpty()) VBoxSlot->SetHorizontalAlignment(ParseHAlign(Node.HAlign, HAlign_Fill));
			if (!Node.VAlign.IsEmpty()) VBoxSlot->SetVerticalAlignment(ParseVAlign(Node.VAlign, VAlign_Fill));
			if (Node.bHasFill)  VBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			return;
		}

		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
		{
			if (Node.bHasPadding) OverlaySlot->SetPadding(Node.Padding);
			if (!Node.HAlign.IsEmpty()) OverlaySlot->SetHorizontalAlignment(ParseHAlign(Node.HAlign, HAlign_Fill));
			if (!Node.VAlign.IsEmpty()) OverlaySlot->SetVerticalAlignment(ParseVAlign(Node.VAlign, VAlign_Fill));
			return;
		}

		// Other slot types (Button content, Border, etc.) carry no positional data here.
	}

	/** Recursively construct widgets and wire them into the tree. */
	UWidget* BuildWidget(UWidgetTree* Tree, const FWidgetGenNode& Node, TSet<FName>& UsedNames, FString& OutError)
	{
		UClass* WidgetClass = FWidgetTreeGenerator::ResolveWidgetClass(Node.Type);
		if (!WidgetClass)
		{
			OutError = FString::Printf(TEXT("Could not resolve widget type \"%s\" (node \"%s\")."), *Node.Type, *Node.Name);
			return nullptr;
		}

		const FName WidgetName = MakeUniqueWidgetName(Node.Name, UsedNames);
		UWidget* Widget = Tree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
		if (!Widget)
		{
			OutError = FString::Printf(TEXT("Failed to construct widget \"%s\" of type \"%s\"."), *Node.Name, *Node.Type);
			return nullptr;
		}

		// Expose as a Blueprint variable, accessible from designer & graph.
		Widget->bIsVariable = true;

		// Text content for text-style widgets.
		if (Node.bHasText)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				TextBlock->SetText(FText::FromString(Node.Text));
			}
		}

		// Children.
		if (Node.Children.Num() > 0)
		{
			UPanelWidget* AsPanel = Cast<UPanelWidget>(Widget);
			if (!AsPanel)
			{
				OutError = FString::Printf(TEXT("Widget \"%s\" (type \"%s\") is not a panel but has children."), *Node.Name, *Node.Type);
				return nullptr;
			}

			for (const TSharedPtr<FWidgetGenNode>& ChildNode : Node.Children)
			{
				UWidget* ChildWidget = BuildWidget(Tree, *ChildNode, UsedNames, OutError);
				if (!ChildWidget)
				{
					return nullptr;
				}

				UPanelSlot* Slot = AsPanel->AddChild(ChildWidget);
				if (Slot)
				{
					ApplySlot(Slot, *ChildNode);
				}
			}
		}

		return Widget;
	}
}

UClass* FWidgetTreeGenerator::ResolveWidgetClass(const FString& TypeString)
{
	if (TypeString.IsEmpty())
	{
		return nullptr;
	}

	auto LoadAsWidgetClass = [](const FString& Path) -> UClass*
	{
		UClass* Loaded = LoadObject<UClass>(nullptr, *Path);
		if (Loaded && Loaded->IsChildOf(UWidget::StaticClass()))
		{
			return Loaded;
		}
		return nullptr;
	};

	// 1) Friendly short-name map.
	if (const FString* MappedPath = GetWidgetTypeAliasMap().Find(TypeString))
	{
		if (UClass* Resolved = LoadAsWidgetClass(*MappedPath))
		{
			return Resolved;
		}
	}

	// 2) Already a full object path ("/Script/UMG.Button", "/Game/UI/WBP_Foo.WBP_Foo_C").
	if (TypeString.Contains(TEXT("/")) || TypeString.Contains(TEXT(".")))
	{
		if (UClass* Resolved = LoadAsWidgetClass(TypeString))
		{
			return Resolved;
		}
	}

	// 3) Fall back to assuming it is a UMG engine widget.
	return LoadAsWidgetClass(FString::Printf(TEXT("/Script/UMG.%s"), *TypeString));
}

FWidgetTreeGenResult FWidgetTreeGenerator::GenerateFromJsonString(
	const FString& JsonString,
	TSubclassOf<UUserWidget> ParentClassOverride,
	const FString& SavePathOverride,
	const FString& AssetNameOverride)
{
	// --- Parse document ---
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return FWidgetTreeGenResult::MakeError(TEXT("Failed to parse JSON document."));
	}

	// --- Resolve parent class ---
	UClass* ParentClass = ParentClassOverride.Get();
	if (!ParentClass)
	{
		FString ParentClassPath;
		if (RootObject->TryGetStringField(TEXT("parentClass"), ParentClassPath) && !ParentClassPath.IsEmpty())
		{
			ParentClass = LoadObject<UClass>(nullptr, *ParentClassPath);
		}
	}
	if (!ParentClass)
	{
		ParentClass = UUserWidget::StaticClass();
	}
	if (!ParentClass->IsChildOf(UUserWidget::StaticClass()))
	{
		return FWidgetTreeGenResult::MakeError(
			FString::Printf(TEXT("Parent class \"%s\" is not a UUserWidget subclass."), *ParentClass->GetPathName()));
	}

	// --- Resolve save path / asset name (function args win over JSON) ---
	FString SavePath = SavePathOverride;
	if (SavePath.IsEmpty())
	{
		RootObject->TryGetStringField(TEXT("savePath"), SavePath);
	}
	FString AssetName = AssetNameOverride;
	if (AssetName.IsEmpty())
	{
		RootObject->TryGetStringField(TEXT("assetName"), AssetName);
	}
	// Default save path when none is specified anywhere.
	if (SavePath.IsEmpty())
	{
		SavePath = TEXT("/Game/UI");
	}
	if (AssetName.IsEmpty())
	{
		return FWidgetTreeGenResult::MakeError(TEXT("\"assetName\" must be provided (in JSON or as an argument)."));
	}
	SavePath.RemoveFromEnd(TEXT("/"));

	// --- Parse the widget node tree ---
	const TSharedPtr<FJsonObject>* RootNodeObj = nullptr;
	if (!RootObject->TryGetObjectField(TEXT("root"), RootNodeObj))
	{
		return FWidgetTreeGenResult::MakeError(TEXT("JSON is missing the required \"root\" object."));
	}
	TSharedPtr<FWidgetGenNode> RootNode;
	FString ParseError;
	if (!ParseNode(*RootNodeObj, RootNode, ParseError))
	{
		return FWidgetTreeGenResult::MakeError(ParseError);
	}

	// --- Create the asset via the widget blueprint factory ---
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, SavePath, UWidgetBlueprint::StaticClass(), Factory);
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(NewAsset);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return FWidgetTreeGenResult::MakeError(
			FString::Printf(TEXT("Failed to create Widget Blueprint \"%s\" at \"%s\" (name may already be in use)."), *AssetName, *SavePath));
	}

	UWidgetTree* Tree = WidgetBP->WidgetTree;

	// The factory may have created a default root (e.g. CanvasPanel). Discard it so our
	// tree fully owns the hierarchy and there are no name collisions.
	if (UWidget* OldRoot = Tree->RootWidget)
	{
		OldRoot->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
		Tree->RootWidget = nullptr;
	}

	// --- Build the hierarchy ---
	TSet<FName> UsedNames;
	FString BuildError;
	UWidget* BuiltRoot = BuildWidget(Tree, *RootNode, UsedNames, BuildError);
	if (!BuiltRoot)
	{
		return FWidgetTreeGenResult::MakeError(BuildError);
	}
	Tree->RootWidget = BuiltRoot;

	// --- Compile & save ---
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	if (WidgetBP->Status == BS_Error)
	{
		return FWidgetTreeGenResult::MakeError(
			FString::Printf(TEXT("Widget Blueprint \"%s\" was created but failed to compile."), *AssetName));
	}

	const FString ObjectPath = FString::Printf(TEXT("%s/%s"), *SavePath, *AssetName);
	if (!UEditorAssetLibrary::SaveLoadedAsset(WidgetBP, /*bOnlyIfIsDirty=*/false))
	{
		UE_LOG(LogWidgetTreeGen, Warning, TEXT("Generated \"%s\" but saving to disk failed."), *ObjectPath);
	}

	UE_LOG(LogWidgetTreeGen, Log, TEXT("Generated Widget Blueprint: %s"), *ObjectPath);
	return FWidgetTreeGenResult::MakeSuccess(ObjectPath);
}

FWidgetTreeGenResult FWidgetTreeGenerator::GenerateFromJsonFile(
	const FString& JsonFilePath,
	TSubclassOf<UUserWidget> ParentClassOverride,
	const FString& SavePathOverride,
	const FString& AssetNameOverride)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
	{
		return FWidgetTreeGenResult::MakeError(FString::Printf(TEXT("Could not read JSON file: %s"), *JsonFilePath));
	}
	return GenerateFromJsonString(JsonString, ParentClassOverride, SavePathOverride, AssetNameOverride);
}
