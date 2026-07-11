# LastFPS 코드 작성 지침

이 프로젝트의 실제 컨벤션을 정리한 것. 목표는 **SOLID · 유지보수성 · 확장성**이고,
새 코드는 아래를 기본으로 따른다. (UE 5.7+, C++17/20)

---

## 1. 핵심 원칙

- **데이터 주도(Data-Driven)**: 값·행동을 코드에 박지 말고 DataAsset/DataTable/GameplayTag로 뺀다.
  예) 공격 방식 = `AttackAbilityTag`, 스탯 = `StatData`, AI 파라미터 = `AIProfile`.
- **서버 권위(Server-Authoritative)**: 게임플레이 상태 변경은 서버에서. 클라는 복제된 결과만 표시.
  `HasAuthority()` 가드를 습관화.
- **단일 책임**: 한 클래스는 한 가지 이유로만 바뀌게. (컨트롤러=결정, BT노드=행동조각, 데이터에셋=파라미터)
- **개방-폐쇄**: 새 적/무기/능력은 **새 데이터에셋 + 태그**로 추가. 기존 코드 수정 최소화.
- **의존 역전**: 구체 클래스가 아니라 인터페이스/태그/추상에 의존.
  예) 공격이 `GA_BasicShoot`(구체)이 아니라 `Ability.Enemy.Shoot`(태그)에 의존.

---

## 2. 네이밍 · 파일 구조

- 클래스 접두사 + 프로젝트 접두사: `ALastFPS...`(액터), `ULastFPS...`(오브젝트), `FLastFPS...`(구조체), `ELastFPS...`(enum).
  단, BT 노드는 관례상 `UBTTask_...` / `UBTService_...`.
- 파일 1개 = 타입 1개(주 타입). 헤더/구현 쌍.
- 폴더는 도메인별로: `Character/`, `Character/AI/`, `AbilitySystem/Abilities/`, `Projectiles/`, `UI/...`, `Data/...`.
- include 경로는 모듈 루트 기준: `#include "Character/AI/Foo.h"` (프로젝트가 ModuleDirectory를 include path에 추가함).

---

## 3. UPROPERTY · 메모리 · 리플렉션

- UObject 포인터 멤버는 **`TObjectPtr<T>` + `UPROPERTY()`**. 날포인터 금지(GC 안전).
- 약참조는 `TWeakObjectPtr<T>` (예: 소유하지 않는 대상 캐시).
- 에디터 노출은 목적에 맞는 지정자:
  - 디자이너가 에셋에서 설정 = `EditDefaultsOnly` (인스턴스별이면 `EditAnywhere`)
  - BP 읽기 전용 = `BlueprintReadOnly`, 카테고리 항상 지정 `Category="LastFPS|..."`
- 헤더에선 **전방 선언** 우선, `#include`는 .cpp에. (컴파일 시간·순환 방지)
  단, USTRUCT를 UFUNCTION/UPROPERTY에 값으로 쓰면 헤더 include 필요(UHT 요구).
- 숫자 파라미터엔 `meta=(ClampMin=...)` 등으로 안전 범위.

---

## 4. const · 안전성 · 방어 코딩

- 상태를 안 바꾸는 함수는 `const`. 게터는 `const` + 가능하면 인라인.
- 이른 반환(early-return)으로 중첩 줄이기. null 체크 먼저.
  ```cpp
  UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
  if (!ASC) return;
  ```
- 포인터는 쓰기 전에 유효성 검사. `Cast<>` 결과도 항상 체크.
- `TObjectPtr`/약참조는 `.Get()` 후 null 체크.
- 매직넘버 대신 이름 있는 상수/데이터. (예: `AssistTimeWindow`)

---

## 5. GAS(Gameplay Ability System) 규칙

- **ASC 소유권 이중 모델**을 지킨다: 플레이어=PlayerState 소유, 논플레이어(적/더미)=캐릭터 자신 소유.
  접근은 항상 `GetAbilitySystemComponent()`를 통해(둘 다 처리됨).
