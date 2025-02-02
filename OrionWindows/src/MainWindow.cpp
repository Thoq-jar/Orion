#include "../include/MainWindow.hpp"
#include <shobjidl.h> 
#include <filesystem>
#include <shlwapi.h>
#include <shellapi.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;

MainWindow::MainWindow() : 
    m_shouldCancel(false),
    m_searchProgress(0.0),
    m_isSearching(false) {
    Initialize();
}

MainWindow::~MainWindow() {
    CancelSearch();
    if (m_searchThread.joinable()) {
        m_searchThread.join();
    }
}

void MainWindow::Initialize() {
    m_window = Window();
    m_window.Title(L"Orion File Search - Windows");

    CreateUIElements();
    SetupLayout();
    RegisterEventHandlers();
}

void MainWindow::CreateUIElements() {
    m_rootPanel = StackPanel();
    m_rootPanel.Padding(Thickness{20, 20, 20, 20});
    m_rootPanel.Spacing(16);

    auto searchPanel = StackPanel();
    searchPanel.Orientation(Orientation::Horizontal);
    searchPanel.Spacing(8);

    m_searchQueryBox = TextBox();
    m_searchQueryBox.PlaceholderText(L"Search query");
    m_searchQueryBox.Width(300);

    m_extensionBox = TextBox();
    m_extensionBox.PlaceholderText(L"Extension (e.g., txt)");
    m_extensionBox.Width(150);

    m_browseButton = Button();
    m_browseButton.Content(box_value(L"Browse"));

    searchPanel.Children().Append(m_searchQueryBox);
    searchPanel.Children().Append(m_extensionBox);
    searchPanel.Children().Append(m_browseButton);

    m_pathBox = TextBox();
    m_pathBox.PlaceholderText(L"Search path");
    m_pathBox.Text(std::filesystem::current_path().wstring());

    auto progressPanel = StackPanel();
    progressPanel.Orientation(Orientation::Horizontal);
    progressPanel.Spacing(8);

    m_progressBar = ProgressBar();
    m_progressBar.Width(400);
    m_progressBar.Visibility(Visibility::Collapsed);

    m_searchButton = Button();
    m_searchButton.Content(box_value(L"Search"));

    m_cancelButton = Button();
    m_cancelButton.Content(box_value(L"Cancel"));
    m_cancelButton.Visibility(Visibility::Collapsed);

    progressPanel.Children().Append(m_progressBar);
    progressPanel.Children().Append(m_searchButton);
    progressPanel.Children().Append(m_cancelButton);

    m_errorText = TextBlock();
    m_errorText.Foreground(SolidColorBrush(Windows::UI::Colors::Red()));
    m_errorText.Visibility(Visibility::Collapsed);

    m_resultsList = ListView();
    m_resultsList.Height(400);
    m_resultsList.BorderThickness(Thickness{1});
    m_resultsList.BorderBrush(SolidColorBrush(Windows::UI::Colors::Gray()));

    m_rootPanel.Children().Append(searchPanel);
    m_rootPanel.Children().Append(m_pathBox);
    m_rootPanel.Children().Append(progressPanel);
    m_rootPanel.Children().Append(m_errorText);
    m_rootPanel.Children().Append(m_resultsList);

    m_window.Content(m_rootPanel);
}

void MainWindow::SetupLayout() {
    m_window.Activate();
    m_window.MinWidth(800);
    m_window.MinHeight(600);
}

void MainWindow::RegisterEventHandlers() {
    m_searchButton.Click([this](auto&&, auto&&) { OnSearchButtonClick(nullptr, nullptr); });
    m_cancelButton.Click([this](auto&&, auto&&) { OnCancelButtonClick(nullptr, nullptr); });
    m_browseButton.Click([this](auto&&, auto&&) { OnBrowseButtonClick(nullptr, nullptr); });
    m_resultsList.DoubleTapped([this](auto&&, auto&& e) { OnResultDoubleClick(nullptr, e); });
}

