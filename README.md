# Unity Editor Topbar Dark Mode (GitUPM)

![Unity Editor](https://img.shields.io/badge/Unity-2019.4%2B-black?logo=unity&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows-blue?logo=windows)
![License](https://img.shields.io/badge/License-MIT-green)

Windows 환경의 유니티 에디터 상단 바(Title Bar, Menu Bar)를 세련된 다크 모드로 테마링해주는 플러그인입니다. 유니티 공식 다크 테마보다 더욱 깊이 있는 색감을 제공하며, 패키지 매니저(UPM)를 통해 간편하게 설치하여 사용할 수 있습니다.

---

## 📸 Preview

<img width="3839" height="2046" alt="image" src="https://github.com/user-attachments/assets/a631bf9c-6ce6-48c2-b7c8-bcece7230669" />


---

## ✨ Key Improvements (vs Original)

이 프로젝트는 [0x7c13/UnityEditor-DarkMode](https://github.com/0x7c13/UnityEditor-DarkMode)의 향상된 버전입니다.

-   **Seamless Theme Switching**: 에디터의 테마 설정을 자동으로 감지하여 라이트/다크 모드에 맞게 유동적으로 반응합니다.
-   **Deep Dark Aesthetic**: 고해상도 모니터와 최신 유니티 6 스킨에 최적화된 고유의 다크 톤을 적용했습니다.
-   **UPM Support**: Git URL을 통해 복잡한 과정 없이 즉시 설치 가능합니다.

---

## 📦 How to Install (Git UPM)

Unity Package Manager에서 다음 URL을 추가하세요:

```text
https://github.com/NK-Studio/Unity-Topbar-DarkMode.git#upm
```

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

## 🛠 Prerequisites

-   Windows 10 (1903 이상) 또는 Windows 11
-   Unity 2019.4 이상 (Unity 6 포함)

---

## 📜 Credits

Based on the work of [0x7c13 (Jiaqi Liu)](https://github.com/0x7c13/UnityEditor-DarkMode).

## 📄 License

Distributed under the MIT License. See `LICENSE.txt` for more information.
