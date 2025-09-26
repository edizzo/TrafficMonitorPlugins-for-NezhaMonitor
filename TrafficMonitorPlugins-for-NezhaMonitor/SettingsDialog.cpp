#include "pch.h"
#include "SettingsDialog.h"
#include <sstream>
#include <thread>

SettingsDialog::SettingsDialog()
    : m_hWnd(nullptr)
    , m_hUrlEdit(nullptr)
    , m_hUserEdit(nullptr)
    , m_hPwdEdit(nullptr)
    , m_hIdEdit(nullptr)
    , m_hShowPwdCheck(nullptr)
    , m_hTestBtn(nullptr)
    , m_modalResult(false)
    , m_dpi(96) {
}

SettingsDialog::~SettingsDialog() {
}

bool SettingsDialog::ShowModal(HWND hParent, const SettingsData& currentSettings) {
    m_settings = currentSettings;
    m_modalResult = false;

    // 获取DPI
    typedef UINT(WINAPI* GETDPIFORWINDOW)(HWND);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        GETDPIFORWINDOW pGetDpiForWindow = (GETDPIFORWINDOW)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGetDpiForWindow && hParent) {
            m_dpi = pGetDpiForWindow(hParent);
        }
    }

    // 注册窗口类
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"NezhaSettingsDialog";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255)); // 纯白色背景

    static ATOM atom = RegisterClassW(&wc);
    if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    // 计算窗口大小 - 增加宽度和高度以提供更多空间
    RECT rc{ 0, 0, 900, 500 };
    AdjustWindowRectEx(&rc, WS_CAPTION | WS_SYSMENU, FALSE, 0);

    // 计算居中位置
    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    // 创建窗口
    m_hWnd = CreateWindowExW(
        WS_EX_CONTROLPARENT | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Nezha Monitor 设置",
        WS_CAPTION | WS_SYSMENU,
        x, y,
        windowWidth, windowHeight,
        hParent, nullptr, GetModuleHandleW(nullptr), this
    );

    if (!m_hWnd) {
        return false;
    }

    // 创建控件
    CreateControls();
    UpdateControls();

    // 显示窗口
    if (hParent) EnableWindow(hParent, FALSE);
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    // 消息循环
    MSG msg{};
    while (IsWindow(m_hWnd) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(m_hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // 恢复父窗口
    if (hParent) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
    }

    return m_modalResult;
}

