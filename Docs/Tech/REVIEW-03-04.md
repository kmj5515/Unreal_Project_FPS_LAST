# REVIEW: 03-data-driven.md / 04-ui-presentation.md 코드 대조 검수

> 검수 방식: 문서에서 검증 가능한 주장(심볼명·파일:줄번호·개수·동작)을 추출해 `LastFPS/Source/LastFPS/`, `LastFPS/Config/`, `Tools/` 원본과 대조.
> 판정: **정확 / 부정확 / 과장 / 미검증**
> 코드·대상 문서 무수정, 빌드 미실행.

---

## Ⅰ. 작성자 자가신고 6건 결론

### (a) `ULastFPSBattleDefinition`의 실제 필드와 소비자 — "미검증" 표기는 해소 가능

**파일**: `LastFPS/Source/LastFPS/Data/Definitions/LastFPSBattleDefinition.h` (`UCLASS(BlueprintType, Const)`, `UPrimaryDataAsset` 파생)

필드 9개:

| 필드 | 타입 | 성격 |
|---|---|---|
| `MapId` | `FPrimaryAssetId` (`meta=(AllowedTypes="Map")`) | CommonSession이 호스트할 월드 |
| `MatchmakingPoolTag` | `FGameplayTag` (`Matchmaking.Pool`) | 퀵플레이 세션 호환 식별자 |
| `MaxPlayerCount` | `int32` = 3 | |
| `ContentSet` | `TSoftObjectPtr<ULastFPSDestinationContentSet>` (`AssetBundles="Game"`) | 목적지 콘텐츠 묶음 |
| `DropProfile` | `TSoftObjectPtr<ULastFPSDropProfile>` (`AssetBundles="Game"`) | 레벨별 드랍 규칙 |
| `DisplayName` / `DisplayNameStringTableKey` / `Description` / `Thumbnail` | FText·FName·FText·TSoftObjectPtr\<UTexture2D\> | 표시 데이터 |

**소비자** (참조 파일 17개 중 실사용): `Game/LastFPSGameModeBase`, `Game/Travel/LastFPSLevelTravelSettings`·`LastFPSLevelTravelSubsystem`, `Network/LastFPSMasterLobbyClientSubsystem`, `Network/Party/LastFPSPartyGameMode`, `UI/Map/LastFPSMapScreenWidget`, `Localization/LastFPSLocalization`.

→ 문서 표의 "소비하는 시스템: `ALastFPSGameModeBase` 계열"은 **과소 서술**이다. 실제로는 **트래블·매치메이킹·마스터 로비·파티·맵 화면**이 함께 소비하는 핵심 계약이다. 클래스 주석도 "세션 검색 결과나 참가 상태 같은 런타임 값은 이 에셋에 저장하지 않는다"고 명시해, 문서 ③장(불변/런타임 분리)의 **최상급 근거인데 문서가 이를 버렸다.**

**수정 제안**: "(미검증 — 파일 존재만 확인)" 삭제 후 표 행 교체 —
`| ULastFPSBattleDefinition | 전투 목적지 1개의 불변 규칙 (MapId·매치메이킹 풀 태그·최대 인원·ContentSet·DropProfile·표시 데이터) | LevelTravelSubsystem / MasterLobbyClientSubsystem / PartyGameMode / MapScreenWidget / GameModeBase |`
③장 표에 `ULastFPSBattleDefinition (불변) ↔ 세션 검색·참가 상태 (런타임)` 행 추가 권장.

---

### (b) Excel → CSV → DataTable 임포트 — **자동화 툴 실재. 수동 아님.**

- `Tools/Excel/xlsx_to_csv.py` — "Excel 워크시트를 Unreal 임포트용 CSV로 변환한다". 생성 CSV 상단에 `# 자동 생성됨 - 직접 수정하지 말고 원본 xlsx를 편집하세요.` 배너를 삽입.
- `Tools/Excel/verify_roundtrip.py` — xlsx↔csv 왕복 검증.
- `Tools/Excel/bootstrap_from_csv.py` — 역방향 부트스트랩.
- `LastFPS/Plugins/EditorUtility/Content/Python/import_datatables.py` — **"EditorUtility C++ 서비스가 호출하는 xlsx 파싱 진입점"**. 즉 전용 에디터 플러그인이 호출한다.
- 원본: `LastFPS/Excel/{Battle,Core,Hub,Localize}.xlsx` → `LastFPS/Excel/Csv/*.csv` (17개).

