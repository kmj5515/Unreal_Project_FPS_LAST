# 적 AI — EQS 카이팅(거리 유지) 셋업 가이드

원거리 적이 플레이어가 가까워지면 뒤로/옆으로 빠져 거리를 유지하도록, EQS로 이동 지점을 고른다.
C++ 골격은 만들어져 있고, **EQS 쿼리 에셋과 BT 배선만 에디터에서** 하면 된다.

---

## 1. 추가된 C++

- `Character/AI/EnvQueryContext_LastFPSTarget.h/.cpp` — **EQS 컨텍스트 "타깃"**. 쿼리어(적)의 AIController 블랙보드 `TargetActor`(폴백 FocusActor)를 EQS에 제공. 거리/시야 테스트의 기준점.
- `LastFPSAIProfile.KeepDistance` — 이 거리보다 가까우면 카이팅(cm). 0이면 카이팅 안 함. 보통 `AttackRange * 0.5`.
- BB 키 `bTargetTooClose`(Bool), `KiteLocation`(Vector) 추가.
- `BTService_UpdateCombatTarget` — `bTargetTooClose = (KeepDistance>0 && Distance<KeepDistance)` 매 틱 갱신.

> EQS 클래스는 `AIModule`(이미 의존성)에 포함돼 있어 모듈 추가 불필요. 새 파일이라 **풀 빌드** 한 번.
> EQS 에셋이 안 보이면 Project Settings → AI System에서 EQS가 켜져 있는지 확인.

---

## 2. Blackboard 키 추가 (`BB_Enemy`)

| 키 | 타입 |
|----|------|
| `bTargetTooClose` | Bool |
| `KiteLocation` | Vector |

(기존 `TargetActor`/`bInAttackRange`/`bHasLineOfSight`/`TargetLocation`은 그대로.)

---

## 3. EQS 쿼리 만들기 — `EQS_EnemyKite`

콘텐츠 브라우저 → Artificial Intelligence → Environment Query.

**Generator (후보 생성):** `Points: Donut`
- Center = **Querier**(자기 자신)
- Inner Radius ≈ `KeepDistance`, Outer Radius ≈ `AttackRange`
- Number of Rings 2~3, Points Per Ring 8 정도 → 자기 주변 링 형태로 후보점.

**Tests (점수/필터):**
1. **Distance** — To = `LastFPSTarget` 컨텍스트. Scoring: **Prefer Greater**(타깃에서 멀수록 고득점). → 뒤로 빠짐.
2. **Trace (Line Of Sight)** — Context = `LastFPSTarget`. Filter = 통과만(시야 있는 점만). → 빠지면서도 쏠 수 있는 자리.
3. **Pathfinding** — Test = *Path Exists*. Filter. → 내브메시로 갈 수 있는 점만.
4. (선택) **Distance To Querier** — Prefer Lesser. 너무 멀리 안 가게.

Run Mode = **Single Best Item**.

> 핵심은 Test들의 Context를 전부 **`LastFPSTarget`**(방금 만든 C++ 컨텍스트)으로 지정하는 것. 이게 블랙보드의 플레이어를 EQS에 연결한다.

---

## 4. Behavior Tree 배선

최상위 **Selector** 아래에 카이팅 분기를 **공격보다 위(우선순위 높게)** 둔다:

```
Selector  [Service: Update Combat Target]
├─ Sequence  [Decorator: bTargetTooClose Is Set, Observer Aborts = Both]   ← 카이팅(최우선)
│    ├─ Run EQS Query   (Query = EQS_EnemyKite, Blackboard Key = KiteLocation)
│    └─ MoveTo          (Blackboard Key = KiteLocation, Acceptance ~50)
│
├─ Sequence  [Decorator: bInAttackRange Is Set, bHasLineOfSight Is Set, Observer Aborts = Both]  ← 공격
│    └─ Enemy Attack    (Target Actor Key = TargetActor)
│
└─ Chase Target         (Target Actor Key = TargetActor)                    ← 추격(폴백)
```

동작:
- 타깃이 `KeepDistance`보다 가까움 → **카이팅**(EQS로 뒤로 빠질 점 골라 이동).
- 너무 가깝지 않고 사거리+시야 확보 → **공격**.
- 그 외 → **추격**.

`Observer Aborts = Both`라서 상황이 바뀌면(가까워짐/멀어짐/사거리 진입) 즉시 분기가 전환된다.

> 주의: `Run EQS Query`의 결과 저장 키와 뒤 `MoveTo`의 키를 **둘 다 `KiteLocation`**으로 맞출 것.
> 그리고 Enemy Attack / Chase Target의 `Target Actor Key`는 `SelfActor`가 아니라 **`TargetActor`**여야 한다.

---

## 5. AIProfile 값 (원거리 적 예시)

| 필드 | 예시값 |
|------|--------|
| DetectionRange | 1600 |
| LoseSightRange | 2000 |
| AttackRange | 1200 |
| **KeepDistance** | 600 |
| ReactionDelay | 1.0 |
| bCanAttack | ✔ |
| AttackAbilityTag | `Ability.Enemy.Shoot` |

`KeepDistance(600) < AttackRange(1200)`이라, 플레이어가 600 안으로 들어오면 빠지고, 600~1200 사이에서 사격하는 원거리 카이터가 된다.

---

## 6. 테스트 체크리스트

- [ ] 레벨에 NavMeshBoundsVolume(카이팅 이동에 필수).
- [ ] `LastFPSTarget` 컨텍스트가 쿼리 Test들에 지정됐는지.
- [ ] 플레이어가 KeepDistance 안으로 들어가면 적이 뒤로 빠지는지.
- [ ] 빠진 뒤 사거리+시야에서 다시 사격하는지.
- [ ] 벽 쪽으로 몰면 EQS가 시야 있는 점을 못 찾아 추격/공격으로 폴백하는지(막히면 KeepDistance/반경 조정).
