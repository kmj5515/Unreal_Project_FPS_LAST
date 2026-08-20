# REVIEW — 01-combat-gas.md / 02-network-framework.md 사실 검증

> 검증 기준: 인용된 심볼의 실존 여부, `파일:줄번호`가 실제 그 내용을 가리키는지(±10줄 허용), 동작 서술이 코드와 일치하는지.
> 검증 범위: 두 문서에서 뽑은 검증 가능한 주장 40건. 코드·문서 수정 없음, 빌드 미실행.

---

## ① 판정 표

| # | 문서 | 주장 | 판정 | 근거 |
|---|---|---|---|---|
| 1 | 01 | `ULastFPSGameplayAbility` = `LastFPSGameplayAbility.h:26` | 정확 | `UCLASS(Abstract)` 26행, 클래스 선언 27행 |
| 2 | 01 | `ULastFPSActiveGameplayAbility` = `.h:11` | 정확 | 클래스 선언 10행 |
| 3 | 01 | `ULastFPSPassiveGameplayAbility`는 `ServerOnly` 고정 (`.cpp:7`) | 정확 | `NetExecutionPolicy = ServerOnly` 8행, 생성자 5행 |
| 4 | 01 | `ILastFPSConfirmableAbility` = `.h:14`, `UGA_IceStorm`이 구현 | 정확 | 인터페이스 14행. 구현체는 `GA_IceStorm.h:24` 하나뿐(전수 grep) |
| 5 | 01 | 스킬 행 조회는 `아바타 → CharacterDefinition → CharacterId + AbilityTags` (`cpp:33-50`) | 정확 | `FindSkillRow` 32-51행이 정확히 그 경로. 구체 스킬 이름 없음 |
| 6 | 01 | AttributeSet에 속성별 데미지 배율 5종 + 메타 `Damage` | 정확 | Physical/Fire/Ice/Electric/Poison(h:68-84) + `Damage`(h:93) |
| 7 | 01 | `RollAndApplySetByCallerDamage`가 유일한 데미지 계산 지점 | 정확 | 호출부 7곳(무기·투사체·ImpactRule·AreaEffect·ExpandingMesh·BossLaser·EnemyMelee) 전부 이 함수 경유. 우회 계산 없음 |
| 8 | 01 | `IsDamageGameplayEffect()` = `LastFPSDamageCalculation.cpp:101` | 정확 | 101행 |
| 9 | 01 | 시작 효과에 데미지 GE가 섞이면 경고 후 skip (`LastFPSCharacterBase.cpp:793`) | 정확 | 793행 `Skipping damage GameplayEffect in startup effects` |
| 10 | 01 | `bHasDied` 래치 (`LastFPSCharacterBase.cpp:411`) | 정확 | `HandleDeath` 411행, 래치 413-416행 |
| 11 | 01 | 쿨다운 스니펫 = `LastFPSActiveGameplayAbility.cpp:97-129` | 정확 | 97-130행. 인용 코드가 실제 코드와 일치(주석 일부 생략) |
| 12 | 01 | `GE_Cooldown.cpp:5` 주석 "여기 리터럴을 두면 …밸런스 수정이 삼켜진다" | 정확 | `AbilitySystem/Effects/GE_Cooldown.cpp:5-6` 원문 일치 |
| 13 | 01 | 클라 예측 차감 `TryConsumePredictedRound` (`WeaponComponent.cpp:134`) / 서버 `TryConsumeServerFirePermission` (`:1560`) | 정확 | 134행 / 1559행 |
| 14 | 01 | 서버 검증 스니펫 `:1356-1370`, `:1539-1558` | 정확 | 1356-1369, 1539-1557행. "거절이 아닌 대체" 서술도 코드와 일치 |
| 15 | 01 | 히트 판정은 `LineTraceSingleByObjectType` 단독 | 정확 | WeaponComponent 내 유일한 트레이스(1423행) |
| 16 | 01 | 서버 디버그 드로잉은 RPC 인자가 아니라 서버 설정으로 판단 (`WeaponComponent.h:273`) | 정확 | 273-275행 주석 + `Server_FireFromClientAim` 시그니처에 디버그 플래그 없음 |
| 17 | 01 | 데디 서버는 발사 연출 자체 재생 skip | 정확 | `Multicast_PlayFireEffects_Implementation` 671행 `NM_DedicatedServer` early return |
| 18 | 01 | 투사체 스폰은 노티파이 + 조준 AND 게이트 (`GA_Projectile.cpp:318`) | 정확 | 318행 `TrySpawnProjectile`, `bProjectileSpawnEventReceived && bHasCachedAimTarget` |
| 19 | 01 | 몽타주는 전부 `ASC->PlayMontage` (`GA_BasicShoot.cpp:266`, `GA_Reload.cpp:68`) | 정확 | 266-267행 / 67-68행 주석·구현 일치 |
| 20 | 01 | `Multicast_ApplyVisualData` 사용 이유 (`LastFPSProjectile.h:117-121`) | 정확 | 117-121행 주석이 문서 서술과 동일 |
| 21 | 01 | 광역 판정 필드 `NotReplicated` (`LastFPSAreaEffectActor.h:43-68`) | 정확 | 43-48행 주석 + 49-68행 전 필드 `NotReplicated` |
| 22 | 01 | 입력 우선순위 취소→확정→활성화 (`LastFPSHero.cpp:1027-1050`) | 정확 | `InputPressed` 1027행, 1029/1034행 순서 일치 |
| 23 | 01 | 플레이어 ASC `Mixed`(`LastFPSPlayerState.cpp:19`) / 그 외 `Minimal`(`LastFPSCharacterBase.cpp:33`) | 정확 | 19행 / 33행 |
| 24 | 01 | ImpactRule 공통 헬퍼 3개 (`LastFPSProjectileImpactRule.cpp:15-80`) | 정확 | `GetAbilitySystemComponent`(9), `DoesTargetPassTags`(15), `ApplyGameplayEffectsToTarget`(41). 3개 맞음 |
| 25 | 01 | 다이어그램: `UGA_Boss*` 5종, Viola 3종, Enemy 2종이 `ULastFPSGameplayAbility` 직계 | 정확 | Boss 5개 파일 존재, `UGA_EnemyShoot/EnemyMelee/BossLaser` 모두 `: public ULastFPSGameplayAbility`, `UGA_ViolaIceTrail : public UGA_ViolaIceAura` |
| 26 | 01 | §3.1 시퀀스 순서: 검증(`ValidateClientMuzzleLocation`) → `Multicast_PlayFireEffects` | **부정확** | 실제는 반대. 1362행 멀티캐스트가 1365-1369행 위치 검증보다 **먼저** 실행된다 |
| 27 | 01 | 다이어그램: `ALastFPSEnemyCharacter -- 소유 --> ULastFPSWeakpointComponent` (전투 파이프라인 구성원) | **과장** | 컴포넌트는 실재하나 반환한 약점 배수가 호출부에서 버려져 데미지에 반영되지 않음(§②-(b)). 전투 코어 다이어그램에 실효 없는 노드 |
| 28 | 02 | 마스터 로비 설계 근거 = `LastFPSMasterLobbyPC.h:14-16` | 정확 | 14-16행 주석 원문 일치 |
| 29 | 02 | Beacon 클라 = `LastFPSMasterLobbyBeaconClient.h:7-15` | 정확 | 7-15행 클래스 주석 |
| 30 | 02 | `ShouldCreateSubsystem()`이 `IsRunningDedicatedServer()`로 차단 (`cpp:18-26`) | 정확 | 18-26행 |
| 31 | 02 | `COND_OwnerOnly`: WeaponSlots/Magazine/Reserve (`WeaponComponent.cpp:56-59`) | 정확 | 56,57,59행 + 58행 주석이 문서 인용문과 동일 |
| 32 | 02 | `COND_SkipOwner`: `CombatState` + 러버밴딩 근거 (`LastFPSHero.cpp:100`) | 정확 | 100행. 95-99행 주석이 문서 서술과 동일 |
| 33 | 02 | `REPNOTIFY_Always`가 **전체** GAS 어트리뷰트 (`AttributeSet.cpp:98-112`) | **과장** | 98-112행 맞지만 메타 어트리뷰트 `Damage`는 복제 목록에 없음. "복제되는 전체 어트리뷰트"가 정확 |
| 34 | 02 | RPC 종류 표 5종(`Client_NotifyDamageDealt` Unreliable 등) | 정확 | `PlayerState.h:128-143`, `PlayerController.h:164-165`, `RoomSpawnPresentationComponent.h:33`, `StreamingLevelTransitionRuntime.h:61` 전부 문서 표기와 일치 |
| 35 | 02 | `Server_ReportPartyQuestProgress_Validate` 스니펫 (`PlayerController.cpp:88-106`) | 정확 | 88-106행, 인용 코드 원문 일치 |
| 36 | 02 | `MergeProgress` 계약 — 되돌림 금지·`int64` 누적 (`GameStateBase.h:40-54`) | 정확 | 40-54행 구현 그대로 |
| 37 | 02 | 장비 제출 상한 64 + `IsSubmittedSlotWellFormed` + `ponytail:` 한계 주석 (`PlayerState.cpp:131-171`) | 정확 | 131-171행 전부 확인 |
| 38 | 02 | `RegisterOrUpdateRoom` 클램프·`MaxActiveRooms`·`Config=Game` (`MasterLobbyGameMode.cpp:99-138`) | 정확 | 99-138행, 제한값 전부 `UPROPERTY(Config)` |
| 39 | 02 | 하트비트는 인원 변화 시에만 브로드캐스트 (`GameMode.cpp:150-155`) | 정확 | 150-155행 |
| 40 | 02 | 인카운터 `HasAuthority()` 게이트 10개 줄번호 (214/230/490/745/773/794/873/1147/1300/1362) | 정확 | 10개 전부 해당 줄에 `if (!HasAuthority() ...)` 존재 |
| 41 | 02 | 치트는 `#if UE_BUILD_SHIPPING`으로 본체 제외 (`PlayerController.cpp:1252-1298`) | 정확 | 1252행 함수, 1254행 `#if UE_BUILD_SHIPPING` … `#else` |
| 42 | 02 | 빌드 인자 + Installed Engine 분기 (`Build-MasterLobbyServer.ps1:25-63`) | 정확 | 25-44 공통 인자, 45-63 분기. 문서에 옮긴 인자 목록 전부 일치 |
| 43 | 02 | 배포 시 `Logs/`,`Runtime/`,`Server.config.psd1` 보존 (`ps1:77-90`) | 정확 | `$preservedNames`/`$preservedDirectories` 79-90행 |
| 44 | 02 | 기동 인자 (`Start-Server.ps1:11-15`) | 정확 | 10-12행에서 문서와 동일한 인자 조립 |
| 45 | 02 | 감시 루프 코드 (`Run-Supervisor.ps1`) | 정확 | 인용 의사코드가 4-22행 실제 루프와 일치 |
| 46 | 02 | 예약 작업 `SYSTEM`/`RunLevel Highest`/`ExecutionTimeLimit Zero` (`Install-StartupTask.ps1:4-8`) | 정확 | 6-7행 |
| 47 | 02 | 버전 문자열 `yyyyMMdd-HHmmss\|<바이트수>` + `Runtime/installed-build.txt` 비교 | 정확 | `Publish-ServerBuild.ps1:56-58`, `Update-ServerBuild.ps1:24,31,48` |
| 48 | 02 | 포트 3중 일치표(7777/15000) | 정확 | `MasterLobbySettings.h:29,36,51`, `MasterLobbyGameMode.h:95,99`, `Server.config.psd1 GamePort/BeaconPort` |
| 49 | 02 | `FMasterLobbyRoomInfo::HostAddress` 주석 = `MasterLobbyTypes.h:159-164` | **부정확** | 파일 전체가 **41줄**. 해당 주석은 27-32행. 인용문 자체는 원문과 일치 → 줄번호만 오류 |
| 50 | 02 | `CreateLobbyWidgetForLocalPlayer`의 `FApp::CanEverRender()` = `MasterLobbyPC.cpp:209-213` | **부정확** | 파일 전체가 **107줄**. 실제 `CanEverRender()`는 43행 |
| 51 | 02 | 정상 종료 시 즉시 제거 = `MasterLobbyBeaconHostObject.cpp:154-166` | **부정확** | 파일 전체가 **28줄**. `NotifyClientDisconnected`는 15-27행 |
| 52 | 02 | `-MasterServer=` 오버라이드 = `MasterLobbySettings.h:180-216` | **부정확** | 파일 전체가 **73줄**. `ServerAddress` 25행, `CommandLineOverrideKey` 43행 |
| 53 | 02 | 45초 TTL이 `Server.config.psd1`에 대응 (`RoomTimeoutSeconds=45`) | **부정확** | `Server.config.psd1`에 `RoomTimeoutSeconds` 없음. 실제 출처는 `LastFPS/Config/DefaultGame.ini:154` `[/Script/LastFPS.LastFPSMasterLobbyGameMode] RoomTimeoutSeconds=45.0` (C++ 기본값 `MasterLobbyGameMode.h:84`) |
| 54 | 02 | 운영 패키지 경로 = `Deploy_Server/Server-PC-Package/` | **부정확** | 그쪽은 빌드가 만드는 **배포 사본**이며 `.gitignore:91`로 저장소에서 제외됨. 원본은 `Tools/ServerAutomation/Server-PC-Package/` |
| 55 | 02 | 매치 시작 조건 `IsPartyHost && AreAllPlayersReady && MinPlayersToStart` (`PartyGameMode.cpp:88-116`, `:171-172`) | 정확 | 88-116행, 방장 예외 171-172행, `MinPlayersToStart` 검사 179행(`AreAllPlayersReady` 내부) |
| 56 | 02 | 시퀀스 ⑥ "결과 / 실패" | **부정확** | 실패 경로만 그려져 있고 성공 결과 화면 경로가 없음(§②-(e)) |

