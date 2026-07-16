# 버프·디버프 아이콘 데이터 구성

## 데이터 구조

- `DT_StatusData`: HUD와 캐릭터 Overlay를 포함한 모든 상태 표시 행을 소유하는 최상위 데이터 원본
- `FLastFPSStatusEffectUIData`: Data Table의 한 행을 표현하는 불변 상태 표시 데이터
- `ULastFPSStatusEffectDataSubsystem`: Data Table을 로드하고 `Status Gameplay Tag` 인덱스를 만드는 읽기 전용 레지스트리
- `AbilitySystemComponent`: 활성 여부, 남은 시간, 총 지속시간, 현재 스택 같은 런타임 상태의 진실 공급원
- HUD 위젯: 태그 변경 이벤트를 구독하고 서브시스템의 표시 데이터와 ASC의 런타임 상태를 조합
- `ULastFPSStatusOverlayComponent`: 캐릭터별 태그 구독, Overlay 우선순위, MID와 혼합값 보간을 담당
- `ULastFPSStatusAnimationComponent`: 상태 태그에 지정된 애니메이션 정책을 캐릭터별로 적용

별도의 `ULastFPSStatusEffectUIDataSet`이나 Data Asset 래퍼는 사용하지 않는다. Data Table 자체가 전체 행 컬렉션이기 때문이다.

`StatusTag`가 게임플레이 조회의 고유 식별자다. Data Table의 행 이름은 편집 편의를 위한 이름이며 런타임 키로 사용하지 않는다.

## 에디터 설정

1. `DT_StatusData`의 Row Structure를 `Last FPS Status Effect UI Data`로 지정한다.
2. `/Game/Data/StatusData/DT_StatusData`에 상태 효과별 행을 추가한다.
3. 각 행의 `Status Tag`, `Category`, `Display Name`, `Icon`, `Icon Tint`, `Display Priority`와 HUD 표시 옵션을 지정한다.
4. 캐릭터 Overlay가 필요하면 행의 `Overlay`를 활성화하고 Material, Stack Tag, 혼합값과 우선순위를 설정한다.
5. 현재 애니메이션 포즈를 멈춰야 하면 `Animation Policy`를 `Pause Current Pose`로 지정한다.
6. 각 Gameplay Effect가 해당 행의 `Status Tag`를 `Target Tags Gameplay Effect Component`로 부여하는지 확인한다.

`DefaultGame.ini`의 `StatusEffectUITable`은 위 Data Table을 직접 가리킨다. `FallbackIcon`은 모든 행에 공통인 선택 설정이며 서브시스템 설정에 별도로 둘 수 있다.

Reimport용 UTF-8 예시는 `Docs/DataSeeds/DT_StatusData.csv`에 있다. Unreal Editor가 Export한 `DT_StatusEffectData.csv`는 원본 백업으로 보존한다.

## Overlay 구성

- `Enabled`: 해당 상태 행의 Overlay 연출 사용 여부
- `Stack Tag`: 완성 상태 전까지 누적 강도를 계산할 상태 태그
- `Material`: 비동기로 로드할 Overlay Material
- `Stack Count For Full Mix`: 최종 혼합값에 도달할 명시적인 스택 수
- `Stack Mix Start Value`, `Stack Mix End Value`: 누적 시작과 완료 혼합값
- `Interpolate Stack Mix`, `Stack Mix Interp Speed`: 혼합값 보간 설정
- `Priority`: 여러 상태가 동시에 활성화됐을 때 선택할 우선순위

`ULastFPSStatusEffectDataSubsystem`은 설정만 제공한다. 캐릭터별 ASC, MID, 타이머와 현재 혼합값은 `ULastFPSStatusOverlayComponent`가 소유한다. Overlay Material은 상태가 처음 필요할 때 비동기로 로드한다.

