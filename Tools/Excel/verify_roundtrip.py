#!/usr/bin/env python3
"""워크북을 CSV로 역변환해 기존 CSV 데이터와 셀 단위로 비교한다."""

from __future__ import annotations

import argparse
import csv
import io
import tempfile
from pathlib import Path

from bootstrap_from_csv import WORKBOOK_SHEETS, decode_csv
from xlsx_to_csv import GENERATED_NOTICE, convert_sheet


REQUIRED_TABLES = {"DT_WeaponBalance", "DT_LoadingTips", "DT_QuestData"}


def read_csv_rows(path: Path) -> list[list[str]]:
    text = decode_csv(path.read_bytes(), path)
    rows = [list(row) for row in csv.reader(io.StringIO(text, newline=""))]
    return [row for row in rows if not (row and row[0] == GENERATED_NOTICE)]


def compare_rows(table_name: str, expected: list[list[str]], actual: list[list[str]]) -> list[str]:
    differences: list[str] = []
    max_rows = max(len(expected), len(actual))
    for row_index in range(max_rows):
        if row_index >= len(expected):
            differences.append(f"{table_name}: 예상에 없는 행 {row_index + 1}: {actual[row_index]}")
            continue
        if row_index >= len(actual):
            differences.append(f"{table_name}: 결과에 행 {row_index + 1}이 없습니다: {expected[row_index]}")
            continue

        expected_row = expected[row_index]
        actual_row = actual[row_index]
        max_columns = max(len(expected_row), len(actual_row))
        for column_index in range(max_columns):
            expected_value = expected_row[column_index] if column_index < len(expected_row) else "<컬럼 없음>"
            actual_value = actual_row[column_index] if column_index < len(actual_row) else "<컬럼 없음>"
            if expected_value != actual_value:
                header = expected[0][column_index] if expected and column_index < len(expected[0]) else f"#{column_index + 1}"
                differences.append(
                    f"{table_name}: 행 {row_index + 1}, 컬럼 {header}: "
                    f"원본={expected_value!r}, 결과={actual_value!r}"
                )
    return differences


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--excel-dir", type=Path, default=Path("LastFPS/Excel"))
    # 생성된 CSV는 워크북과 섞이지 않도록 별도 폴더에 모아둔다. 에디터 설정의 CsvDirectory와 같은 기본값을 쓴다.
    parser.add_argument("--csv-dir", type=Path, default=None)
    args = parser.parse_args()
    excel_directory = args.excel_dir.resolve()
    csv_directory = (args.csv_dir.resolve() if args.csv_dir else excel_directory / "Csv")

    all_differences: list[str] = []
    verified: set[str] = set()
    with tempfile.TemporaryDirectory(prefix="lastfps_excel_verify_") as temporary_directory:
        temp_root = Path(temporary_directory)
        for workbook_name, sheet_names in WORKBOOK_SHEETS.items():
            workbook_path = excel_directory / workbook_name
            if not workbook_path.is_file():
                all_differences.append(f"워크북을 찾을 수 없습니다: {workbook_path}")
                continue
            for sheet_name in sheet_names:
                source_csv = csv_directory / f"{sheet_name}.csv"
                converted_csv = temp_root / f"{sheet_name}.csv"
                convert_sheet(workbook_path, sheet_name, converted_csv, include_notice=True)
                expected = read_csv_rows(source_csv)
                actual = read_csv_rows(converted_csv)
                differences = compare_rows(sheet_name, expected, actual)
                if differences:
                    all_differences.extend(differences)
                    print(f"실패: {sheet_name} ({len(differences)}개 불일치)")
                else:
                    verified.add(sheet_name)
                    print(f"통과: {sheet_name} ({len(expected) - 1}개 데이터 행)")

    missing_required = REQUIRED_TABLES - verified
    if missing_required:
        all_differences.append(f"필수 검증 테이블이 통과하지 못했습니다: {sorted(missing_required)}")

    if all_differences:
        print("\n왕복 검증 실패:")
        for difference in all_differences[:200]:
            print(f"- {difference}")
        if len(all_differences) > 200:
            print(f"- 추가 불일치 {len(all_differences) - 200}개 생략")
        return 1

    print(f"\n왕복 검증 통과: {len(verified)}개 테이블, 필수 3종 포함")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