**정확 44 / 부정확 9 / 과장 2 / 미검증 1**(아래 (f) 관련 문서 서술 부재 항목).

### 수정 문구 제안

- **#26** — "① 위치·조준 검증 → ② 멀티캐스트 연출" 순서를 뒤집을 것. 실제는 발사 허가 직후 `Multicast_PlayFireEffects()`가 나가고, 그 뒤 총구·시점 좌표 검증이 이뤄진다. (연출 전파와 탄착 계산이 독립이라는 점을 근거로 쓰면 오히려 설계 의도로 읽힌다.)
- **#27** — 다이어그램에서 `WeakpointComponent`를 빼거나 "발광·파괴 연출 전용(데미지 배수는 현재 미반영)"으로 캡션을 달 것.
- **#33** — "전체 GAS 어트리뷰트" → "복제되는 전체 어트리뷰트(메타 `Damage` 제외)".
- **#49~52** — 줄번호를 각각 `LastFPSMasterLobbyTypes.h:27-32`, `LastFPSMasterLobbyPC.cpp:43`, `LastFPSMasterLobbyBeaconHostObject.cpp:15-27`, `LastFPSMasterLobbySettings.h:25,43`로 교체.
- **#53** — "(`BeaconClient.cpp:11`, `Server.config.psd1` 대응 `RoomTimeoutSeconds=45`)" → "(하트비트 10초는 `BeaconClient.cpp:11`, TTL 45초는 `DefaultGame.ini` `[/Script/LastFPS.LastFPSMasterLobbyGameMode] RoomTimeoutSeconds=45.0`)".
- **#54** — 문서 머리말과 §6-4의 경로를 `Tools/ServerAutomation/Server-PC-Package/`(원본, 저장소 포함)로 바꾸고, `Deploy_Server/`는 "빌드 산출물 + 스크립트 사본이 놓이는 배포 폴더(gitignore)"로 한 줄 설명할 것. 지금 표기는 저장소를 받은 사람이 열 수 없는 경로다.
- **#56** — 시퀀스 제목을 "⑥ 실패 → 귀환"으로 좁히고, 성공 결과 화면은 별도 항목으로 추가(§②-(e) 참조).

