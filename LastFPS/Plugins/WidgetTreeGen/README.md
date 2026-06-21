# WidgetTreeGen

Editor-only Unreal Engine **5.7** plugin that generates UMG Widget Blueprints
(`.uasset`) from a JSON description of the widget hierarchy. Built for the
`LastFPS` project.

> **Editor only.** The single module is `Type: "Editor"`, so nothing here is
> ever compiled into a runtime/packaged build.

---

## What it does

Given a JSON document, the generator:

1. Resolves the parent class (`UUserWidget` or a subclass).
2. Creates a `UWidgetBlueprint` via `UWidgetBlueprintFactory` + `IAssetTools::CreateAsset`.
3. Discards the factory's default root and rebuilds the widget tree recursively
   from the JSON, naming each node and marking it `bIsVariable = true` so it is
   exposed as a Blueprint variable (usable from the designer and the graph).
4. Applies slot properties based on the **parent panel** type:
   - `CanvasPanel` children → absolute `position` / `size` (`UCanvasPanelSlot`).
   - `HorizontalBox` / `VerticalBox` / `Overlay` children → `padding` / `halign` /
     `valign` / `fill` (`UHorizontalBoxSlot` / `UVerticalBoxSlot` / `UOverlaySlot`).
5. `MarkBlueprintAsStructurallyModified` → `CompileBlueprint` → saves the asset.

---

## JSON schema

```json
{
  "parentClass": "/Script/UMG.UserWidget",
  "savePath": "/Game/UI/Generated",
  "assetName": "WBP_Generated",
  "root": {
    "type": "CanvasPanel",
    "name": "RootCanvas",
    "children": [
      {
        "type": "Button",
        "name": "PlayButton",
        "position": [100, 200],
        "size": [240, 64],
        "children": [
          { "type": "TextBlock", "name": "PlayLabel", "text": "Play" }
        ]
      }
    ]
  }
}
```

### Fields

| Field          | Scope            | Notes |
|----------------|------------------|-------|
| `parentClass`  | document         | Full class path. Optional; defaults to `UserWidget`. Overridden by the API arg. |
| `savePath`     | document         | Content path, e.g. `/Game/UI/Generated`. Overridden by the API arg. |
| `assetName`    | document         | e.g. `WBP_Generated`. Overridden by the API arg. |
| `overwrite`    | document         | `true` to regenerate an **existing** asset in place (see below). Default false. |
| `root`         | document         | **Required.** The root node. |
| `type`         | node (required)  | Short name (`Button`, `TextBlock`, ...) **or** a full path (`/Script/UMG.Button`, `/Game/UI/WBP_Foo.WBP_Foo_C`). |
| `name`         | node (required)  | Becomes the widget variable name (sanitized + made unique). |
| `text`         | node             | Text content for text widgets (`TextBlock`). |
| `position`     | CanvasPanel child| `[X, Y]` absolute position. |
| `size`         | CanvasPanel child| `[W, H]` absolute size (also turns off auto-size). |
| `autoSize`     | CanvasPanel child| `true` to auto-size instead of an explicit size. |
| `padding`      | Box/Overlay child| Number (uniform), `[H, V]`, or `[L, T, R, B]`. |
| `halign`       | Box/Overlay child| `Left` / `Center` / `Right` / `Fill`. |
| `valign`       | Box/Overlay child| `Top` / `Center` / `Bottom` / `Fill`. |
| `fill`         | Box child        | Number/`true` → slot uses the **Fill** size rule. |
| `children`     | panel node       | Array of child nodes. Only panel widgets may have children. |

#### Visual styling (optional, applied by widget type)

| Field        | Applies to     | Notes |
|--------------|----------------|-------|
| `width` / `height` | SizeBox  | `SetWidthOverride` / `SetHeightOverride` (px). |
| `minWidth` / `minHeight` | SizeBox | Min desired size (px). |
| `fontSize`   | TextBlock      | Font size (px). |
| `color`      | TextBlock / Image | Text color / image tint, `[R,G,B]` or `[R,G,B,A]` (0..1). |
| `justify`    | TextBlock      | `Left` / `Center` / `Right`. |
| `autoWrap`   | TextBlock      | `true` to auto-wrap. |
| `bgColor`    | Border         | Background brush color `[R,G,B,A]`. |
| `image`      | Image          | Texture object path, e.g. `/Game/.../T_Icon.T_Icon` → `SetBrushFromTexture`. |
| `bgImage`    | Border         | Background texture object path → `SetBrushFromTexture`. |
| `desiredSize`| Image          | `[W,H]` desired size override. |
| `buttonText` | any widget with an FText `ButtonText` property | Sets the common button's label (set via reflection). |

