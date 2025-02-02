#include "../include/MainWindow.hpp"
#include <shobjidl.h> 
#include <filesystem>
#include <shlwapi.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <uxtheme.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#define IDC_SEARCH_QUERY 101
#define IDC_EXTENSION 102
#define IDC_PATH 103
#define IDC_SEARCH_BUTTON 104
#define IDC_CANCEL_BUTTON 105
#define IDC_BROWSE_BUTTON 106
#define IDC_PROGRESS 107
#define IDC_RESULTS 108
#define IDC_ERROR 109

MainWindow::MainWindow() : 
    m_hwnd(nullptr),
    m_shouldCancel(false),
    m_searchProgress(0.0),
    m_isSearching(false) {
}

MainWindow::~MainWindow() {
    CancelSearch();
    if (m_searchThread.joinable()) {
        m_searchThread.join();
    }
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    } else {
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool MainWindow::Create() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"OrionMainWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        return false;
    }

    HDC hdc = GetDC(nullptr);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    float dpiScale = dpiX / 96.0f;

    int windowWidth = static_cast<int>(800 * dpiScale);
    int windowHeight = static_cast<int>(600 * dpiScale);

    RECT windowRect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"Orion File Search - Windows",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_hwnd) {
        return false;
    }

    BOOL isDarkMode = FALSE;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        if (RegQueryValueEx(hKey, L"AppsUseDarkTheme", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            isDarkMode = value != 0;
        }
        RegCloseKey(hKey);
    }

    if (isDarkMode) {
        BOOL value = TRUE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
        SetWindowTheme(m_hwnd, L"DarkMode_Explorer", nullptr);
    }

    CreateControls();
    return true;
}