---

## ② 작성자 신고 항목 6건 — 결론

### (a) `LastFPSProjectile`는 VFX 전용인가 → **아니다. 주석이 낡았다 (코드 쪽 문제)**

`Projectiles/LastFPSProjectile.h:25`의 `// VFX 전용 투사체 — 데미지는 GA_BasicShoot의 LineTrace가 처리`는 사실이 아니다.
같은 클래스가 `InitializeGameplayProjectile(...ImpactRules, LegacyEffectsOnHit, BaseDamageOverride...)`(h:36-43),
`ExecuteImpactRules`(h:93), `ApplyEffectToTarget`(h:94), `ImpactRules` 배열(h:135)을 갖고,
`LastFPSProjectile.cpp:324`에서 직접 `RollAndApplySetByCallerDamage`를 호출한다.
→ **게임플레이 투사체 겸 VFX 투사체**가 맞고, 문서 §3.2의 서술이 코드와 일치한다. 고쳐야 할 것은 **문서가 아니라 h:25 주석**이다.
(문서에 남길 값어치가 있는 사실이기도 하다: 히트스캔 사격은 트레이스, 스킬 투사체는 ImpactRule로 데미지가 갈린다.)

### (b) 약점 배수는 실제 데미지에 반영되는가 → **반영되지 않는다 (실버그)**

