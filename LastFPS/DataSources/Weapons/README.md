# 무기 밸런스 데이터

`DT_WeaponBalance.csv`는 `FLastFPSWeaponBalanceData` 행 구조로 임포트한다.

- 행 이름은 `ULastFPSWeaponDefinition::WeaponId`와 같아야 한다.
- `Damage`는 최소/최대 피해가 아니라 ±20% 공식을 적용하기 전 기준값이다.
- 메시, 애니메이션, 사운드, 투사체 클래스 같은 에셋 구성은 WeaponDefinition DA에 유지한다.
- 무기 고유 숫자는 `Parameters`에 Gameplay Tag 키로 추가한다.
