# LastFPS — 포트폴리오 멀티플레이어 TPS/FPS 프로젝트

> Unreal Engine 5.7 + GAS(Gameplay Ability System) 기반  
> Time Takers에서 영감을 받은 **개인전(FFA)** 경쟁 슈팅 게임 (동시 입장 최대 3명)

---

## 목차

1. [게임 개요](#게임-개요)
2. [핵심 기능 목록](#핵심-기능-목록)
3. [기술 스택](#기술-스택)
4. [아키텍처 설계](#아키텍처-설계) — GAS, 피해 파이프라인, 매치 통계, 사망 애니
5. [개발 로드맵](#개발-로드맵)
6. [폴더 구조 계획](#폴더-구조-계획)
7. [빌드 및 실행](#빌드-및-실행)

---

## 게임 개요

**장르:** 3인칭 기반 멀티플레이어 개인전(FFA) 슈팅  
**플랫폼:** PC (Windows)  
**네트워크:** 멀티플레이어 전용 (Dedicated Server 목표)  
**동시 입장:** 최대 **3명** (팀 없음, 모두 적대)

### 컨셉

Time Takers의 빠른 템포와 스킬 기반 전투를 참고하여, 각 플레이어가 고유한 스킬셋으로 **다른 모든 참가자와 개인전**을 벌이는 경쟁 슈팅 게임.

단순 DPS 경쟁이 아닌 **캐릭터별 스킬 운용과 판단**이 승부를 가르는 것을 목표로 한다. (팀 협력·팀 점수 없음)

---

## 핵심 기능 목록

### 캐릭터 & 이동
| 기능 | 설명 | 입력 |
|------|------|------|
| 이동 | WASD 전방향 이동 | W / A / S / D |
| 달리기 | 좌 Shift 홀드 시 스프린트, 스태미나 소모 | Left Shift |
| 점프 | 기본 점프 | Space |
| 2단 점프 | 공중에서 한 번 더 점프 가능 | Space (공중) |
| 조준 | 우클릭 ADS(Aim Down Sights) or 3인칭 어깨 시점 전환 | RMB |
| 사격 | 단발 / 연사 모드 | LMB |
| 오버히트 | 연사 시 열 게이지 누적 → 최대치 도달 시 발사 불가, 완전 냉각 후 재개 | — |

### 어빌리티 시스템 (GAS)
| 슬롯 | 키 | 설명 |
|------|-----|------|
| 스킬 1 | Q | 캐릭터별 고유 전술 스킬 (쿨다운 8~12초) |
| 스킬 2 | E | 캐릭터별 보조/이동/유틸리티 스킬 (쿨다운 15~20초) |
| 궁극기 | F | 강력한 한 방, 쿨다운 길거나 게이지 소모 (쿨다운 60~90초 or 게이지) |

### 어트리뷰트 (GAS AttributeSet)
- **Health** — 최대 체력 / 현재 체력
- **Damage** — 메타 어트리뷰트: GE가 여기에 가산한 뒤 `PostGameplayEffectExecute`에서 **Health**를 깎는 방식으로 소비됨
- **Stamina** — 달리기·특수 이동 소모 자원
- **UltimateGauge** — 궁극기 차징 게이지 (전투 참여 시 누적)
- **AttackDamage** — 기본 공격력 (무기 별 계수)
- **Defense** — 피해 감소 수치
- **MoveSpeed** — 기본 이동속도 배율

### 게임 모드
- **데스매치 (FFA):** 동시 입장 최대 3명, 제한 시간 내 **개인 킬 수** 등으로 순위 결정 (승자 1명 또는 상위 랭킹)
- (추후 확장) 다른 규칙·맵 변형 등은 규모에 맞게 검토

### 세션 구성
- **로비 → 매치:** 최대 3명이 모이면 시작하는 흐름 유지 (코드·문서의 “3명 입장” 조건과 일치)
- 팀 없음 → 아군 구분·FF 처리 불필요
- 집계는 **개인 킬/데스·딜 등** 중심 (팀 점수 없음)

### 게임 흐름 연출

#### 1. 매치 인트로 시퀀스 (Match Start)
매치 시작 직전 **참가자 소개 화면** 표시 (최대 3명, 팀 개념 없음):
- 이번 매치에 입장한 플레이어(최대 3명)와 캐릭터가 포즈를 취하며 등장
- 닉네임 / 캐릭터 / 구분용 컬러 등 표시 (팀명·팀 컬러 없음)
- Sequencer 또는 UMG 애니메이션으로 연출 (2~3초)
- 이후 **낙하 인트로**로 자연스럽게 전환

#### 2. 낙하 & 착지 인트로 (Drop Intro)
매치 인트로 후 전투 개시 전 **하늘에서 낙하하는 연출**:
- 모든 플레이어가 공중 고고도 지점에서 스폰
- 낙하 중 캐릭터 낙하 애니메이션 재생 (팔다리 펼침 → 수직 다이브)
- 맵 특정 착지 포인트(스폰존)로 낙하
- 착지 시 **임팩트 이펙트 + 카메라 셰이크** 연출
- 착지 완료 후 전투 입력 활성화 (착지 전 이동 입력 차단)
- 구현 방식: `GA_DropIntro` GameplayAbility로 관리, 착지 판정은 `OnLanded` 오버라이드

#### 3. MVP 결과 화면 (Match End)
게임 종료 후 **MVP / 우승자 하이라이트**:
- **1위(우승)** 기준: 최다 킬 또는 종합 점수 규칙에 따른 1명 선정
- 우승 캐릭터가 화면 중앙에서 **Victory 포즈 애니메이션** 재생
- 나머지 참가자(2명 이하)는 스코어보드·부차 연출로 표시 (팀 단위 연출 없음)
- 개인 스탯 (킬/데스/어시스트/딜량) 스코어보드 오버레이
- 구현 방식: 전용 MVP 레벨 또는 GameState → Level Sequence 트리거

---

## 기술 스택

| 분야 | 사용 기술 |
|------|-----------|
| 엔진 | Unreal Engine 5.7 |
| 언어 | C++ (코어) + Blueprint (폴리싱·이펙트) |
| 어빌리티 | Gameplay Ability System (GAS) |
| 입력 | Enhanced Input System |
| 네트워크 | UE Replication + GameplayAbility Prediction |
| 애니메이션 | Linked Animation Layers + Motion Warping |
| UI | UMG + CommonUI |
| 물리 | Chaos Physics |
| 렌더링 | Lumen + Nanite |

---

## 아키텍처 설계

### GAS 구성도
```
ALastFPSPlayerState
  └── UAbilitySystemComponent (ASC)
        ├── ULastFPSAttributeSet         // Health, Stamina, UltGauge...
        ├── GameplayAbilities
        │     ├── GA_BasicShoot          // 기본 사격
        │     ├── GA_Sprint              // 달리기
        │     ├── GA_Jump / GA_DoubleJump
        │     ├── GA_SkillMoveBoost (Q)  // 3초 이동속도 증가
        │     ├── GA_SkillHeal (E)       // 즉시 체력 회복
        │     └── GA_Ultimate (F)        // 태그·입력 준비, GA 미구현
        ├── GameplayEffects
        │     ├── ULastFPSGE_DamageInstant  // 네이티브 즉시 피해 (Damage 메타 +Additive, 기본 15)
        │     ├── (선택) BP 피해 GE — GA_BasicShoot `DamageEffectClass`로 지정, **Damage** 메타에 Additive 양수 권장
        │     ├── ULastFPSGE_HealInstant       // E 스킬: 즉시 체력 회복
        │     ├── ULastFPSGE_MoveSpeedBuff    // Q 스킬: 3초간 이동속도 증가 (Duration)
        │     ├── GE_SprintSpeed              // 달리기: MoveSpeed +300 (Infinite, GA_Sprint 내부)
        │     ├── GE_SprintStaminaDrain       // 달리기: Stamina -20/s (Periodic, GA_Sprint 내부)
        │     ├── GE_StaminaRegen             // Stamina +5/s (Passive, always-on) — 미구현
        │     └── GE_UltGaugeCharge           // 궁극기 게이지 충전 — 미구현
        └── GameplayCues                 // 이펙트·사운드 트리거
```

**사망 애니:** `LastFPSAnimInstance`에서 `bIsDead` → AnimBP 레이어 전환.

- 이 프로젝트의 피해 처리는 UE 기본 `ApplyDamage`가 아니라 **GameplayEffect 기반**으로 통일되어 있다.
- 발사 시 **Hitscan(LineTrace) 방식**으로 서버에서 즉시 히트 판정 후 대상 ASC에 피해 GE를 적용한다.
- `ALastFPSProjectile`은 VFX 전용(충돌 없음, 탄도 트레일 표시)이며 데미지 역할을 하지 않는다.
- 피해 적용 결과(체력 감소, 피격 반응, 매치 통계 갱신)는 AttributeSet/PlayerState 흐름에서 처리된다.

### 매치 통계 (`ALastFPSPlayerState`)

- 매치 통계(킬/데스/딜·힐)는 서버에서 누적하고 클라이언트로 복제한다.
- `PlayerState` 클래스가 `ALastFPSPlayerState`로 설정되어 있어야 통계/GAS 연동이 정상 동작한다.

### 애니메이션 — 사망

- 사망 판정은 캐릭터 생존 상태를 기준으로 `bIsDead`에 반영된다.
- 실제 사망 포즈/레이어 전환은 AnimBP에서 `bIsDead`를 사용해 처리한다.

### 네트워크 구조
```

### 로비 캐릭터 선택 → 매치 스폰

- 로비 UI(`ULastFPSLobbyWidget`)에서 `Button_C1/C2/C3` 클릭 시 `SelectCharacterByIndex(0/1/2)` 호출
- 선택 인덱스는 서버 `ALastFPSPlayerController`에서 처리되어 `ALastFPSPlayerState`와 `ULastFPSGameInstance`에 저장
- 매치 스폰 시 `ALastFPSGameModeBase::GetDefaultPawnClassForController`가  
  `PlayerState` 인덱스를 우선 사용하고, 필요 시 `GameInstance` 저장값으로 보정
- `CharacterPawnClasses` 배열 인덱스(0-based)와 UI 버튼 인덱스가 동일해야 원하는 캐릭터가 스폰됨
- 로비 전용 폰은 `ALastFPSLobbyGameMode::LobbyPawnClass`로 분리하여 전투 입력/사격을 차단
Dedicated Server
  ├── ALastFPSGameMode        // 규칙 관리 (서버 전용)
  ├── ALastFPSGameState        // 매치 타이머·(필요 시) 개인/매치 상태 (모든 클라 복제)
  ├── ALastFPSPlayerState × N  // 개인 스탯, ASC 보유
  └── ALastFPSCharacter × N    // 폰, 이동·애니 복제
        └── ALastFPSPlayerController × N  // 입력, HUD
```

### 캐릭터 클래스 계층
```
ACharacter
  └── ALastFPSCharacterBase      // GAS 인터페이스, AttributeSet
        ├── ALastFPSHero         // 플레이어 캐릭터 (3인칭)
        │     └── (캐릭터별 파생 클래스)
        └── ALastFPSAICharacter  // (추후) AI 봇
```

---

## 개발 로드맵

### Phase 0 — 프로젝트 세팅 (현재)
- [x] UE 5.7 C++ 프로젝트 생성
- [x] Git 저장소 초기화
- [x] GAS 모듈 의존성 추가 (`GameplayAbilities`, `GameplayTags`, `GameplayTasks`)
- [x] Enhanced Input 입력 매핑 기초 설정
- [x] 기본 폴더 구조 정립

### Phase 1 — 캐릭터 & 기본 이동 (목표: ~2주)
- [x] `ALastFPSCharacterBase` C++ 클래스 구현
- [x] 3인칭 카메라 + 스프링 암 설정 (ADS 보간 포함)
- [x] Enhanced Input으로 이동 / 카메라 회전 연결
- [x] GAS AttributeSet 기초 (Health, Stamina, MoveSpeed)
- [x] `GA_Jump` + `GA_DoubleJump` 구현 (Gameplay Ability)
- [x] `GA_Sprint` 스태미나 소모 구현
- [x] 기본 이동 애니메이션 블렌드스페이스 연결 (Speed / Direction BlendSpace)

### Phase 2 — 전투 시스템 기초 (목표: ~3주)
- [x] 무기 컴포넌트 설계 (`UWeaponComponent`) — 기본 무기 BP 장착, WeaponSocket 부착
- [x] `GA_BasicShoot` — Hitscan 발사: `LocalFire`(즉시 이펙트) + `ServerFire`(LineTrace 히트 판정 + 데미지 GE 적용 + VFX 투사체 스폰)
- [x] `ULastFPSGE_DamageInstant` — **Damage** 메타에 즉시 가산 → AttributeSet에서 Health 반영; `DamageEffectClass`는 GA_BasicShoot에서 관리
- [x] 오버히트 시스템 — 열 게이지 누적/냉각, 오버히트 시 발사 잠금
- [x] 발사 이펙트 — 머즐플래시 Cascade 파티클 + 발사음 (MuzzleFlash 소켓 기준)
- [x] 발사체 비주얼 — 스태틱 메시 대신 Cascade 트레일 파티클 (`UParticleSystemComponent`)
- [x] 무기별 애니메이션 레이어 — `ALI_AnimLayerBase` 인터페이스 + `ABP_Rifle_Layers` / `ABP_Pistol_Layers`, `WeaponAnimLayerClass` 장착 시 `LinkAnimClassLayers` 적용
- [x] 사망 애니메이션 레이어 — `bIsDead` 감지 → `UpperBody_Death` 레이어로 전환 (Blend Poses by bool)
- [x] 피격 반응 — 피격 시 사운드 재생
- [x] 크로스헤어 — 무기 장착 시 화면 중앙 크로스헤어 표시
- [x] 히트마커 — 적 명중 시 공격자 크로스헤어에 히트마커 UI 표시
- [x] ADS(어깨 너머 시점) 카메라 블렌딩

### Phase 3 — 멀티플레이어 기초 (목표: ~3주)
- [x] PlayerState에 ASC 이전 (`ALastFPSPlayerState` 신규, Owner=PlayerState / Avatar=Character)
- [x] **FFA 스타일 매치 통계** — `ALastFPSPlayerState`에 킬/데스/딜/힐 카운터 복제, `LastFPSAttributeSet::PostGameplayEffectExecute`에서 서버 갱신
- [x] 기본 이동 및 사격 네트워크 복제 검증 (CharacterMovement 기본 복제 + WeaponComponent Heat/Overheat Replicated)
- [x] GAS Prediction 적용 — GA_Jump 신규 (LocalPredicted + CMC 물리 예측), GA_Sprint 기존 LocalPredicted 확인
- [x] GameMode 기초 구현 (`ALastFPSGameModeBase`)
- [x] **FFA 정비** — `ELastFPSTeam`/팀 로스터/팀 점수 코드 일괄 제거, `PostLogin` 팀 배정 제거, FF 자동 허용. 매치 종료는 시간 만료 또는 개인 킬(`MatchKillLimit=3`) 단일 기준
- [x] GameState — 매치 타이머 노출 (`ALastFPSMatchGameState::MatchTimeRemaining` Replicated, 서버 0.25s 주기 갱신; 라운드 종료 조건은 보류)
- [x] 기본 리스폰 시스템

### Phase 4 — 스킬 시스템 (목표: ~4주)
- [x] 스킬 슬롯 아키텍처 — `InputTag.Skill1/2/Ultimate` → `ULastFPSInputConfig::AbilityInputActions` → `ALastFPSHero`에서 `TryActivateAbilitiesByTag` (`Ability.Skill1` / `Skill2` / `Ultimate`). Q/E/F는 **Started**만 사용 (홀드 취소 없음)
- [x] 프로토타입 스킬 2종 (C++ GA + C++ 기본 GE, BP에서 `DefaultAbilities`·수치 튜닝 가능)
  - [x] Q: `GA_SkillMoveBoost` — 3초 이속 증가 (`ULastFPSGE_MoveSpeedBuff`)
  - [x] E: `GA_SkillHeal` — 즉시 체력 회복 (`ULastFPSGE_HealInstant`)
  - [ ] F: 궁극기 GA 및 연출 (태그·입력만 준비됨)
- [ ] `UltimateGauge` 충전 및 임계값 트리거
- [ ] 쿨다운 UI 표시 (HUD)
- [ ] GameplayCue — 스킬 이펙트 / 사운드

### Phase 5 — UI & HUD (목표: ~2주)
- [x] 체력바 / 스태미나바 / 궁극게이지 HUD — C++ 베이스(`ULastFPSHUDWidget`) + GAS 어트리뷰트 델리게이트 바인딩, Blueprint에서 Progress Bar 연결
- [x] 오버히트 게이지 바 — `OnHeatChanged` RepNotify + 멀티캐스트 델리게이트, 오버히트 시 색상 분기
- [ ] 스킬 슬롯 쿨다운 아이콘 (Q / E / F)
- [ ] 킬피드 / 개인 점수판
- [ ] 미니맵 (참가자 위치 표시, 최대 3명)
- [x] 게임 종료 스코어보드 — `ALastFPSHUD`가 `ALastFPSMatchGameState::OnMatchEnded` 바인딩 후 자동 표시
- [x] HUD 매치 타이머 — `ULastFPSHUDWidget::OnMatchTimeChanged(float)` BP 이벤트 (1초 단위 갱신)

### Phase 6 — 게임 흐름 & 연출 (목표: ~3주)
- [x] 로비 / 매치 시작 흐름 (프로토타입) — `ALastFPSLobbyGameMode`에서 3명 입장 시 `ServerTravel(MatchMapURL)`로 게임 맵 이동, `ALastFPSMatchGameMode`로 인게임 흐름 분리
- [x] 로비 캐릭터 선택/확정 스폰 (프로토타입) — 로비 선택 인덱스 저장(`PlayerState` + `GameInstance`), 매치 첫 스폰 시 인덱스 기반 `CharacterPawnClasses` 적용
- [x] 로비 전용 폰 분리 — 로비에서는 `LobbyPawnClass` 사용, 매치에서는 전투 Pawn 스폰
- [x] 드랍 인트로 보정 — 드랍 진행 중 후속 접속 플레이어도 `PostLogin`에서 동일 낙하 연출 적용
- [ ] **[매치 인트로]** 매치 시작 시 참가자(최대 3명) 소개 UI 연출
  - UMG 참가자 소개 위젯 (닉네임, 캐릭터, 구분 컬러, 포즈)
  - 멀티에서 모든 클라이언트 동기화 (RPC 또는 GameState 플래그)
- [ ] **[낙하 인트로]** 하늘 스폰 → 착지 연출
  - `GA_DropIntro` Gameplay Ability 구현
  - 낙하 애니메이션 + 착지 임팩트 카메라 셰이크
  - 착지 완료 전 전투 입력 잠금
- [~] **[MVP 결과 화면]** 게임 종료 후 결과 화면 (C++ 데이터/흐름 완료, BP 비주얼은 작업 중)
  - [x] GameState 기반 종료(`bMatchEnded` RepNotify, `WinnerPlayerState`, `EndReason`) + MVP 선정(최다 킬, 동률 시 데스 적은 사람, 동률 시 무승부)
  - [x] 종료 시 모든 클라에 자동 스코어보드 표시 + Enhanced Input `ClearAllMappings()`로 입력 차단
  - [x] `MatchResultDisplaySeconds`(기본 8초) 후 `ServerTravel(LobbyMapURL)`
  - [ ] BP: Victory 포즈 Montage / Level Sequence 등 비주얼 폴리싱
- [ ] 히트마커, 피격 방향 표시기
- [ ] 발소리 / 총소리 3D 사운드
- [ ] 포스트 프로세싱 (피격 화면 효과)
- [ ] 기본 레벨 디자인 1맵

### Phase 7 — 최적화 & 마무리 (목표: ~2주)
- [ ] 네트워크 최적화 (넷 업데이트 빈도 튜닝)
- [ ] LOD / Nanite 설정
- [ ] 프로파일링 + 병목 제거
- [ ] 포트폴리오 빌드 패키징
- [ ] 플레이 영상 녹화 / GIF 제작

---

## 폴더 구조 계획

```
LastFPS/
├── Source/LastFPS/
│   ├── AbilitySystem/
│   │   ├── Abilities/          # GA_* C++ 클래스
│   │   ├── Effects/            # GE_* C++ 클래스
│   │   ├── AttributeSets/      # ULastFPSAttributeSet
│   │   └── GameplayCues/
│   ├── Animation/
│   │   └── LastFPSAnimInstance.h/.cpp   # Speed/Direction/bIsADS/bIsDead 등 AnimBP용 변수
│   ├── Character/
│   │   ├── LastFPSCharacterBase.h/.cpp
│   │   ├── LastFPSHero.h/.cpp
│   │   └── Components/         # WeaponComponent, etc.
│   ├── Game/
│   │   ├── LastFPSGameModeBase.h/.cpp
│   │   ├── LastFPSGameInstance.h/.cpp  # 로비 캐릭터 선택 인덱스 저장/복원
│   │   ├── LastFPSLobbyGameMode.h/.cpp   # 로비: 인원 대기, 시작 조건, 맵 이동
│   │   ├── LastFPSLobbyGameState.h/.cpp  # 로비 상태 복제 (입장 인원·시작 트리거)
│   │   ├── LastFPSMatchGameMode.h/.cpp   # 인게임: 매치 시작/종료/리스폰
│   │   ├── LastFPSMatchGameState.h/.cpp  # 매치 타이머(MatchTimeRemaining) 복제
│   │   └── LastFPSPlayerState.h/.cpp
│   ├── Input/                  # InputConfig DataAsset, IMC
│   ├── UI/
│   │   ├── LastFPSHUD.h/.cpp          # HUD 기본 클래스 (히트마커 표시)
│   │   └── LastFPSHUDWidget.h/.cpp    # UMG 위젯 C++ 베이스 (GAS 델리게이트 바인딩)
│   └── Weapons/
│
└── Content/
    ├── Blueprints/
    │   ├── Character/
    │   ├── Abilities/
    │   └── Weapons/
    ├── Maps/
    ├── UI/
    ├── VFX/
    └── Audio/
```

---

## 빌드 및 실행

### 요구 사항
- Unreal Engine 5.7
- Visual Studio 2022 (MSVC v143)
- Windows 10/11 64-bit

### 빌드
1. `LastFPS.uproject` 우클릭 → **Generate Visual Studio project files**
2. `LastFPS.sln` 열기
3. `Development Editor` | `Win64` 설정 후 빌드
4. UE Editor에서 `LastFPS.uproject` 오픈

### 서버 실행 (로컬 테스트)
```bash
# 서버
LastFPS.exe /Game/Maps/MainMap -server -log

# 클라이언트
LastFPS.exe 127.0.0.1 -game -log
```

---

## 참고 자료
- [GAS Documentation (tranek)](https://github.com/tranek/GASDocumentation)
- [UE5 Enhanced Input System](https://docs.unrealengine.com/5.0/en-US/enhanced-input-in-unreal-engine/)
- [UE Network Compendium](https://cedric-neukirchen.net/Downloads/Compendium/UE4_Network_Compendium_by_Cedric_eXi_Neukirchen.pdf)

---

*Last updated: 2026-05-11 — **종료 UX 한 사이클 완성:** ① HUD 매치 타이머(`OnMatchTimeChanged`, 1초 단위) ② 매치 종료 자동 스코어보드(`ALastFPSMatchGameState::OnMatchEnded` 델리게이트 + RepNotify, 모든 클라 자동) ③ MVP 선정(최다 킬·동률 무승부) + 입력 차단(`ClearAllMappings`) + `MatchResultDisplaySeconds`(8s) 후 `ServerTravel(LobbyMapURL)`. 전 단계 FFA 정비/매치 타이머 노출 흐름 위에 누적.*