### Widget type resolution (extensible)

`FWidgetTreeGenerator::ResolveWidgetClass` resolves `type` in this order:

1. The friendly short-name map in `WidgetTreeGenerator.cpp`
   (`GetWidgetTypeAliasMap`) — add a line to support more short names.
2. A full object path (anything containing `/` or `.`), e.g. a custom Blueprint
   widget `"/Game/UI/WBP_MyWidget.WBP_MyWidget_C"`.
3. Fallback to `"/Script/UMG.<type>"`.

`ANY_PACKAGE` is **not** used (it is deprecated in UE5).

A ready-to-run sample is at `Resources/sample_widget.json`.

### Regenerating an existing asset (overwrite)

By default, generating onto an existing asset path is an error. Set `"overwrite": true`
(or the `bOverwriteExisting` argument / the request panel checkbox) to **regenerate in place**:
the existing Widget Blueprint is loaded and only its **widget tree** is rebuilt, while its
**parent class, event graph and variables are preserved**. Note that per-widget visual styling
(brushes, colors, fonts, sizes, common-button text) lives on the tree and is therefore reset —
re-apply those in the designer. Close the asset editor before regenerating.

---

## How to run it in the editor

### Option A — Editor menu (no Blueprint needed)

Main menu bar → **LastFPS** → *Widget Tree Gen* section
(the entries are added into the existing LastFPS menu created by the EditorUtility plugin):

- **Open Generator** — opens a details panel for a `UWidgetTreeGenRequest`
  object. Set `Parent Class`, paste `Json Text` (or tick `Read From File` and
  pick a file), optionally override `Save Path` / `Asset Name`, then press the
  **Generate** button (`CallInEditor`).
- **Generate from JSON File...** — file picker; generates immediately and shows
  a notification with the result.

### Option B — Editor Utility Widget (Blutility)

1. Create an **Editor Utility Widget** (Right-click in Content Browser →
   *Editor Utilities → Editor Utility Widget*).
2. Add a button; in its `OnClicked` graph call one of the `BlueprintCallable`
   nodes from **WidgetTreeGen**:
   - **Generate Widget Blueprint From JSON Text** — `JsonText`, `ParentClass`,
     `SavePath`, `AssetName`.
   - **Generate Widget Blueprint From JSON File** — `JsonFilePath`,
     `ParentClass`, `SavePath`, `AssetName`.
   - `SavePath` / `AssetName` may be left empty to use the JSON's values.
   The node returns a `FWidgetTreeGenResult` (`bSuccess`, `AssetPath`, `ErrorMessage`).
3. Run the Editor Utility Widget and click the button.

### Option C — C++

```cpp
#include "WidgetTreeGenerator.h"

FWidgetTreeGenResult Result =
    FWidgetTreeGenerator::GenerateFromJsonFile(
        TEXT("D:/Some/sample_widget.json"));
```

---

## Building

This plugin is enabled in `LastFPS.uproject`. After adding the source, regenerate
project files and build the **editor** target:

```bat
:: 1) Regenerate project files
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
    -projectfiles -project="%CD%\LastFPS\LastFPS.uproject" -game -rocket -progress

:: 2) Build the editor target
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
    LastFPSEditor Win64 Development ^
    -project="%CD%\LastFPS\LastFPS.uproject" -waitmutex
```

Or right-click `LastFPS.uproject` → *Generate Visual Studio project files*, then
build the `LastFPSEditor | Development` configuration in your IDE.

---

## Module dependencies

`UMG`, `UMGEditor`, `UnrealEd`, `AssetTools`, `Kismet`, `Slate`, `SlateCore`,
`Json`, `JsonUtilities`, `EditorScriptingUtilities`, `Blutility`, `ToolMenus`,
`PropertyEditor`, `DesktopPlatform`.

## Source layout

| File | Responsibility |
|------|----------------|
| `WidgetTreeGenTypes.h` | `FWidgetTreeGenResult` (Blueprint-exposed result). |
| `WidgetTreeGenerator.{h,cpp}` | Core: JSON → `FWidgetGenNode` tree → recursive widget build → compile/save. Type→`UClass` map. |
| `WidgetTreeGenLibrary.{h,cpp}` | `UBlueprintFunctionLibrary` — `BlueprintCallable` entry points. |
| `WidgetTreeGenRequest.{h,cpp}` | `UObject` with a `CallInEditor` **Generate** button. |
| `WidgetTreeGenModule.{h,cpp}` | Editor menu entries (Open Generator / Generate from File). |
