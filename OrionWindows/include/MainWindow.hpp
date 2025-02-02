#pragma once

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

struct SearchResult {
    std::wstring path;
    std::wstring id;
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Initialize();
    winrt::Microsoft::UI::Xaml::Window& GetWindow() { return m_window; }

private:
    // UI Elements
    winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::StackPanel m_rootPanel{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBox m_searchQueryBox{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBox m_extensionBox{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBox m_pathBox{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button m_searchButton{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button m_cancelButton{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button m_browseButton{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ProgressBar m_progressBar{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ListView m_resultsList{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock m_errorText{ nullptr };

    // Search state
    std::atomic<bool> m_shouldCancel;
    std::thread m_searchThread;
    std::vector<SearchResult> m_searchResults;
    double m_searchProgress;
    bool m_isSearching;

    // UI Event handlers
    void OnSearchButtonClick(winrt::Windows::Foundation::IInspectable const& sender,
                           winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void OnCancelButtonClick(winrt::Windows::Foundation::IInspectable const& sender,
                           winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void OnBrowseButtonClick(winrt::Windows::Foundation::IInspectable const& sender,
                           winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void OnResultDoubleClick(winrt::Windows::Foundation::IInspectable const& sender,
                           winrt::Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& args);

    // Search methods
    void StartSearch();
    void CancelSearch();
    void PerformSearch(const std::wstring& query, const std::wstring& directory, const std::wstring& extension);
    void UpdateProgress(double progress);
    void UpdateSearchResults(const std::vector<SearchResult>& results);
    void ShowError(const std::wstring& message);

    // Helper methods
    void CreateUIElements();
    void SetupLayout();
    void RegisterEventHandlers();
}; 