→ 문서 ①장 mermaid는 `XLSX --> CSV --> DT` 화살표만 그리고 **누가 그 화살표를 실행하는지 쓰지 않았다.** 채용 관점에서 가장 아까운 누락이며, "엑셀을 손으로 저장하는가?"라는 의심을 남긴다.

**수정 제안**:
`XLSX -->|Tools/Excel/xlsx_to_csv.py| CSV -->|EditorUtility 플러그인 + import_datatables.py| DT`
및 본문 한 줄 — "CSV는 생성물이라 직접 편집 금지 배너가 박혀 있고, `verify_roundtrip.py`로 xlsx↔csv 일치를 검증한다."

---

### (c) `LoadDataSet`의 `WaitUntilComplete()` — 추정 결론은 맞고, 코드 근거가 실재한다

`LastFPSGameDataSubsystem.h:37`

```cpp
/** 다른 GameInstanceSubsystem이 초기화될 때 즉시 사용할 전역 데이터 묶음이다. */
UPROPERTY(Config, EditDefaultsOnly, ...) FPrimaryAssetId StartupDataSetId;
```

이 주석이 곧 근거다. 추가로 `FindTable()` / `GetCharacterRoster()` / `GetScreenRegistry()` 가 전부 진입 즉시 `EnsureStartupDataSetLoaded()` 를 호출하는 **동기 조회 API**여서, 비동기로 바꾸면 전 호출부 계약이 깨진다. `LoadDataSet`이 `bool`을 반환하고 실패 시 즉시 Error 로그를 남기는 구조도 동기 전제를 뒷받침한다.

**판정**: 결론은 정확하나 "~로 읽힌다"는 추정 서술이 불필요하게 약하다.

**수정 제안**: "전역 데이터라 초기화 순서를 확정하려는 의도로 읽힌다" →
"`StartupDataSetId` 주석이 '다른 GameInstanceSubsystem이 초기화될 때 즉시 사용할' 묶음임을 명시하고, 모든 조회 API가 진입 시 `EnsureStartupDataSetLoaded()`를 부르는 동기 계약이라 블로킹이 의도된 선택이다."

---

### (d) `bEnableDefaultInputConfig` — **정확. `LastFPS/Config/DefaultGame.ini:12`에 실제 `=False`.**

주석만 보고 쓴 것이 아니라 실제 ini 값과 일치한다. 코드 3곳(`LastFPSPlayerController.cpp:498`, `LastFPSActivatableWidget.cpp:17`, `LastFPSActivatableWidget.h:37`)이 이 설정을 전제로 주석을 남기고 있어 일관적이다. **수정 불필요.**

---

### (e) HUD Presenter "9종" — **부정확. 실제 10종.**

`UI/HUD/Presenters/` 에 9개(Ammo, CombatFeedback, EnemyHealth, GrapplingReticle, ObjectiveHud, Reload, SkillCooldown, VitalsGauge, WeaponSlot) + **`UI/HUD/LastFPSEasyCrosshairPresenter`** (폴더 밖). `LastFPSHUDWidget.h:371`에 `TObjectPtr<ULastFPSEasyCrosshairPresenter> CrosshairPresenter` 멤버가 있고 클래스 주석(`:55`)도 이를 열거한다. **View가 소유·조립하는 Presenter는 10개.**

데이터 원본 매핑 검증:

- **`Reload`** — 개념은 맞고 **심볼명이 틀렸다.** 실제 멤버는 `ReloadProgressParameterName`이 아니라 `FName ProgressParameterName = TEXT("ReloadProgress")` (`LastFPSReloadPresenter.h:43`)이며 `Initialize(UImage*, UOverlay*, FName InProgressParameterName)`로 주입된다. 머티리얼 파라미터인 것은 사실 — `LastFPSReloadPresenter.cpp:82-84`에서 `ReloadImage->GetDynamicMaterial()->SetScalarParameterValue(ProgressParameterName, Progress)`. 다만 진행값은 델리게이트가 아니라 `SetReloadStarted(Duration)` 이후 `Tick` 경과 누적이라, 2.3장 표의 "게임플레이 다이내믹 델리게이트" 분류와 어긋난다(시작/종료만 이벤트).
- **`EnemyHealth`** — 정확. 진입점 `HandleDamage(...)` (`LastFPSEnemyHealthPresenter.cpp:25`), 보스는 `ObjectiveHudSubsystem` 점유(`RequestBossPresentation`)를 거친다.
- **`WeaponSlot`** — 정확. `BindToWeaponComponent`에서 `OnWeaponLoadoutChanged` / `OnWeaponSlotChanged`를 `AddUniqueDynamic`으로 구독(`LastFPSWeaponSlotPresenter.cpp:34-37`).