- 어트리뷰트 추가 시 세트(4종): `UPROPERTY(ReplicatedUsing=OnRep_X)` + `ATTRIBUTE_ACCESSORS` + `GetLifetimeReplicatedProps`의 `DOREPLIFETIME_CONDITION_NOTIFY` + `OnRep_X`, 그리고 필요 시 `PreAttributeChange` 클램프 + 생성자 `InitX`.
- 어빌리티는 **태그로 발동**(`TryActivateAbilitiesByTag`). 어빌리티의 Asset Tag를 그 태그로.
- 데미지는 반드시 GAS 파이프라인(메타 어트리뷰트 `Damage` + GE)로. `ApplyDamage` 직접 호출로 우회 금지(크리티컬·어시스트·킬피드 로직 유지).
- AI/논플레이어용 어빌리티는 **Hero 의존 금지**. `ALastFPSCharacterBase` 기준으로 작성, NetExecutionPolicy=ServerOnly.

---

## 6. 네트워크 · 복제

- 복제 변수는 `UPROPERTY(Replicated 또는 ReplicatedUsing=OnRep_...)` + `GetLifetimeReplicatedProps` 등록.
- 권위 분기: 게임플레이 변경은 `HasAuthority()` 안에서. 스폰/데미지/사망 처리 서버 전용.
- 시각/사운드 등 연출은 `Multicast`(신뢰성은 상황에 맞게), 특정 클라만이면 `Client`.
- 하드 트래블(`OpenLevel`/`ServerTravel`)은 월드를 파괴함을 전제로 설계. 유지 대상은 GameInstance 소유로.

---

## 7. AI · Behavior Tree

- 컨트롤러=감지/결정, BT 노드=행동 조각, `AIProfile`=파라미터. 값은 프로파일/어트리뷰트에서 읽는다.
- Blackboard 키 이름은 **상수 헤더 한 곳**(`LastFPSEnemyBlackboardKeys.h`)에서 공유. 문자열 오타 방지.
- BT 노드 키 선택은 `FBlackboardKeySelector` + `ResolveSelectedKey` + 타입 필터.
- AIController 로직은 서버에서만 도는 걸 전제(예측 없음).

---

## 8. 게임플레이 태그

- C++에서 참조하는 태그는 **네이티브 선언**: `UE_DECLARE_GAMEPLAY_TAG_EXTERN`(.h) + `UE_DEFINE_GAMEPLAY_TAG_COMMENT`(.cpp).
- 데이터/에디터에서만 쓰는 태그는 `DefaultGameplayTags.ini`로도 충분.
- 태그 문자열은 계층 네이밍: `Ability.Enemy.Shoot`, `Input.Move`, `Status.Freeze`.

---

## 9. 주석 · 문서화

- 헤더 주석은 **"무엇/왜"**(책임·설계 의도), 구현 주석은 **"까다로운 이유"**만.
  ```cpp
  // 오프스크린에서도 포즈 갱신 — 없으면 첫 프레임에 T-포즈로 보임.
  ```
- 공개 API엔 `/** ... */` 요약. 자명한 코드에 장황한 주석 금지.
- 임시/차선책은 `// TODO:`로 의도와 교체 조건 명시. (예: 팀 시스템 교체 예정)

---

## 10. 로깅 · 디버깅

- 도메인별 로그 카테고리 사용(`DEFINE_LOG_CATEGORY_STATIC`). 남발 금지.
- 실패 경로엔 원인이 드러나는 로그: `*GetNameSafe(Obj)` 활용.
- 디버그 시각화/치트성 콘솔 명령은 `ECVF_Cheat`, 소멸 시 해제.

---

## 11. 커밋 전 체크

- [ ] 새 UObject 멤버가 `TObjectPtr`+`UPROPERTY`인가.
- [ ] 게임플레이 변경에 `HasAuthority()` 가드가 있는가.
- [ ] 복제 변수가 `GetLifetimeReplicatedProps`에 등록됐는가.
- [ ] 하드코딩 값이 데이터(에셋/태그)로 뺄 수 있는 건 아닌가.
- [ ] 논플레이어 코드가 Hero에 의존하지 않는가.
- [ ] 헤더 include를 전방 선언으로 줄일 수 있는가.

---

> 이 문서는 살아있는 규칙이라, 새 패턴이 자리 잡으면 갱신한다.
> 핵심 규칙 몇 개는 `CLAUDE.md`에 요약해 두면 AI 보조 작업에도 일관되게 반영된다.
