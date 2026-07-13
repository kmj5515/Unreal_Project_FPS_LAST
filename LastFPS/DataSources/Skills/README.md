# 스킬 데이터 구성

- `CharacterSkill DT`: 이름, 설명, 아이콘, 키 표시, 입력/쿨다운 태그와 밸런스 행 연결을 관리한다.
- `SkillBalance DT`: 데미지, 쿨다운, 코스트, 사거리, 반경, 지속시간 등 수치만 관리한다.
- `AbilitySet DA`: 실제 동작 클래스만 기존 `GrantedAbilities`에서 관리한다.
- `SkillDataSubsystem`: 두 공용 DT를 한 번 로드하고 `CharacterId + Slot` 및 `SkillId`로 자동 검색한다.

## 가져오기와 연결

1. `DT_SkillBalance.csv`를 `FLastFPSSkillBalanceData` 행 구조로 가져온다.
2. `DT_CharacterSkills.csv`를 `FLastFPSCharacterSkillData` 행 구조로 가져온다.
3. CharacterSkill DT의 `SkillId`와 SkillBalance DT의 행 이름을 동일하게 유지한다.
4. `DefaultGame.ini`의 `LastFPSSkillDataSubsystem`에 두 공용 DT를 한 번만 지정한다.
5. 실제 Gameplay Ability 클래스는 AbilitySet DA의 `GrantedAbilities`에 유지한다. 캐릭터별 스킬 행 설정은 필요하지 않다.

CharacterSkill DT의 `InputTag`와 실제 Gameplay Ability의 Asset Tag가 일치해야 해당 능력이 올바른 밸런스 행을 찾는다. 아이콘은 아직 준비되지 않은 경우 비워 둘 수 있다.
