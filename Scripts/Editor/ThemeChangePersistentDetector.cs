using System.Runtime.InteropServices;
using UnityEditor;
using UnityEngine;

namespace UnityTopbarDarkMode
{
    [InitializeOnLoad]
    public static class ThemeChangePersistentDetector
    {
        private const string STATE_KEY = "MyTool_LastProSkinState";

        [DllImport("UnityEditorDarkMode", EntryPoint = "SetThemeMode")]
        private static extern void SetThemeMode(bool isDark);

        static ThemeChangePersistentDetector()
        {
            // 에디터 시작 시 현재 테마를 DLL에 즉시 반영
            ApplyCurrentTheme();
            CheckTheme();
        }

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
    }
}