# LastFPS — 포트폴리오 멀티플레이어 TPS/FPS 프로젝트

> Unreal Engine 5.7 + GAS(Gameplay Ability System) 기반  
> Time Takers에서 영감을 받은 팀 기반 경쟁 슈팅 게임

---

## 목차

1. [게임 개요](#게임-개요)
2. [핵심 기능 목록](#핵심-기능-목록)
3. [기술 스택](#기술-스택)
4. [아키텍처 설계](#아키텍처-설계)
5. [개발 로드맵](#개발-로드맵)
6. [폴더 구조 계획](#폴더-구조-계획)
7. [빌드 및 실행](#빌드-및-실행)

---

## 게임 개요

**장르:** 3인칭 기반 멀티플레이어 팀 배틀 슈팅  
**플랫폼:** PC (Windows)  
**네트워크:** 멀티플레이어 전용 (Dedicated Server 목표)  
**최대 인원:** 12명 (4팀 × 3인 1팀)

### 컨셉

Time Takers의 빠른 템포와 팀 시너지 메카닉을 참고하여, 각 플레이어가 고유한 스킬셋을 가지고 팀원과 협력해 상대 팀을 제압하는 경쟁 슈팅 게임.

단순 DPS 경쟁이 아닌 **역할 분담 + 스킬 조합**이 승리 핵심인 게임을 목표로 한다.

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
| 재장전 | 탄창 교체 | R |

### 어빌리티 시스템 (GAS)
| 슬롯 | 키 | 설명 |
|------|-----|------|
| 스킬 1 | Q | 캐릭터별 고유 전술 스킬 (쿨다운 8~12초) |
| 스킬 2 | E | 캐릭터별 보조/이동/유틸리티 스킬 (쿨다운 15~20초) |
| 궁극기 | F | 강력한 한 방, 쿨다운 길거나 게이지 소모 (쿨다운 60~90초 or 게이지) |

### 어트리뷰트 (GAS AttributeSet)
- **Health** — 최대 체력 / 현재 체력
- **Stamina** — 달리기·특수 이동 소모 자원
- **UltimateGauge** — 궁극기 차징 게이지 (전투 참여 시 누적)
- **AttackDamage** — 기본 공격력 (무기 별 계수)
- **Defense** — 피해 감소 수치
- **MoveSpeed** — 기본 이동속도 배율

### 게임 모드
- **팀 데스매치 (TDM):** 4팀 3인, 제한 시간 내 최다 킬 팀 승리
- (추후 확장) 점령전, 폭탄 설치전 등

### 팀 구성
```
Team A (3명)  |  Team B (3명)
Team C (3명)  |  Team D (3명)
```
- 팀당 최소 1명 ~ 최대 3명
- 팀 간 FF(아군 피격) 없음
- 개인 점수 + 팀 점수 이중 집계

### 게임 흐름 연출

#### 1. 팀 인트로 시퀀스 (Match Start)
매치 시작 직전 **팀 소개 화면** 표시:
- 화면이 팀별로 나뉘어 각 팀의 캐릭터 3인이 포즈를 취하며 등장
- 팀 이름 / 팀 컬러 / 멤버 닉네임 표시
- Sequencer 또는 UMG 애니메이션으로 연출 (2~3초)
- 이후 **낙하 인트로**로 자연스럽게 전환

#### 2. 낙하 & 착지 인트로 (Drop Intro)
팀 소개 후 전투 개시 전 **하늘에서 낙하하는 연출**:
- 모든 플레이어가 공중 고고도 지점에서 스폰
- 낙하 중 캐릭터 낙하 애니메이션 재생 (팔다리 펼침 → 수직 다이브)
- 맵 특정 착지 포인트(팀 스폰존)로 낙하
- 착지 시 **임팩트 이펙트 + 카메라 셰이크** 연출
- 착지 완료 후 전투 입력 활성화 (착지 전 이동 입력 차단)
- 구현 방식: `GA_DropIntro` GameplayAbility로 관리, 착지 판정은 `OnLanded` 오버라이드

#### 3. MVP 결과 화면 (Match End)
게임 종료 후 **MVP 캐릭터 하이라이트**:
- 1위 팀 + 개인 MVP(최다 킬 or 최고 점수) 선정
- MVP 캐릭터가 화면 중앙에서 **Victory 포즈 애니메이션** 재생
- 뒤에 팀원들이 함께 등장하는 그룹 연출
- 패배팀은 화면 한편에 흐릿하게 표시
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
        │     ├── GA_Skill1 (Q)
        │     ├── GA_Skill2 (E)
        │     └── GA_Ultimate (F)
        ├── GameplayEffects
        │     ├── GE_Damage              // 피해 적용
        │     ├── GE_Heal
        │     ├── GE_SprintSpeed         // MoveSpeed +300 (Infinite)
        │     ├── GE_SprintStaminaDrain  // Stamina -20/s (Periodic)
        │     ├── GE_StaminaRegen        // Stamina +5/s (Passive, always-on)
        │     └── GE_UltGaugeCharge
        └── GameplayCues                 // 이펙트·사운드 트리거
```

### 네트워크 구조
```
Dedicated Server
  ├── ALastFPSGameMode        // 규칙 관리 (서버 전용)
  ├── ALastFPSGameState        // 팀 점수, 타이머 (모든 클라 복제)
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
- [ ] 기본 이동 애니메이션 블렌드스페이스 연결

### Phase 2 — 전투 시스템 기초 (목표: ~3주)
- [x] 무기 컴포넌트 설계 (`UWeaponComponent`)
- [x] `GA_BasicShoot` — 발사체 선택
- [x] `GE_Damage` — 피해 GameplayEffect + 어트리뷰션
- [ ] 피격 반응 (히트 리액션, 사망 처리)
- [ ] 재장전 어빌리티 구현
- [ ] ADS(어깨 너머 시점) 카메라 블렌딩

### Phase 3 — 멀티플레이어 기초 (목표: ~3주)
- [ ] PlayerState에 ASC 이전 (멀티 표준 구조)
- [ ] 기본 이동 및 사격 네트워크 복제 검증
- [ ] GAS Prediction 적용 (GA_Jump, GA_Sprint)
- [x] GameMode 기초 구현 (`ALastFPSGameModeBase`)
- [x] 팀 배정 로직 (최대 4팀 × 3인, 인원 균등 배분)
- [ ] GameState 팀 점수 관리
- [ ] 기본 리스폰 시스템

### Phase 4 — 스킬 시스템 (목표: ~4주)
- [ ] 스킬 슬롯 아키텍처 확정 (Q / E / F 바인딩)
- [ ] 프로토타입 스킬 3종 세트 구현
  - [ ] Q: 예) 섬광탄 or 순간이동 대시
  - [ ] E: 예) 실드 배리어 or 회복 필드
  - [ ] F: 예) 광역 공격 궁극기
- [ ] `UltimateGauge` 충전 및 임계값 트리거
- [ ] 쿨다운 UI 표시 (HUD)
- [ ] GameplayCue — 스킬 이펙트 / 사운드

### Phase 5 — UI & HUD (목표: ~2주)
- [ ] 체력바 / 스태미나바 / 궁극게이지 HUD
- [ ] 스킬 슬롯 쿨다운 아이콘 (Q / E / F)
- [ ] 킬피드 / 팀 점수판
- [ ] 미니맵 (팀원 위치 표시)
- [ ] 게임 종료 스코어보드

### Phase 6 — 게임 흐름 & 연출 (목표: ~3주)
- [ ] 로비 / 매치 시작 흐름
- [ ] 팀 데스매치 규칙 완성 (타이머, 킬 리밋)
- [ ] **[팀 인트로]** 매치 시작 시 팀별 캐릭터 소개 UI 연출
  - UMG 팀 소개 위젯 (팀 컬러, 닉네임, 캐릭터 포즈)
  - 멀티에서 모든 클라이언트 동기화 (RPC 또는 GameState 플래그)
- [ ] **[낙하 인트로]** 하늘 스폰 → 착지 연출
  - `GA_DropIntro` Gameplay Ability 구현
  - 낙하 애니메이션 + 착지 임팩트 카메라 셰이크
  - 착지 완료 전 전투 입력 잠금
- [ ] **[MVP 결과 화면]** 게임 종료 후 MVP 하이라이트
  - GameState에서 MVP 선정 로직 (킬/딜/어시 가중치)
  - MVP Victory 포즈 Montage + 팀원 그룹 연출
  - 개인 스탯 스코어보드 오버레이 (K/D/A/딜량)
  - Level Sequence 또는 전용 MVP 레벨 전환
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
│   ├── Character/
│   │   ├── LastFPSCharacterBase.h/.cpp
│   │   ├── LastFPSHero.h/.cpp
│   │   └── Components/         # WeaponComponent, etc.
│   ├── Game/
│   │   ├── LastFPSGameMode.h/.cpp
│   │   ├── LastFPSGameState.h/.cpp
│   │   └── LastFPSPlayerState.h/.cpp
│   ├── Input/                  # InputConfig DataAsset, IMC
│   ├── UI/                     # UMG 위젯 C++ 베이스
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

*Last updated: 2026-05-02*