void MainWindow::CreateControls() {
    HDC hdc = GetDC(m_hwnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(m_hwnd, hdc);
    float dpiScale = dpiX / 96.0f;

    int margin = static_cast<int>(10 * dpiScale);
    int controlHeight = static_cast<int>(25 * dpiScale);
    int buttonWidth = static_cast<int>(80 * dpiScale);

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;

    m_searchQueryEdit = CreateWindowEx(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        margin, margin, 
        static_cast<int>(300 * dpiScale), controlHeight,
        m_hwnd, (HMENU)IDC_SEARCH_QUERY,
        GetModuleHandle(nullptr), nullptr
    );

    m_extensionEdit = CreateWindowEx(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        margin + static_cast<int>(310 * dpiScale), margin,
        static_cast<int>(100 * dpiScale), controlHeight,
        m_hwnd, (HMENU)IDC_EXTENSION,
        GetModuleHandle(nullptr), nullptr
    );

    m_pathEdit = CreateWindowEx(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        margin, margin + controlHeight + 5, 
        static_cast<int>(500 * dpiScale), controlHeight,
        m_hwnd, (HMENU)IDC_PATH,
        GetModuleHandle(nullptr), nullptr
    );

    m_searchButton = CreateWindow(
        L"BUTTON", L"Search",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + static_cast<int>(320 * dpiScale), margin + 5, 
        buttonWidth, controlHeight,
        m_hwnd, (HMENU)IDC_SEARCH_BUTTON,
        GetModuleHandle(nullptr), nullptr
    );

    m_cancelButton = CreateWindow(
        L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + static_cast<int>(410 * dpiScale), margin + 5, 
        buttonWidth, controlHeight,
        m_hwnd, (HMENU)IDC_CANCEL_BUTTON,
        GetModuleHandle(nullptr), nullptr
    );
    EnableWindow(m_cancelButton, FALSE);

    m_browseButton = CreateWindow(
        L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + static_cast<int>(320 * dpiScale), margin + controlHeight + 5, 
        buttonWidth, controlHeight,
        m_hwnd, (HMENU)IDC_BROWSE_BUTTON,
        GetModuleHandle(nullptr), nullptr
    );

    m_progressBar = CreateWindowEx(
        0, PROGRESS_CLASS, nullptr,
        WS_CHILD | WS_VISIBLE,
        margin, margin + 2 * controlHeight + 10, 
        static_cast<int>(680 * dpiScale), controlHeight,
        m_hwnd, (HMENU)IDC_PROGRESS,
        GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    m_resultsList = CreateWindowEx(
        0, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_BORDER,
        margin, margin + 3 * controlHeight + 15, 
        static_cast<int>(680 * dpiScale), static_cast<int>(440 * dpiScale),
        m_hwnd, (HMENU)IDC_RESULTS,
        GetModuleHandle(nullptr), nullptr
    );

    m_errorText = CreateWindowEx(
        0, L"STATIC", nullptr,
        WS_CHILD | SS_LEFT,
        margin, margin + 4 * controlHeight + 550, 
        static_cast<int>(680 * dpiScale), controlHeight,
        m_hwnd, (HMENU)IDC_ERROR,
        GetModuleHandle(nullptr), nullptr
    );

    HFONT hFont = CreateFont(
        -static_cast<int>(14 * dpiScale), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    SendMessage(m_searchQueryEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_extensionEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_pathEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_searchButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_cancelButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_browseButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_resultsList, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_errorText, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessage(m_searchQueryEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Enter search query...");
    SendMessage(m_extensionEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"File extension...");
    SendMessage(m_pathEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search path...");

    wchar_t currentPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentPath);
    SetWindowText(m_pathEdit, currentPath);
}

void MainWindow::SetupLayout() {}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_USER + 1:
            SendMessage(m_progressBar, PBM_SETPOS, wParam, 0);
            return 0;

        case WM_USER + 2:
            ShowError(reinterpret_cast<const wchar_t*>(lParam));
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SEARCH_BUTTON:
                    OnSearchButtonClick();
                    return 0;
                case IDC_CANCEL_BUTTON:
                    OnCancelButtonClick();
                    return 0;
                case IDC_BROWSE_BUTTON:
                    OnBrowseButtonClick();
                    return 0;
                case IDC_RESULTS:
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        int index = SendMessage(m_resultsList, LB_GETCURSEL, 0, 0);
                        if (index != LB_ERR) {
                            OnResultDoubleClick(index);
                        }
                    }
                    return 0;
            }
            break;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            HWND hwnd = (HWND)lParam;
            
            BOOL isDarkMode = FALSE;
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD value = 0;
                DWORD size = sizeof(value);
                if (RegQueryValueEx(hKey, L"AppsUseDarkTheme", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
                    isDarkMode = value != 0;
                }
                RegCloseKey(hKey);
            }

            if (isDarkMode) {
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(32, 32, 32));
                static HBRUSH hBrush = CreateSolidBrush(RGB(32, 32, 32));
                return (LRESULT)hBrush;
            }
            else {
                SetTextColor(hdc, RGB(0, 0, 0));
                SetBkColor(hdc, RGB(255, 255, 255));
                return (LRESULT)GetStockObject(WHITE_BRUSH);
            }
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::OnSearchButtonClick() {
    StartSearch();
}

void MainWindow::OnCancelButtonClick() {
    CancelSearch();
}

void MainWindow::OnBrowseButtonClick() {
    IFileOpenDialog* pFileDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                                 IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileDialog));

    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        hr = pFileDialog->GetOptions(&dwOptions);
        if (SUCCEEDED(hr)) {
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
            hr = pFileDialog->Show(nullptr);
            if (SUCCEEDED(hr)) {
                IShellItem* pItem;
                hr = pFileDialog->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr)) {
                        SetWindowText(m_pathEdit, pszFilePath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
        }
        pFileDialog->Release();
    }
}

void MainWindow::OnResultDoubleClick(int index) {
    if (index >= 0 && index < static_cast<int>(m_searchResults.size())) {
        ShellExecute(nullptr, L"explore", m_searchResults[index].path.c_str(),
                    nullptr, nullptr, SW_SHOW);
    }
}

