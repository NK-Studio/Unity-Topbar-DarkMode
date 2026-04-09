# Unity Editor Topbar Dark Mode

![Unity Editor Dark Mode](https://img.shields.io/badge/Unity-2019.4%2B-black?logo=unity&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows-blue?logo=windows)
![License](https://img.shields.io/badge/License-MIT-green)

Windows 기반 유니티 에디터의 상단 타이틀 바와 메뉴 바를 세련된 다크 모드로 변경해주는 라이브러리입니다. 본 프로젝트는 [0x7c13/UnityEditor-DarkMode](https://github.com/0x7c13/UnityEditor-DarkMode) 소스코드를 기반으로 한 향상된 버전(Enhanced Version)입니다.

---

## 📸 Screenshots

*(여기에 유니티 에디터 상단 타이틀 바 스크린샷이 들어갈 예정입니다)*

---

## ✨ Features (Enhanced)

기존 원본 버전에서 다음과 같은 기능들이 개선되었습니다:

- **라이트 모드 테마 존중**: 유니티 에디터가 라이트 모드일 때는 다크 모드를 강제하지 않도록 수정되었습니다.
- **Deep Dark 테마**: Unity 6의 새로운 스킨 디자인에 맞춰 더욱 깊고 진한 색감으로 고도화되었습니다.
- **안정성 향상**: 버튼 렌더링 및 에디터 재시작 시의 테마 감지 로직이 개선되었습니다.

---

## 🚀 Installation (Git UPM)

1. Unity Editor에서 **Window > Package Manager**를 엽니다.
2. `+` 버튼을 클릭하고 **Add package from git URL...**을 선택합니다.
3. 아래 URL을 입력합니다:

`https://github.com/NK-Studio/Unity-Topbar-DarkMode.git#upm`

---

## 🎨 How to change the theme? (INI)

첫 실행 후, DLL이 있는 디렉토리에 `UnityEditorDarkMode.dll.ini` 파일이 생성됩니다. 이 파일의 값을 수정하여 테마 색상을 변경할 수 있습니다 (수정 후 에디터 재시작 필요).

```ini
menubar_textcolor = 200,200,200
menubar_textcolor_disabled = 160,160,160
menubar_bgcolor = 48,48,48
menubaritem_bgcolor = 48,48,48
menubaritem_bgcolor_hot = 62,62,62
menubaritem_bgcolor_selected = 62,62,62
```

---

## 🛠 How to build it? (C++)

직접 DLL을 빌드하고 싶다면 `UnityEditor-DarkMode` 폴더에서 다음 과정을 수행하세요:

1. `CMake`, `Visual Studio`, `MSVC toolchain`이 설치되어 있어야 합니다.
2. 터미널에서 다음 명령을 실행합니다:
   ```cmd
   cmake -B build && cmake --build build --config Release
   ```
3. 빌드 성공 시 `build\Release` 디렉토리에 `UnityEditorDarkMode.dll`이 생성됩니다.

---

## 📜 Credits
This project is an enhanced version of the original work by **0x7c13 (Jiaqi Liu)**.
Original Repository: [0x7c13/UnityEditor-DarkMode](https://github.com/0x7c13/UnityEditor-DarkMode)

## 📄 License
이 프로젝트는 MIT 라이선스에 따라 라이선스가 부여됩니다. 자세한 내용은 `LICENSE.txt` 파일을 참조하세요.
