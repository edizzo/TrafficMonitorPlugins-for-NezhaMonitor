#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include "api.h"
class SettingsDialog {
public:
    struct SettingsData {
        std::string serverUrl;
        std::string username;
        std::string password;
        std::vector<int> serverIds;
    };

    using OnTestConnectionCallback = std::function<bool(const SettingsData&)>;
    using OnSettingsChangedCallback = std::function<void(const SettingsData&)>;

    SettingsDialog();
    ~SettingsDialog();

    // 显示设置对话框
    bool ShowModal(HWND hParent, const SettingsData& currentSettings);

    // 获取设置数据
    const SettingsData& GetSettings() const { return m_settings; }

    // 设置回调函数
    void SetTestConnectionCallback(OnTestConnectionCallback callback) { m_testConnectionCallback = callback; }
    void SetSettingsChangedCallback(OnSettingsChangedCallback callback) { m_settingsChangedCallback = callback; }

private:
    SettingsData m_settings;
    OnTestConnectionCallback m_testConnectionCallback;
    OnSettingsChangedCallback m_settingsChangedCallback;

    HWND m_hWnd;
    HWND m_hUrlEdit;
    HWND m_hUserEdit;
    HWND m_hPwdEdit;
    HWND m_hIdEdit;
    HWND m_hShowPwdCheck;
    HWND m_hTestBtn;

    bool m_modalResult;
    UINT m_dpi;

    // 窗口过程
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 创建控件
    void CreateControls();
    void UpdateControls();
    void LoadSettings();
    void SaveSettings();

    // 事件处理
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnSize(int width, int height);
    void OnTestConnection();
    void OnShowPassword();
    void OnOk();
    void OnCancel();

    // 辅助函数
    std::wstring Utf8ToWide(const std::string& str);
    std::string WideToUtf8(const std::wstring& wstr);
    int GetDpiScaled(int value) { return MulDiv(value, m_dpi, 96); }

    // 控件ID定义
    enum {
        IDC_URL_EDIT = 1001,
        IDC_USER_EDIT = 1002,
        IDC_PWD_EDIT = 1003,
        IDC_ID_EDIT = 1004,
        IDC_SHOW_PWD_CHECK = 1100,
        IDC_TEST_BTN = 1200,
        IDC_MAIN_GROUP = 2000,
        IDC_SERVER_GROUP = 2001,
        IDC_MONITOR_GROUP = 2002,
        IDC_STATUS_GROUP = 2003
    };
};
