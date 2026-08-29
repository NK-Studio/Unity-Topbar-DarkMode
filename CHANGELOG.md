# Changelog

All notable changes to this project will be documented in this file.

## [1.2.1] - 2026-08-29
### Fixed
- 플랫폼별 DLL 호출 처리 개선: UnityEditorDarkMode 네이티브 DLL 호출이 Windows Editor에서만 수행되도록 UNITY_EDITOR_WIN 조건부 컴파일을 추가했습니다. 이를 통해 macOS 등 비Windows 환경에서 발생하던 DLL 로드 실패 경고를 방지합니다.

## [1.2.0] - 2026-04-10

### Improved
- **라이트 모드 호환성 향상**: 라이트 모드 사용자에게도 상단 바가 다크모드로 강제 적용되던 문제를 해결하여 라이트 모드 테마를 존중하도록 개선되었습니다.
- **색상 테마 고도화**: 유니티 6 스킨의 심미성에 맞춰 전체적인 색상 테마를 기존보다 더 깊고 진한(Deep Dark) 톤으로 변경하였습니다.

### Fixed
- **버튼 렌더링 수정**: 간혹 버튼에 다크모드가 제대로 적용되지 않던 세부적인 버그들을 수정하였습니다.

## [1.1] - 2024-04-08

### Added
- Initial release of the DarkMode Mod for Unity Editor.
- Support for dark title bar, menu bar, and context menu.
- Support for Windows 10 (1903+) and Windows 11.
- Verified compatibility with Unity 2019 through Unity 6.
