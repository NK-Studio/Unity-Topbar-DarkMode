using System.Runtime.InteropServices;
using UnityEditor;
using UnityEngine;

#if UNITY_6000_5_OR_NEWER
using Unity.Scripting.LifecycleManagement;
#endif

namespace UnityTopbarDarkMode
{
#if !UNITY_6000_5_OR_NEWER
    [InitializeOnLoad]
#endif
    public static class ThemeChangePersistentDetector
    {
        private const string STATE_KEY = "MyTool_LastProSkinState";

#if UNITY_EDITOR_WIN
        [DllImport("UnityEditorDarkMode", EntryPoint = "SetThemeMode")]
        private static extern void SetThemeMode(bool isDark);
#endif

#if UNITY_6000_5_OR_NEWER

        [OnCodeInitializing]
        private static void Initialize()
        {
#if UNITY_EDITOR_WIN
            ApplyCurrentTheme();
            CheckTheme();
#endif
        }

#else

        static ThemeChangePersistentDetector()
        {
#if UNITY_EDITOR_WIN
            // 에디터 시작 시 현재 테마를 DLL에 즉시 반영
            ApplyCurrentTheme();
            CheckTheme();
#endif
        }

#endif

#if UNITY_EDITOR_WIN
        private static void CheckTheme()
        {
            bool currentIsPro = EditorGUIUtility.isProSkin;
            bool lastIsPro = SessionState.GetBool(STATE_KEY, currentIsPro);

            if (currentIsPro != lastIsPro)
            {
                OnThemeChanged(currentIsPro);
            }

            SessionState.SetBool(STATE_KEY, currentIsPro);
        }

        private static void ApplyCurrentTheme()
        {
            try
            {
                SetThemeMode(EditorGUIUtility.isProSkin);
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[ThemeDetector] DLL 호출 실패: {e.Message}");
            }
        }

        private static void OnThemeChanged(bool isDark)
        {
            try
            {
                SetThemeMode(isDark);
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[ThemeDetector] DLL 호출 실패: {e.Message}");
            }
        }
#endif
    }
}