**수정 제안**: "Presenter 9종" → "10종", 표에 `| EasyCrosshair | 크로스헤어 표시·확산·발사 연출 | 무기 정의의 CrosshairSpreadBehavior 에셋 런타임 사본 |` 추가. ⑨장 요약표 "Presenter 9개"도 동시 수정. Reload 행 원본은 "리로드 시작/종료 이벤트 + 틱 진행률 → 머티리얼 스칼라 파라미터(`ProgressParameterName`, 기본값 `ReloadProgress`)"로.

---

### (f) `OnCinematicStarted` 브로드캐스트 시점 — **시퀀스 로드 완료 후. 문서 다이어그램 정확.**

`LastFPSCinematicPlaybackSubsystem.cpp`: `BeginLoad()`(`:158`) → `RequestAsyncLoad(..., HandleSequenceLoaded)`(`:167-169`) → `HandleSequenceLoaded()`(`:183`) 내부에서 시퀀스 null 체크 → `CreateLevelSequencePlayer` → `SequencePlayer->Play()` → **`OnCinematicStarted.Broadcast(PendingRequest.bSkippable)` (`:232`)**.

즉 **로드 완료 + 플레이어 생성 + Play() 직후**다. 로드 실패·플레이어 생성 실패 시에는 `FinishPlayback(false)`로 빠져 **Started가 아예 발화하지 않는다** — UI가 "숨김 상태로 갇히는" 사고가 구조적으로 불가능하다. **수정 불필요이나**, 이 실패 경로 대칭성은 문서가 놓친 강점이라 한 줄 추가 권장.

---

## Ⅱ. 판정표 (52건)