LRESULT CALLBACK SettingsDialog::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* self = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<SettingsDialog*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else {
        self = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT SettingsDialog::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        OnCommand(wParam, lParam);
        return 0;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)CreateSolidBrush(RGB(255, 255, 255));
    }

    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(240, 240, 240));
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)CreateSolidBrush(RGB(240, 240, 240));
    }

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        return 1;
    }

    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_USER + 1: // 测试完成，恢复按钮
        if (m_hTestBtn) {
            SetWindowTextW(m_hTestBtn, L"测试连接");
            EnableWindow(m_hTestBtn, TRUE);
        }
        return 0;

    case WM_USER + 2: // 连接失败
        MessageBoxW(hWnd, L"连接失败，请检查服务器地址和凭据", L"测试连接", MB_OK | MB_ICONERROR);
        return 0;

    case WM_USER + 3: // 连接成功
        MessageBoxW(hWnd, L"连接成功！", L"测试连接", MB_OK | MB_ICONINFORMATION);
        return 0;

    case WM_USER + 4: // 连接异常
        MessageBoxW(hWnd, L"连接失败，请检查网络和服务器状态", L"测试连接", MB_OK | MB_ICONERROR);
        return 0;

    case WM_CLOSE:
        OnCancel();
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void SettingsDialog::CreateControls() {
    // 创建统一字体
    HFONT hNormalFont = CreateFontW(-MulDiv(10, m_dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    HFONT hTitleFont = CreateFontW(-MulDiv(14, m_dpi, 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    HFONT hGroupFont = CreateFontW(-MulDiv(10, m_dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    HFONT hTipFont = CreateFontW(-MulDiv(9, m_dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    // 主标题 - 居中显示
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"Nezha Monitor 设置",
        WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 20, 900, 30,
        m_hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

    // ---------------- 服务器配置 Group ----------------
    HWND hServerGroup = CreateWindowExW(0, L"BUTTON", L"服务器配置",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        40, 70, 400, 200,  // 增加高度和宽度
        m_hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(hServerGroup, WM_SETFONT, (WPARAM)hGroupFont, TRUE);

    int labelW = 100, editW = 250, rowH = 32;
    int startX = 60, startY = 100;  // 增加间距

    // 服务器地址
    HWND hUrlLabel = CreateWindowExW(0, L"STATIC", L"服务器地址:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        startX, startY, labelW, rowH, m_hWnd, nullptr, nullptr, nullptr);
    SendMessageW(hUrlLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hUrlEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        startX + labelW + 10, startY, editW, rowH,
        m_hWnd, (HMENU)IDC_URL_EDIT, nullptr, nullptr);
    SendMessageW(m_hUrlEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // 用户名
    HWND hUserLabel = CreateWindowExW(0, L"STATIC", L"用户名:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        startX, startY + rowH + 15, labelW, rowH, m_hWnd, nullptr, nullptr, nullptr);
    SendMessageW(hUserLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hUserEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        startX + labelW + 10, startY + rowH + 15, editW, rowH,
        m_hWnd, (HMENU)IDC_USER_EDIT, nullptr, nullptr);
    SendMessageW(m_hUserEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // 密码 + 显示复选框
    HWND hPwdLabel = CreateWindowExW(0, L"STATIC", L"密码:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        startX, startY + 2 * (rowH + 15), labelW, rowH, m_hWnd, nullptr, nullptr, nullptr);
    SendMessageW(hPwdLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hPwdEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD,
        startX + labelW + 10, startY + 2 * (rowH + 15), editW - 90, rowH,
        m_hWnd, (HMENU)IDC_PWD_EDIT, nullptr, nullptr);
    SendMessageW(m_hPwdEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hShowPwdCheck = CreateWindowExW(0, L"BUTTON", L"显示密码",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        startX + labelW + editW - 80, startY + 2 * (rowH + 15), 80, rowH,
        m_hWnd, (HMENU)IDC_SHOW_PWD_CHECK, nullptr, nullptr);
    SendMessageW(m_hShowPwdCheck, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // ---------------- 监控配置 Group ----------------
    HWND hMonitorGroup = CreateWindowExW(0, L"BUTTON", L"监控配置",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        460, 70, 400, 200,  // 调整位置和大小
        m_hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(hMonitorGroup, WM_SETFONT, (WPARAM)hGroupFont, TRUE);

    int monitorX = 480, monitorY = 100;

    HWND hIdLabel = CreateWindowExW(0, L"STATIC", L"服务器ID:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        monitorX, monitorY, labelW, rowH, m_hWnd, nullptr, nullptr, nullptr);
    SendMessageW(hIdLabel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    m_hIdEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        monitorX + labelW + 10, monitorY, editW, rowH,
        m_hWnd, (HMENU)IDC_ID_EDIT, nullptr, nullptr);
    SendMessageW(m_hIdEdit, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    // 提示文字
    HWND hTip = CreateWindowExW(0, L"STATIC", L"多个ID请用逗号分隔",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        monitorX + labelW + 10, monitorY + rowH + 8, editW, 25,
        m_hWnd, nullptr, nullptr, nullptr);
    SendMessageW(hTip, WM_SETFONT, (WPARAM)hTipFont, TRUE);

    // ---------------- 底部按钮 ----------------
    int btnW = 120, btnH = 35, margin = 40;
    int bottomY = 320;  // 降低按钮位置，增加空间

    m_hTestBtn = CreateWindowExW(0, L"BUTTON", L"测试连接",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin, bottomY, btnW, btnH,
        m_hWnd, (HMENU)IDC_TEST_BTN, nullptr, nullptr);
    SendMessageW(m_hTestBtn, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    HWND btnOk = CreateWindowExW(0, L"BUTTON", L"保存",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        900 - 2 * (btnW + 20) - margin, bottomY, btnW, btnH,
        m_hWnd, (HMENU)IDOK, nullptr, nullptr);
    SendMessageW(btnOk, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    HWND btnCancel = CreateWindowExW(0, L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE,
        900 - (btnW + margin), bottomY, btnW, btnH,
        m_hWnd, (HMENU)IDCANCEL, nullptr, nullptr);
    SendMessageW(btnCancel, WM_SETFONT, (WPARAM)hNormalFont, TRUE);

    SetFocus(m_hUrlEdit);
}

void SettingsDialog::UpdateControls() {
    LoadSettings();
}

void SettingsDialog::LoadSettings() {
    SetWindowTextW(m_hUrlEdit, Utf8ToWide(m_settings.serverUrl).c_str());
    SetWindowTextW(m_hUserEdit, Utf8ToWide(m_settings.username).c_str());
    SetWindowTextW(m_hPwdEdit, Utf8ToWide(m_settings.password).c_str());

    // 组合服务器ID
    std::wstringstream wIds;
    for (size_t i = 0; i < m_settings.serverIds.size(); ++i) {
        if (i > 0) wIds << L",";
        wIds << m_settings.serverIds[i];
    }
    SetWindowTextW(m_hIdEdit, wIds.str().c_str());
}

void SettingsDialog::SaveSettings() {
    wchar_t wbuf[512]{};

    GetWindowTextW(m_hUrlEdit, wbuf, 511);
    m_settings.serverUrl = WideToUtf8(std::wstring(wbuf));

    GetWindowTextW(m_hUserEdit, wbuf, 511);
    m_settings.username = WideToUtf8(std::wstring(wbuf));

    GetWindowTextW(m_hPwdEdit, wbuf, 511);
    m_settings.password = WideToUtf8(std::wstring(wbuf));

    GetWindowTextW(m_hIdEdit, wbuf, 511);
    // 解析服务器ID
    m_settings.serverIds.clear();
    std::wstring wline = wbuf;
    std::wstringstream ss(wline);
    std::wstring token;
    while (std::getline(ss, token, L',')) {
        try {
            int id = std::stoi(token);
            if (id > 0) m_settings.serverIds.push_back(id);
        }
        catch (...) {}
    }
    if (m_settings.serverIds.empty()) {
        m_settings.serverIds.push_back(1);
    }
}

void SettingsDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
    case IDC_SHOW_PWD_CHECK:
        if (HIWORD(wParam) == BN_CLICKED) {
            OnShowPassword();
        }
        break;

    case IDC_TEST_BTN:
        if (HIWORD(wParam) == BN_CLICKED) {
            OnTestConnection();
        }
        break;

    case IDOK:
        OnOk();
        break;

    case IDCANCEL:
        OnCancel();
        break;
    }
}

void SettingsDialog::OnSize(int width, int height) {
    // 固定布局，不需要调整
}

void SettingsDialog::OnTestConnection() {
    SaveSettings();

    if (m_settings.serverUrl.empty() || m_settings.username.empty() || m_settings.password.empty()) {
        MessageBoxW(m_hWnd, L"请填写完整的连接信息", L"测试连接", MB_OK | MB_ICONWARNING);
        return;
    }



    // 显示测试中状态
    SetWindowTextW(m_hTestBtn, L"测试中...");
    EnableWindow(m_hTestBtn, FALSE);

    // 在后台线程中测试连接
    std::thread([this]() {
        try {
            bool success = m_testConnectionCallback(m_settings);

            // 在主线程中显示结果
            PostMessageW(m_hWnd, WM_USER + 1, 0, 0);

            if (success) {
                PostMessageW(m_hWnd, WM_USER + 3, 0, 0);
            }
            else {
                PostMessageW(m_hWnd, WM_USER + 2, 0, 0);
            }
        }
        catch (...) {
            PostMessageW(m_hWnd, WM_USER + 4, 0, 0);
        }
        }).detach();
}

void SettingsDialog::OnShowPassword() {
    LRESULT checked = SendMessageW(m_hShowPwdCheck, BM_GETCHECK, 0, 0);
    SendMessageW(m_hPwdEdit, EM_SETPASSWORDCHAR, (WPARAM)(checked ? 0 : L'*'), 0);
    InvalidateRect(m_hPwdEdit, nullptr, TRUE);
}

void SettingsDialog::OnOk() {
    SaveSettings();
    m_modalResult = true;

    if (m_settingsChangedCallback) {
        m_settingsChangedCallback(m_settings);
    }

    DestroyWindow(m_hWnd);
}

void SettingsDialog::OnCancel() {
    m_modalResult = false;
    DestroyWindow(m_hWnd);
}

std::wstring SettingsDialog::Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    if (size <= 0) return std::wstring();
    std::wstring out((size_t)size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), out.data(), size);
    return out;
}

std::string SettingsDialog::WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return std::string();
    std::string out((size_t)size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), out.data(), size, nullptr, nullptr);
    return out;
}