`WeaponComponent.cpp` 1492-1509행 순서가 결정적이다.

```cpp
LastFPSDamage::RollAndApplySetByCallerDamage(*Spec.Data.Get(), DamageRange); // 1495
TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());                  // 1496  ← 데미지 확정
...
Weakpoint->HandleHitOnBone(HitResult.BoneName, GetWeaponBaseDamage());       // 1508  ← 반환값 버림
```

- 데미지 GE는 **1496행에서 이미 적용**되고, 약점 호출은 그 **뒤** 1508행이다. 순서상 배수를 곱할 여지가 없다.
- `HandleHitOnBone`은 `DamageMultiplier`(약점 히트 시) 또는 `1.f`를 반환하는데(`LastFPSWeakpointComponent.cpp:68-86`), 유일한 호출부가 반환값을 버린다. 코드베이스 전체에서 다른 호출부 없음.
- 실제로 하는 일은 약점 체력 차감·발광·`Multicast_PlayHitFlash`뿐이다. 즉 **약점은 연출/파괴 단계 전용**이고 헤드샷 배수는 존재하지 않는다.
- 참고(문서 밖 이슈): 1500-1505행과 `LastFPSWeakpointComponent.cpp:70-72`에 `UE_LOG(LogTemp, Warning, ...)` 진단 로그가 **모든 피격마다** 남아 있다. 주석에도 "문제 해결 후 제거"라고 적혀 있다. 채용 담당자가 코드를 열면 바로 보이는 지점이다.