void MainWindow::OnSearchButtonClick(IInspectable const&, RoutedEventArgs const&) {
    StartSearch();
}

void MainWindow::OnCancelButtonClick(IInspectable const&, RoutedEventArgs const&) {
    CancelSearch();
}

void MainWindow::OnBrowseButtonClick(IInspectable const&, RoutedEventArgs const&) {
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
                        m_pathBox.Text(pszFilePath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
        }
        pFileDialog->Release();
    }
}

void MainWindow::OnResultDoubleClick(IInspectable const&, Input::DoubleTappedRoutedEventArgs const&) {
    if (auto selected = m_resultsList.SelectedItem()) {
        if (auto result = selected.try_as<PropertyValue>()) {
            std::wstring path = result.GetString();
            ShellExecute(nullptr, L"explore", path.c_str(), nullptr, nullptr, SW_SHOW);
        }
    }
}

void MainWindow::StartSearch() {
    CancelSearch();
    m_shouldCancel = false;
    m_searchProgress = 0;
    m_isSearching = true;
    m_errorText.Visibility(Visibility::Collapsed);
    m_progressBar.Visibility(Visibility::Visible);
    m_searchButton.Visibility(Visibility::Collapsed);
    m_cancelButton.Visibility(Visibility::Visible);
    m_resultsList.Items().Clear();

    std::wstring query = m_searchQueryBox.Text();
    std::wstring path = m_pathBox.Text();
    std::wstring extension = m_extensionBox.Text();

    m_searchThread = std::thread([this, query, path, extension]() {
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
        m_progressBar.Visibility(Visibility::Collapsed);
        m_searchButton.Visibility(Visibility::Visible);
        m_cancelButton.Visibility(Visibility::Collapsed);
    }
}

void MainWindow::PerformSearch(const std::wstring& query, const std::wstring& directory, const std::wstring& extension) {
    std::vector<SearchResult> results;
    std::error_code ec;
    auto dirIter = std::filesystem::recursive_directory_iterator(directory, ec);
    if (ec) {
        m_window.DispatcherQueue().TryEnqueue([this, ec]() {
            ShowError(L"Error accessing directory: " + std::wstring(ec.message().begin(), ec.message().end()));
        });
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
        std::transform(filename.begin(), filename.end(), std::back_inserter(lowerFilename), ::towlower);
        std::transform(query.begin(), query.end(), std::back_inserter(lowerQuery), ::towlower);

        if (lowerFilename.find(lowerQuery) != std::wstring::npos) {
            SearchResult result;
            result.path = file.wstring();
            result.id = std::to_wstring(results.size());
            results.push_back(result);
        }

        processedFiles++;
        UpdateProgress(static_cast<double>(processedFiles) / allFiles.size());
    }

    m_window.DispatcherQueue().TryEnqueue([this, results]() {
        UpdateSearchResults(results);
    });
}

void MainWindow::UpdateProgress(double progress) {
    m_window.DispatcherQueue().TryEnqueue([this, progress]() {
        m_progressBar.Value(progress * 100);
    });
}

void MainWindow::UpdateSearchResults(const std::vector<SearchResult>& results) {
    m_searchResults = results;
    m_resultsList.Items().Clear();

    for (const auto& result : results) {
        m_resultsList.Items().Append(box_value(result.path));
    }

    m_isSearching = false;
    m_progressBar.Visibility(Visibility::Collapsed);
    m_searchButton.Visibility(Visibility::Visible);
    m_cancelButton.Visibility(Visibility::Collapsed);
}

void MainWindow::ShowError(const std::wstring& message) {
    m_errorText.Text(message);
    m_errorText.Visibility(Visibility::Visible);
    m_isSearching = false;
    m_progressBar.Visibility(Visibility::Collapsed);
    m_searchButton.Visibility(Visibility::Visible);
    m_cancelButton.Visibility(Visibility::Collapsed);
}