void MainWindow::StartSearch() {
    wchar_t queryBuffer[256];
    wchar_t pathBuffer[MAX_PATH];
    wchar_t extensionBuffer[32];

    GetWindowText(m_searchQueryEdit, queryBuffer, 256);
    GetWindowText(m_pathEdit, pathBuffer, MAX_PATH);
    GetWindowText(m_extensionEdit, extensionBuffer, 32);

    if (wcslen(queryBuffer) == 0) {
        MessageBox(m_hwnd, L"Please enter a search query", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    CancelSearch();
    if (m_searchThread.joinable()) {
        m_searchThread.join();
    }

    m_shouldCancel = false;
    m_isSearching = true;
    SendMessage(m_resultsList, LB_RESETCONTENT, 0, 0);
    EnableWindow(m_searchButton, FALSE);
    EnableWindow(m_cancelButton, TRUE);
    EnableWindow(m_searchQueryEdit, FALSE);
    EnableWindow(m_pathEdit, FALSE);
    EnableWindow(m_extensionEdit, FALSE);
    ShowWindow(m_errorText, SW_HIDE);

    m_searchThread = std::thread([this, query = std::wstring(queryBuffer),
                                path = std::wstring(pathBuffer),
                                extension = std::wstring(extensionBuffer)]() {
        PerformSearch(query, path, extension);
    });
}

void MainWindow::CancelSearch() {
    if (m_isSearching) {
        m_shouldCancel = true;
        if (m_searchThread.joinable()) {
            m_searchThread.join();
        }
        m_isSearching = false;
        EnableWindow(m_searchButton, TRUE);
        EnableWindow(m_cancelButton, FALSE);
        EnableWindow(m_searchQueryEdit, TRUE);
        EnableWindow(m_pathEdit, TRUE);
        EnableWindow(m_extensionEdit, TRUE);
        SendMessage(m_progressBar, PBM_SETPOS, 0, 0);
    }
}

void MainWindow::UpdateProgress(double progress) {
    SendMessage(m_progressBar, PBM_SETPOS, static_cast<int>(progress * 100), 0);
}

void MainWindow::PerformSearch(const std::wstring& query, const std::wstring& directory,
                             const std::wstring& extension) {
    std::vector<SearchResult> results;
    std::error_code ec;

    try {
        auto dirIter = std::filesystem::recursive_directory_iterator(
            directory,
            std::filesystem::directory_options::skip_permission_denied
        );

        std::vector<std::filesystem::path> allFiles;
        for (const auto& entry : dirIter) {
            if (m_shouldCancel) return;
            
            std::error_code ec;
            if (entry.is_regular_file(ec)) {
                allFiles.push_back(entry.path());
            }
        }

        size_t totalFiles = allFiles.size();
        size_t processedFiles = 0;

        for (const auto& file : allFiles) {
            if (m_shouldCancel) return;

            try {
                if (!extension.empty()) {
                    std::wstring fileExt = file.extension().wstring();
                    std::wstring searchExt = extension[0] == L'.' ? extension : L"." + extension;
                    if (_wcsicmp(fileExt.c_str(), searchExt.c_str()) != 0) {
                        processedFiles++;
                        double progress = static_cast<double>(processedFiles) / totalFiles;
                        PostMessage(m_hwnd, WM_USER + 1, static_cast<WPARAM>(progress * 100), 0);
                        continue;
                    }
                }

                std::wstring filename = file.filename().wstring();
                std::wstring lowerFilename;
                std::wstring lowerQuery;
                lowerFilename.resize(filename.size());
                lowerQuery.resize(query.size());
                std::transform(filename.begin(), filename.end(), lowerFilename.begin(), ::towlower);
                std::transform(query.begin(), query.end(), lowerQuery.begin(), ::towlower);

                if (lowerFilename.find(lowerQuery) != std::wstring::npos) {
                    SearchResult result;
                    result.path = file.wstring();
                    result.id = std::to_wstring(results.size());
                    results.push_back(result);

                    SendMessage(m_resultsList, LB_ADDSTRING, 0,
                            reinterpret_cast<LPARAM>(result.path.c_str()));
                }
            }
            catch (const std::exception&) {
                continue;
            }

            processedFiles++;
            double progress = static_cast<double>(processedFiles) / totalFiles;
            PostMessage(m_hwnd, WM_USER + 1, static_cast<WPARAM>(progress * 100), 0);
        }
    }
    catch (const std::exception& e) {
        PostMessage(m_hwnd, WM_USER + 2, 0,
                   reinterpret_cast<LPARAM>(L"Error accessing directory"));
        return;
    }

    m_searchResults = results;
    PostMessage(m_hwnd, WM_COMMAND, IDC_SEARCH_BUTTON, 0);
}

void MainWindow::ShowError(const std::wstring& message) {
    SetWindowText(m_errorText, message.c_str());
    ShowWindow(m_errorText, SW_SHOW);
}