문서(01)는 약점 배수를 주장하지 않으므로 **문서의 거짓말은 아니지만**, §2.3 다이어그램이 이 컴포넌트를 전투 코어 구성원으로 보여주는 것은 과장이다(#27).

### (c) `IsAlive()`의 캐시 AttributeSet과 `GetAbilitySystemComponent()`의 PlayerState 우선 해석이 어긋나는가 → **해석 규칙은 일치. 다만 초기화 전 창(window)이 있다**

- `GetAbilitySystemComponent()`(`cpp:50-56`): PS 있으면 PS의 ASC, 없으면 `OwnedAbilitySystemComponent`.
- `InitAbilitySystem()`(`cpp:567-578`): `AttributeSet = PS ? PS->GetAttributeSet() : OwnedAttributeSet`.
→ **같은 PS 우선 규칙**이므로 "PS ASC인데 자기 AttributeSet을 본다" 같은 불일치는 발생하지 않는다.

남는 위험은 시점 하나다. `AttributeSet` 멤버는 생성자에서 초기화되지 않고 `InitAbilitySystem()`(서버 `PossessedBy` / 클라 `OnRep_PlayerState` / `BeginPlay` 경유)에서만 채워진다.
그 이전에 `IsAlive()`를 부르면 `AttributeSet == nullptr` → **`false`(=사망)로 읽힌다**. 클라이언트에서 PlayerState 복제가 Pawn보다 늦게 도착하는 창에서 UI/AI가 이 값을 물으면 "살아 있는데 죽은 것으로" 판정될 수 있다.
문서 §5.1의 "`InitAbilitySystem()`이 두 경로를 하나로 흡수한다"는 **정확**하다. 다만 "초기화 이전에는 `IsAlive()`가 false를 반환한다"는 계약은 문서에도 코드 주석에도 없다. 한 줄 보강을 권한다.

### (d) `Deploy_Server/` vs `Tools/ServerAutomation/Server-PC-Package/` → **Tools 쪽이 원본, Deploy_Server 쪽이 배포 사본**

`Build-MasterLobbyServer.ps1:74-90`이 결론이다.

```powershell
$automationSource      = (Resolve-Path (Join-Path $PSScriptRoot '..\Server-PC-Package')).Path  # Tools/ServerAutomation/…
$automationDestination = Join-Path $ArchiveDirectory $AutomationDirectoryName                  # Deploy_Server/Server-PC-Package
```

- 원본: `Tools/ServerAutomation/Server-PC-Package/` — git 추적 대상.
- 사본: `Deploy_Server/Server-PC-Package/` — `.gitignore:91`의 `/Deploy_Server/`로 **저장소에서 제외**. 실제로 사본에만 `Logs/`, `Runtime/`이 추가로 존재하며 빌드 스크립트가 `Server.config.psd1`·`Logs/`·`Runtime/`을 덮어쓰지 않고 보존한다.
→ 문서 §① 표와 §6-4 제목이 배포 사본 경로를 가리키고 있어 **저장소를 클론한 독자는 그 경로를 열 수 없다**. #54의 수정 문구대로 원본 경로로 바꿔야 한다. (§6-2 "배포 폴더로 복사" 서술 자체는 정확.)

### (e) 매치 **성공** 결과 화면 흐름 → **클라이언트 로컬 트리거. 문서에 아예 빠져 있다**

`Quest/LastFPSQuestSubsystem.cpp:2604`의 호출부는 `NotifyRewardGranted()`(2570행)이며, 조건은 다음과 같다.

1. 진입점은 퀘스트 **보상 지급**(`cpp:2449`)이지 전투 종료 이벤트가 아니다.
2. `ULastFPSQuestSubsystem`은 GameInstance 서브시스템이고, PC를 `GI->GetFirstLocalPlayerController()`로 얻는다 → **각 머신의 로컬 클라이언트에서 실행**된다. 서버 권한 게이트도, `Client_*` RPC도 아니다.
3. 결과 화면은 `IsQuestMappedToAnyMap(QuestId)`가 참일 때만, 즉 **던전 맵에 매핑된 퀘스트에 한해** 뜬다. 그 외 퀘스트는 기존 공지 팝업.
4. 경과 시간은 `MissionStartRealTimeSeconds`(`cpp:2783`)로 계산한 **로컬 실시간**이다.

문서 02는 실패 경로(`HandleMissionFailed` → `OpenScreen` → `ClientReturnToHub`, 서버 권한 + `bMissionFailedHandled` 래치)만 서술하고, 성공 경로는 다이어그램 제목만 "결과 / 실패"로 걸어 두었다. **성공/실패가 서로 다른 계층(퀘스트 서브시스템 로컬 vs GameMode 서버 권한)에서 트리거된다는 점**은 오히려 설명 값어치가 큰 비대칭이므로, ⑥에 2~3줄 추가할 것을 권한다.

### (f) Party PC의 0.5초 타이머 + `AddToViewport(9999)` → **실재한다 (문서에는 없음)**

`Network/Party/LastFPSPartyPlayerController.cpp:26-69`. 요약하면:

- `ReceivedPlayer()`에서 0.5초 `SetTimer`(`cpp:67`)로 UI 푸시를 지연.
- 위젯 클래스를 `FSoftClassPath(TEXT("/Game/UI/FrontEnd/WBP_PartyPanel..."))` **하드코딩 경로**로 잡고 `LoadSynchronous()`.
- `PushWidgetToLayerStack(...)`으로 정상 푸시한 **직후**, `// Force add to viewport in case Layer_Menu is hidden` 주석과 함께 **같은 위젯을 하나 더 `CreateWidget` 해서 `AddToViewport(9999)`** (cpp:46-49). 즉 위젯이 두 개 생성된다.
- `UE_LOG(LogTemp, Warning, ...)` 4개가 정상 흐름에 남아 있다.

문서 02는 이 회피책을 언급하지 않는다. 문서가 그린 "규칙은 GameMode, 값은 GameState, UI는 façade" 구조와 정면으로 어긋나는 코드이며, 프로젝트 `CLAUDE.md`의 데이터 기반 설계·임시 디버그 코드 금지 항목도 위반한다. **문서에 쓰라는 뜻이 아니라, 제출 전에 코드를 정리하는 편이 낫다**는 항목이다.

---

## ③ 총평 — 채용 담당자 관점

**그대로 제출해도 되는가: 조건부 예. 02는 반나절 수정 후 제출, 01은 두 줄 수정으로 제출 가능.**

두 문서의 검증 통과율은 높다(44/56 정확). 특히 인상적인 부분:

- 인카운터 `HasAuthority()` 게이트 **10개 줄번호가 전부 정확**하고, `COND_SkipOwner`·`COND_OwnerOnly`·`REPNOTIFY_Always` 선택 근거가 코드 주석과 한 글자씩 대조 가능하다. 문서를 위해 지어낸 서사가 아니라 **코드에 이미 남아 있던 판단을 옮겼다**는 증거다.
- "데미지 계산 지점 하나"는 호출부 7곳 전수 확인으로 사실이었다. 이런 종류의 주장은 대개 반례가 나오는데, 나오지 않았다.
- 빌드/배포 스크립트 인용은 인자 단위까지 일치했다.

반면 신뢰를 깎는 지점도 명확하다. **02의 줄번호 4건이 존재하지 않는 줄을 가리킨다**(41줄 파일에 159행, 28줄 파일에 154행 등). 면접관이 무작위로 하나만 열어봐도 걸리며, 걸리는 순간 나머지 정확한 44건까지 의심받는다. 이건 내용 문제가 아니라 **검수 부재의 신호**로 읽힌다.

### 반드시 고쳐야 할 것 Top 3

1. **02의 죽은 줄번호 5건 정정** (#49~53). 파일 길이보다 큰 줄번호 4건 + TTL 값 출처 오귀속 1건. 가장 싸고 가장 치명적인 수정.
2. **`Deploy_Server/` 경로 표기 교체** (#54). 문서가 안내하는 운영 패키지 경로가 저장소에 없다(gitignore). 원본은 `Tools/ServerAutomation/Server-PC-Package/`. "받아서 열어봤는데 없다"는 최악의 첫인상이다.
3. **약점 컴포넌트 처리** (#27 / §(b)). 문서에서 빼든 코드를 고치든 하나는 해야 한다. 지금은 `HandleHitOnBone`의 반환값이 버려지고, 데미지 GE가 그보다 먼저 적용되며, 피격마다 `LogTemp Warning` 진단 로그가 나간다. 문서가 "전투 코어"를 자랑하는 챕터인 만큼 여기가 열릴 확률이 높다.

추가 권고(제출 차단 사유는 아님): §3.1 시퀀스의 멀티캐스트/검증 순서 정정(#26), 성공 결과 화면 흐름 2~3줄 추가(§(e)), `LastFPSProjectile.h:25`의 낡은 "VFX 전용" 주석 삭제(§(a)), 그리고 제출 전 `LastFPSPartyPlayerController`의 0.5초 타이머 + 이중 위젯 생성 정리(§(f)).
