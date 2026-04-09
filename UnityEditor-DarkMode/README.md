# 빌드 방법 (Build Instructions)

이 프로젝트를 직접 빌드하려면 시스템에 최신 버전의 `CMake`, `Visual Studio` 및 `MSVC 도구 모음(toolchain)`이 설치되어 있어야 합니다.

## 지원되는 빌드 도구
- **Visual Studio**: 설치 시 'C++를 사용한 데스크톱 개발' 워크로드를 포함해야 하며, MSVC 컴파일러를 사용합니다.
- **CLion**: 프로젝트 설정의 **Toolchains**에서 **MinGW가 아닌 Visual Studio (MSVC)**를 선택해야 합니다. 이 프로젝트는 Win32 API 및 MSVC 전용 라이브러리를 사용하므로 MinGW로는 빌드가 불가능합니다.

## 빌드 방법

### 1. 에디터 GUI 활용 (추천)
- **Visual Studio**: 프로젝트 폴더를 열거나 생성된 솔루션(`.sln`) 파일을 연 뒤, 상단 메뉴에서 **빌드(Build) > 솔루션 빌드(Build Solution)**를 클릭합니다 (단축키: `Ctrl + Shift + B`).
- **CLion**: 프로젝트를 불러온 후, 상단 툴바의 **빌드 아이콘(망치 모양)**을 클릭하여 빌드할 수 있습니다. 상단 구성(Configuration)이 `Release`로 되어 있는지 확인하세요.

### 2. 터미널(CLI) 활용
터미널 또는 명령 프롬프트(CMD)를 열고 `UnityEditor-DarkMode` 디렉토리로 이동한 후 아래 명령어를 실행합니다:
```cmd
cmake -B build
cmake --build build --config Release
```
> **참고**: `cmake` 명령어를 실행할 수 없는 경우, CMake 설치 경로가 시스템 환경 변수(Path)에 등록되어 있는지 확인하세요.

## 결과물 확인
빌드가 성공적으로 완료되면 `build/Release` 디렉토리(또는 에디터에서 지정한 빌드 출력 폴더)에 `UnityEditorDarkMode.dll` 파일이 생성됩니다.
