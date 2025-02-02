#include "../include/MainWindow.hpp"
#include <shobjidl.h> 
#include <filesystem>
#include <shlwapi.h>
#include <shellapi.h>

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

    if (!RegisterClassEx(&wc)) {
        return false;
    }

    RECT windowRect = {0, 0, 800, 600};
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

    CreateControls();
    SetupLayout();
    return true;
}

void MainWindow::CreateControls() {
    m_searchQueryEdit = CreateWindowEx(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 10, 300, 25,
        m_hwnd, (HMENU)IDC_SEARCH_QUERY,
        GetModuleHandle(nullptr), nullptr
    );

    m_extensionEdit = CreateWindowEx(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        320, 10, 100, 25,
        m_hwnd, (HMENU)IDC_EXTENSION,
        GetModuleHandle(nullptr), nullptr
    );

    m_pathEdit = CreateWindowEx(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 45, 500, 25,
        m_hwnd, (HMENU)IDC_PATH,
        GetModuleHandle(nullptr), nullptr
    );

    wchar_t currentPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentPath);
    SetWindowText(m_pathEdit, currentPath);

    m_searchButton = CreateWindow(
        L"BUTTON", L"Search",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        520, 10, 80, 25,
        m_hwnd, (HMENU)IDC_SEARCH_BUTTON,
        GetModuleHandle(nullptr), nullptr
    );

    m_cancelButton = CreateWindow(
        L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        610, 10, 80, 25,
        m_hwnd, (HMENU)IDC_CANCEL_BUTTON,
        GetModuleHandle(nullptr), nullptr
    );
    EnableWindow(m_cancelButton, FALSE);

    m_browseButton = CreateWindow(
        L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        520, 45, 80, 25,
        m_hwnd, (HMENU)IDC_BROWSE_BUTTON,
        GetModuleHandle(nullptr), nullptr
    );

    m_progressBar = CreateWindowEx(
        0, PROGRESS_CLASS, nullptr,
        WS_CHILD | WS_VISIBLE,
        10, 80, 680, 20,
        m_hwnd, (HMENU)IDC_PROGRESS,
        GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    m_resultsList = CreateWindowEx(
        0, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_BORDER,
        10, 110, 680, 440,
        m_hwnd, (HMENU)IDC_RESULTS,
        GetModuleHandle(nullptr), nullptr
    );

    m_errorText = CreateWindowEx(
        0, L"STATIC", nullptr,
        WS_CHILD | SS_LEFT,
        10, 560, 680, 20,
        m_hwnd, (HMENU)IDC_ERROR,
        GetModuleHandle(nullptr), nullptr
    );
}

void MainWindow::SetupLayout() {}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
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
    auto dirIter = std::filesystem::recursive_directory_iterator(directory, ec);

    if (ec) {
        PostMessage(m_hwnd, WM_COMMAND, IDC_ERROR,
                   reinterpret_cast<LPARAM>(L"Error accessing directory"));
        return;
    }

    std::vector<std::filesystem::path> allFiles;
    for (const auto& entry : dirIter) {
        if (m_shouldCancel) return;
        if (entry.is_regular_file()) {
            allFiles.push_back(entry.path());
        }
    }

    size_t processedFiles = 0;
    for (const auto& file : allFiles) {
        if (m_shouldCancel) return;

        if (!extension.empty()) {
            std::wstring fileExt = file.extension().wstring();
            std::wstring searchExt = extension[0] == L'.' ? extension : L"." + extension;
            if (_wcsicmp(fileExt.c_str(), searchExt.c_str()) != 0) {
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

        processedFiles++;
        UpdateProgress(static_cast<double>(processedFiles) / allFiles.size());
    }

    m_searchResults = results;
    PostMessage(m_hwnd, WM_COMMAND, IDC_SEARCH_BUTTON, 0);
}

void MainWindow::ShowError(const std::wstring& message) {
    SetWindowText(m_errorText, message.c_str());
    ShowWindow(m_errorText, SW_SHOW);
}