| # | 문서 | 주장 | 판정 | 근거 |
|---|---|---|---|---|
| 1 | 03 | Primary Asset Type "7종" 등록 | **정확** | `LastFPSPrimaryAssetTypes.h:8-14` = BattleDefinition/CharacterDefinition/DestinationContentSet/GameDataSet/Map/PopupCatalog/WeaponDefinition 7개. `DefaultGame.ini` `PrimaryAssetTypesToScan`도 동일 7 + PrimaryAssetLabel |
| 2 | 03 | 위 표에 9행 나열 | **부정확** | "7종"이라 쓰고 9행을 실었다. `ULastFPSHeroDefinition`·`ULastFPSEnemyDefinition`·`ULastFPSCharacterRoster`는 **등록된 Primary Asset Type이 아니다**(앞 둘은 CharacterDefinition 타입으로 스캔되는 파생, Roster는 GameDataSet이 참조). 반대로 등록된 `Map` 타입은 표에서 누락. → **수정: "등록 타입 7종"과 "그 파생·부속 에셋"으로 표를 분리하고 `Map` 행 추가** |
| 3 | 03 | `LastFPSPrimaryAssetTypes.h:8-14` | **정확** | 해당 범위가 정확히 7개 `extern` 선언 |
| 4 | 03 | `ULastFPSDestinationFeature`의 가상 함수는 `CollectRequiredPaths` **단 하나** | **정확** | `UCLASS(Abstract, BlueprintType)` + 빈 기본 구현 1개뿐 |
| 5 | 03 | `LastFPSDestinationFeature.h:11-19` | **정확** | 실제 9-21, ±10 이내 |
| 6 | 03 | `DestinationContentSet.cpp:37` Features 순회, switch·캐스팅 없음 | **정확** | `.cpp:33-39` 순회 루프, 캐스팅 없음 |
| 7 | 03 | `DestinationContentSet.h:101-102` Features 배열 | **정확** | 실제 `:102` `TArray<TObjectPtr<ULastFPSDestinationFeature>> Features` |
| 8 | 03 | `ULastFPSActorPoolProfile` 구성 `LastFPSActorPoolProfile.h:192` / `:165-166` / `:159-185` | **부정확** | 해당 파일은 **54줄**. 세 인용 모두 파일 범위 밖. 필드 자체는 존재(`PoolId` `:17`, `ActorClass` `:21`, `InitialSize` `:26`, `MaxSize` `:31`, `bRenderWarmup` `:35`). → **수정: `:16-35`** |
| 9 | 03 | DataTable Row 타입 "15종"이 **전부 `Data/Tables/`에 모여 있다** | **부정확** | 실제 `FTableRowBase` 파생 **17종**. 누락 2종은 `FLastFPSStatusEffectUIData`(`Data/Status/`), `FLastFPSNPCSpawnData`(`Hub/`) — **둘 다 `Data/Tables/` 밖**이라 "모여 있다"도 사실이 아니다. 대응 CSV(`DT_StatusEffectData.csv`, `DT_NPCData.csv`)도 존재 |
| 10 | 03 | 원본 CSV 목록 | **정확(부분)** | `LastFPS/Excel/Csv/` = **17개**로 일치. 단 표는 13개만 언급하고 `DT_NPCData` `DT_StatusEffectData` `LocalizeTable` 누락. `FLastFPSRarityVisualData`의 CSV 없음 표기는 정직 ✅ |
| 11 | 03 | 행 이름 규약이 조인 키, `LastFPSModuleData.h:41-47` | **정확** | 인용 주석이 해당 위치에 그대로 실재 |
| 12 | 03 | `TMap<FGameplayTag,float> Parameters` + `GetParameter`, `LastFPSWeaponBalanceData.h:42-49` | **정확** | 실제 41-49. 인용 코드 블록도 원문과 문자 단위 일치 |
| 13 | 03 | `LastFPSWeaponBalanceData.h:10` 주석 "에셋 구성과 분리해 관리하는" | **정확** | 실제 `:9`, ±10 이내 |
| 14 | 03 | `ULastFPSGameDataSet`/`ULastFPSDropProfile`이 `UCLASS(..., Const)` | **정확** | `LastFPSGameDataSet.h:29`, `LastFPSDropProfile.h:36` |
| 15 | 03 | 그 근거를 `LastFPSGameDataSet.h:101` / `LastFPSDropProfile.h:39`로 표기 | **부정확** | GameDataSet.h는 **64줄** — `:101`, `:125-135`(AssetBundles) 전부 범위 밖. 실제 AssetBundles는 `:46,50,54,58,62`. DropProfile `:39`도 실제 `:36`. → **수정: `GameDataSet.h:29` / `:46-62`, `DropProfile.h:36`** |
| 16 | 03 | 번들 이름이 `StartupTables`/`GameTables`/`UITables` | **부정확** | 실제 번들명은 `Startup`/`Game`/`UI` (`LastFPSPrimaryAssetTypes.h:17-23`, `meta=(AssetBundles="Startup")` 등). `StartupTables`류 식별자는 코드에 없다(배열 변수명과 혼동). 또한 `Audio` 번들 상수도 선언돼 있어 번들은 4종 |
| 17 | 03 | `LoadDataSet()`이 세 번들을 함께 요청, `.cpp:135-144` | **정확** | 135-138에서 Startup/Game/UI를 `BundlesToLoad`에 담고 140에서 `LoadPrimaryAsset` |
| 18 | 03 | `WaitUntilComplete()`가 `.cpp:143` | **정확** | 줄번호까지 정확 |
| 19 | 03 | `StartupDataSetId`/`OnDemandDataSetIds`가 `UPROPERTY(Config)`, `.h:61-66` | **부정확(줄번호)** | Config 속성인 것은 사실. 실제 `:38`, `:42`. → **수정: `.h:37-43`** |
| 20 | 03 | `FindBalance(FName) const`가 const 포인터 반환, `WeaponDataSubsystem.h:49` | **정확** | 실제 `:47`, ±10 이내 |
| 21 | 03 | 로딩 진행률 가중치 0.40 / 0.20 / 0.15 / 0.25 | **정확** | `LastFPSDestinationContentSet.h:24-36`, `AssetAndGameplayCueWeight` `ActorPoolWeight` `RenderComponentWeight` `ShaderAndPSOWeight` 값 4개 모두 일치 (문서가 언급 안 한 `OverallProgressShare=0.70f`도 존재) |
| 22 | 03 | `AreEquipDependenciesResident()`/`GatherEquipDependencyPaths()`가 `WeaponDefinition.h:220-223` | **정확** | 실제 `:220`, `:223`. 인용한 `WeaponComponent.cpp:1106-1123` 코드도 원문과 일치 |
| 23 | 03 | `GatherSpawnDependencyPaths` `.h:310`, `ClassificationTags`·`HasClassificationTag` `.h:266-271` | **부정확** | `LastFPSCharacterDefinition.h`는 **87줄**. 실제 `ClassificationTags` `:44`, `HasClassificationTag` `:47`, `GatherSpawnDependencyPaths` `:86`. 심볼은 전부 존재하나 **인용 줄번호 3개 모두 파일 범위 밖** |
| 24 | 03 | `ApplyWeaponDefinitionValues()`가 `LoadSynchronous()` 사용, `.cpp:1015-1024` | **정확** | 함수 정의 `:985`, 인용 범위가 `LoadSynchronous()` 연속 호출부. 주석 "장착 시점에 즉시 필요한 참조라 동기 로드한다"도 `:1014`에 실재 |
| 25 | 03 | 쿠킹 참조 그래프 실패 사례(`RarityVisualData.h:9-17`, `DestinationContentSet.h:91-96`) | **정확** | 두 주석 모두 인용 위치에 실재. `DefaultGame.ini`에도 동일 취지 주석("ini 의 경로 문자열은 쿡 참조 그래프에 잡히지 않아…")이 있어 교차 확인됨 |
| 26 | 03 | `LastFPSTags.h` 88개 선언 | **정확** | `grep -c UE_DECLARE_GAMEPLAY_TAG_EXTERN` = 88 |
| 27 | 03 | `LastFPSGameDataTags.h` **18개**, 전부 `Data.Table.*` | **부정확** | 실제 **17개**(파일 24줄). 전부 `Data.Table.*` 네임스페이스인 것은 사실. → **수정: 17개** |
| 28 | 03 | `WeaponComponent`에 `switch (WeaponType)`·무기 이름 비교 **0건** | **정확** | `WeaponComponent.cpp` 전체에 `switch` 0건. `EMMWeaponType`은 값 대입/전달로만 쓰이고 분기 없음 |
| 29 | 03 | `EMMWeaponType`은 `Unarmed/Rifle/Pistol` 3값, `LastFPSEnumTypes.h:59-64` | **정확** | 실제 58-64 |
| 30 | 03 | 액터 풀 소비 지점 **9개** | **부정확** | `ActorPoolSubsystem`을 참조하는 `Pooling/` 밖 `.cpp`는 **11개**. 목록 누락 2개: `Game/LastFPSGameModeBase.cpp`(문서 mermaid에는 GM이 그려져 있으면서 목록에서 빠짐), `Projectiles/LastFPSProjectileLaunchUtility.cpp` |
| 31 | 03 | `ILastFPSPoolableActor` 3함수, `LastFPSPoolableActor.h:130-148` | **부정확(줄번호)** | 함수 3개(`OnAcquiredFromPool` `:24`, `OnReleasedToPool` `:28`, `OnPrepareForPoolRenderWarmup` `:35`)는 정확. 파일은 **36줄**이라 인용 범위가 밖. → **수정: `:24-35`** |
| 32 | 03 | 풀 소진 시 `bExhaustionReported` 래치 + nullptr 폴백, `.cpp:246-262` | **정확** | 실제 249-262, 인용 코드 문자 단위 일치 |
| 33 | 03 | `PreparePools()` "동기 로드 허용 안 함" 계약, `ActorPoolSubsystem.h:50-53` | **정확** | `:50-53` 주석 그대로 |
| 34 | 03 | Equipment는 `UGameInstanceSubsystem`, ActorPool은 `UWorldSubsystem` (`.h:41` 양쪽) | **정확** | `LastFPSEquipmentSubsystem.h:41` = `public UGameInstanceSubsystem`, `LastFPSActorPoolSubsystem.h:41` = `public UWorldSubsystem`. **줄번호까지 정확** |
| 35 | 03 | Economy 시작 시 테이블 간 참조 무결성 검사, `.h:88-90` | **정확** | 인용 주석 문장 그대로 실재 |
| 36 | 03 | `ULastFPSEncounterObjectiveDefinition` OCP 보장 주석 `.h:20-21` | **정확** | `:20-21` "신규 목표 유형은 이 클래스를 상속해 ConfigureRuntimeObjective 만 구현하면 되며…" 일치 |
| 37 | 03 | `ULastFPSDropProfile` 한계 자인 주석 `.h:35-36` | **정확** | `:34` "등급이 더 필요해지면 분류 태그 기반으로 일반화한다" |
| 38 | 03 | 요약표 "경로 문자열 하드코딩 **없음** — 전부 GameplayTag + PrimaryAssetId" | **과장** | `DefaultGame.ini`에 `ActiveTheme=/Game/UI/OutUI/Framework/DA_UITheme_Default`, `PreviewStageClass=/Game/UI/OutUI/Preview/BP_PreviewStage_C`, `WaveEnemyMarkerWidgetClass=…` 등 **ini 경로 문자열이 실재**. 문서 4-4장이 스스로 "ini 경로 문자열은 쿡 참조 그래프에 안 잡힌다"고 지적해 놓고 요약에서 "없음"이라 단정 → 자기모순. → **수정: "DataTable/Definition 조회 경로에는 없음. UI 테마·프리뷰 스테이지 등 일부 설정은 여전히 ini 경로 문자열이며 4-4장의 쿡 참조 리스크가 동일하게 남아 있다"** |
| 39 | 03 | ①장 mermaid의 XLSX→CSV→DT 파이프라인 | **미검증(문서 측 누락)** | 실제로는 `Tools/Excel/*.py` 3종 + EditorUtility 플러그인이 존재. Ⅰ-(b) 참조 |
| 40 | 03 | 신규 무기는 `WeaponDefinition` 에셋 + `DT_WeaponBalance` 행이면 끝 | **정확(조건부)** | 무기 조회는 `FindBalance(FName WeaponId)` 단일 경로. 다만 문서 스스로 ⑥-⚠️1에서 애님 레이어 분류(`EMMWeaponType`) 제약을 인정하고 있어 서술이 균형적 |
| 41 | 04 | `bEnableDefaultInputConfig=False` | **정확** | `LastFPS/Config/DefaultGame.ini:12` |
| 42 | 04 | HUD Presenter **9종** | **부정확** | 실제 10종(EasyCrosshair 누락). Ⅰ-(e) 참조. ⑨장 요약표 "Presenter 9개"도 동일 |
| 43 | 04 | `OnCinematicStarted`는 시퀀스 로드 완료 후 발화 | **정확** | `.cpp:232`, `HandleSequenceLoaded` 내부 `Play()` 직후. Ⅰ-(f) 참조 |
| 44 | 04 | 자동화 테스트 **10건**, 전부 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` | **정확** | 5개 파일 = 4(Cooldown) + 3(RewardPresentation) + 1 + 1 + 1 = **10**. 파일별 개수도 표와 일치 |
| 45 | 04 | 레이어 5종을 `BindWidgetOptional`로 덮어쓸 수 있음, `LastFPSPrimaryGameLayout.h:50-63` | **정확** | `:50-63`이 정확히 5개 `LayerStack_*` `BindWidgetOptional` 블록 |
| 46 | 04 | 레이어 태그가 문자열이 아닌 접근자 함수, `LastFPSUITags.h:8-12` | **정확** | `:8-12` = `Layer_Game()` … `Layer_Overlay()` 5개 |
| 47 | 04 | `ULastFPSScreenRegistry`가 `TMap<FGameplayTag, FLastFPSScreenDef>` 하나, `.h:21` | **정확** | `:20-21` `TMap<FGameplayTag, FLastFPSScreenDef> Screens` (`meta=(Categories="UI.Screen")`) |
| 48 | 04 | `OpenScreen`(`.h:34`) / `OpenScreenWithInit`(`:42-45`) / `RegisterScreenTabHost`(`:60-61`) / `PendingTabScreenTag`(`:75`, `:94-97`) | **정확** | 4개 인용 전부 해당 줄에 일치. **04의 줄번호 정확도는 03보다 확연히 높다** |
| 49 | 04 | `HasActiveMenuLikeWidget()`가 단일 판정 기준, `PrimaryGameLayout.cpp:190` | **정확** | `:190` 함수 정의 |
| 50 | 04 | 컷신 레이어 복원 `CinematicRestoreVisibilities`(`.h:37`, `:68-69`), 숨길 레이어는 태그 배열(`:46-47`) | **정확** | 세 인용 모두 일치. `SetLayersHiddenForCinematic` `:37`, `CinematicHiddenLayers` `:46-47`, `TMap<FGameplayTag, ESlateVisibility>` `:68-69` |
| 51 | 04 | `ULastFPSInputConfig` 3배열 구성, `.h:33-42` | **정확(주의)** | `:35-43`에 `NativeInputActions`/`AbilityInputActions`/`ReleaseCancelInputTags` 실재. 단 코드 주석은 어빌리티 액션을 "(Q / E / F / 사격 / 점프)"로 적어 **문서의 "Q / E / Z / F"와 Z 하나가 어긋난다**(코드 주석이 낡았을 가능성이 높으나 인용 근거로는 불일치) |
| 52 | 04 | 콘솔 명령 4개 표 | **부정확(불완전)** | 인용 줄번호 4개(`PopupDebug:201-202`, `QuestDebug:123-124`/`128-129`, `GasInspector:48-49`)는 **전부 정확**. 그러나 등록된 콘솔 명령은 **7개** — 누락 3개: `LastFPS.AddMarksForRadios`(`Cinematics/LastFPSCinematicPlaybackSubsystem.cpp:25`), `LastFPS.Travel`(`Game/LastFPSGameInstance.cpp:63`), `LastFPS.Encounter.Clear`(`Game/LastFPSPlayerController.cpp:172`). 뒤 둘은 **개발 편의 근거로 오히려 좋은 재료** |
| 53 | 04 | "UI/HUD 코드에 임시 화면 메시지가 남아 있지 않다" | **미검증** | 본 검수에서 `AddOnScreenDebugMessage` 전수 스윕 미수행. 문서가 단정형으로 쓸 근거를 제시하지 않았으므로 "grep 기준 0건" 같은 확인 방법을 함께 적을 것을 권장 |

---

## Ⅲ. 종합 진단

### 1) 03과 04의 신뢰도가 확연히 다르다

- **04의 줄번호 인용은 거의 전부 정확하다.** 샘플 12건 오류 0. 실제로 파일을 열고 쓴 문서다.
- **03의 줄번호 인용은 일부 파일에서 통째로 무너져 있다.** `LastFPSCharacterDefinition.h`(87줄인데 `:310` 인용), `LastFPSGameDataSet.h`(64줄인데 `:125-135`), `LastFPSActorPoolProfile.h`(54줄인데 `:192`), `LastFPSPoolableActor.h`(36줄인데 `:130-148`). **인용한 심볼과 서술 내용은 전부 맞는데 위치만 틀렸다** — 리팩터로 파일이 짧아진 뒤 줄번호를 갱신하지 않았거나 다른 버전을 보고 쓴 흔적이다.

문서 서두에 "모든 주장은 실제 소스 라인 근거를 함께 표기했다"고 선언했기 때문에, **면접관이 아무 줄이나 하나 열어 보고 빗나가면 문서 전체의 신뢰가 즉시 무너진다.** 이 문서의 가장 위험한 결함이다.

### 2) 내용 자체의 정확도는 높다

50여 개 주장 중 **동작 서술이 틀린 것은 사실상 없다.** 문제는 (i) 줄번호, (ii) 개수, (iii) "없음/0건" 단정 세 종류에 집중된다. 즉 **분석은 옳고 검증이 덜 됐다.**

### 3) 개수 주장은 전부 재확인 필요

| 문서 표기 | 실제 |
|---|---|
| Primary Asset Type 7종 → 표 9행 | 등록 7종, 표는 파생·부속 혼재 + `Map` 누락 |
| DataTable Row 15종, 전부 `Data/Tables/` | **17종**, 2종은 다른 폴더 |
| GameplayTag `Data.Table.*` 18개 | **17개** |
| 액터 풀 소비 지점 9개 | **11개** |
| HUD Presenter 9종 | **10종** |
| 콘솔 명령 4개 | **7개** |
| 자동화 테스트 10건 | 정확 ✅ |
| `LastFPSTags.h` 88개 | 정확 ✅ |
| CSV 17개 | 정확 ✅ |

**9건 중 6건이 어긋났다.** `grep -c` 한 줄이면 검증되는 숫자라, 틀렸을 때의 인상 손해가 특히 크다.

### 4) 문서가 스스로를 과소평가한 지점

역설적으로 **03이 "미검증"으로 넘긴 두 항목이 이 프로젝트의 최고 강점이었다.**

- `ULastFPSBattleDefinition` — 트래블·매치메이킹·파티·로비를 잇는 핵심 계약이며, "런타임 값은 이 에셋에 저장하지 않는다"는 클래스 주석이 문서 ③장 주제 그 자체다.
- Excel 파이프라인 — 전용 에디터 플러그인 + 파이썬 툴 3종 + 왕복 검증기. **"데이터 기반 설계"를 주장하는 문서에서 저작 도구 체인은 가장 설득력 있는 증거인데 통째로 빠졌다.**

---

## Ⅳ. 채용 담당자 관점 총평

**강점.** 이 두 문서는 잘한 것만 나열하는 포트폴리오 문서가 아니다. 03의 ⑥장은 `EMMWeaponType`·`ELastFPSItemType` enum 확장 필요성을 스스로 지적하고, `ULastFPSDropProfile`의 하드 분기를 인정하며, 04는 CommonUI 기본 경로를 끈 이유와 그 대가를 함께 적는다. **"인스턴스 추가는 데이터, 카테고리 추가는 코드"라는 경계 인식**과 **"변경 지점이 한 함수에 갇혀 있다"는 비용 서술**은 시니어 리뷰어가 듣고 싶어 하는 정확한 언어다. 04의 "폴링은 연속값에만", "포커스 복원을 다음 틱으로", "복제 수신 경로는 반드시 비동기", "쿠킹에서 BP는 `TSoftClassPtr`로" 같은 항목은 실제로 그 버그를 밟아 본 사람만 쓸 수 있는 내용이고, 코드 대조 결과 **전부 사실이었다.**

**약점.** 문서가 "모든 주장에 소스 라인을 붙였다"고 선언한 순간 **검증 가능성이 곧 리스크**가 된다. 03은 그 약속을 절반쯤 지키지 못한다. 면접에서 `LastFPSCharacterDefinition.h:310`을 열었는데 파일이 87줄이면, 뒤에 나오는 옳은 주장 40개까지 함께 의심받는다. 개수 오차도 같은 문제다 — 각각은 사소하지만 6/9가 틀리면 "세지 않고 쓴다"는 인상이 남는다.

---

## Ⅴ. 반드시 고쳐야 할 것 Top 3

**1. 03의 줄번호 전수 재검증 (최우선).**
최소한 `LastFPSCharacterDefinition.h`, `LastFPSGameDataSet.h`, `LastFPSActorPoolProfile.h`, `LastFPSPoolableActor.h`, `LastFPSGameDataSubsystem.h` 5개 파일의 인용은 **파일 길이를 넘는 명백한 오류**다. 갱신 유지가 부담이면 **줄번호를 빼고 `파일:심볼명` 형식으로 낮추는 편이 더 안전하다** — 심볼명은 리팩터에도 살아남고 검증하는 쪽이 grep 한 번으로 찾는다.

**2. 개수 주장 6건 정정.**
Row 타입 15→17(+ "전부 `Data/Tables/`" 문장 삭제), `Data.Table` 태그 18→17, 풀 소비 지점 9→11, Presenter 9→10(+EasyCrosshair 행), 콘솔 명령 4→7, Primary Asset Type 표의 "7종 vs 9행" 자기모순 해소.

**3. 03이 "미검증"으로 버린 두 강점을 되살리고, 과장 1건을 정정할 것.**
`ULastFPSBattleDefinition`을 Ⅰ-(a) 내용으로 채우고, ①장 mermaid에 **Excel 툴체인(`Tools/Excel/xlsx_to_csv.py` + EditorUtility 플러그인 + `verify_roundtrip.py`)을 명시**한다. 동시에 요약표의 "경로 문자열 하드코딩 없음"을 ini 잔존분(`ActiveTheme`, `PreviewStageClass`, `WaveEnemyMarkerWidgetClass`)을 인정하는 문장으로 교체한다 — 4-4장이 이미 그 위험을 정확히 지적해 놓았으므로, 인정하는 편이 오히려 일관되고 설득력 있다.