서버는 최종 선택된 `StatusTag`와 `MixValue`만 Overlay 컴포넌트 상태로 복제한다. 소유·비소유 클라이언트는 복제된 태그로 자신의 Status 서브시스템에서 Material과 보간 설정을 조회한다. 따라서 Material 객체를 RPC로 전달하지 않으며 `Minimal` ASC 복제에서도 중간 스택 혼합값이 유지된다.

기존 `/Game/Data/Characters/Material/DA_StatusOverlayMaterial`은 마이그레이션 참고용으로만 남아 있다. `DT_StatusData`로 값 이전을 확인한 뒤 Blueprint의 `StatusOverlayConfig` 잔여 참조와 함께 제거한다.

## 애니메이션 반응

- `None`: 애니메이션에 영향을 주지 않는다.
- `Pause Current Pose`: 상태 태그가 하나 이상 존재하는 동안 `USkeletalMeshComponent::bPauseAnims`를 활성화해 현재 포즈를 유지한다.

`ULastFPSStatusAnimationComponent`는 서버, 소유 클라이언트와 비소유 클라이언트의 ASC 태그를 각각 구독한다. 정지를 적용하기 전에 기존 `bPauseAnims` 값을 보관하고 상태가 끝나면 복원하므로 다른 시스템이 이미 정지시킨 값을 임의로 해제하지 않는다.

사망 시에는 `LastFPSCharacterBase::UpdateAliveCollisionState`가 애니메이션 반응을 먼저 중단한다. 따라서 빙결된 상태로 죽어도 포즈 정지가 해제된 다음 래그돌 물리로 전환된다.

## 데이터 검증 규칙

- `StatusTag`는 반드시 `Status` 루트 아래의 유효한 Gameplay Tag여야 한다.
- 같은 `StatusTag`를 여러 행에 등록할 수 없다.
- `Display Priority`가 높은 항목을 먼저 배치한다.
- 우선순위가 같으면 `StatusTag` 이름순으로 정렬해 결과를 고정한다.
- 행의 아이콘이 비어 있으면 서브시스템의 `FallbackIcon`을 사용한다.

## HUD 갱신 규칙

- 표시 대상은 Data Table에 등록되어 있고 `Show On HUD`가 켜진 상태 태그뿐이다.
- `Register Gameplay Tag Event`의 `NewOrRemoved` 이벤트로 아이콘 생성과 제거를 처리한다.
- 지속시간과 스택 표시는 해당 태그를 부여한 활성 Gameplay Effect를 조회해 갱신한다.
- `Status.FreezeStack` 같은 내부 누적·방지 태그는 테이블에 넣지 않아 별도 아이콘으로 노출하지 않는다.
- 달리기는 버프가 아니므로 `Character.State.Sprinting`을 사용하며 `Status.Movement.SpeedBoost`와 분리한다.

## 위젯 블루프린트 연결

1. `Last FPS Status Effect Icon Widget`을 부모로 `WBP_StatusEffectIcon`을 만든다.
2. 필요에 따라 다음 이름으로 위젯을 배치한다. 모두 선택 바인딩이므로 사용하지 않는 항목은 생략할 수 있다.
   - `StatusIcon`: `Image`
   - `DurationProgress`: `Progress Bar`
   - `DurationText`: `Text Block`
   - `StackText`: `Text Block`
3. `Last FPS Status Effect List Widget`을 부모로 `WBP_StatusEffectList`를 만든다.
4. 루트 아래에 `Horizontal Box` 또는 `Wrap Box`를 만들고 이름을 `StatusEffectContainer`로 지정한다.
5. 클래스 기본값의 `Status Effect Icon Widget Class`에 `WBP_StatusEffectIcon`을 지정한다.
6. 기존 `WBP_HUD`에 `WBP_StatusEffectList`를 배치하고 변수 이름을 정확히 `WBP_StatusEffectList`로 지정한다.

목록은 상태 Gameplay Tag의 추가·제거 이벤트로 즉시 갱신한다. 지속시간과 스택 표시는 기존 HUD 갱신 타이머를 공유하므로 별도 Tick을 사용하지 않는다.
