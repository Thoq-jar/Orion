#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "bridge.h"

struct SearchResult {
    std::wstring path;
    std::wstring id;
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create();
    HWND GetHandle() const { return m_hwnd; }
    void Show() { ShowWindow(m_hwnd, SW_SHOW); }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CreateControls();
    void SetupLayout();

    // Window handles
    HWND m_hwnd;
    HWND m_searchQueryEdit;
    HWND m_extensionEdit;
    HWND m_pathEdit;
    HWND m_searchButton;
    HWND m_cancelButton;
    HWND m_browseButton;
    HWND m_progressBar;
    HWND m_resultsList;
    HWND m_errorText;
    HWND m_statusLabel;

    // Search state
    std::atomic<bool> m_shouldCancel;
    std::thread m_searchThread;
    std::vector<SearchResult> m_searchResults;
    double m_searchProgress;
    bool m_isSearching;

    // Event handlers
    void OnSearchButtonClick();
    void OnCancelButtonClick();
    void OnBrowseButtonClick();
    void OnResultDoubleClick(int index);

    // Search methods
    void StartSearch();
    void CancelSearch();
    void PerformSearch(const std::wstring& query, const std::wstring& directory, const std::wstring& extension);
    void UpdateProgress(double progress);
    void ShowError(const std::wstring& message);
}; 