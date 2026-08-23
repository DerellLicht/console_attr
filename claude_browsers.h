bool BrowseForFile(HWND hwndOwner, std::wstring& outPath,
                    const COMDLG_FILTERSPEC* filters = nullptr,
                    UINT filterCount = 0);
bool BrowseForFolder(HWND hwndOwner, std::wstring& outPath);
bool BrowseForFileSave(HWND hwndOwner, std::wstring& outPath,
                        const COMDLG_FILTERSPEC* filters = nullptr,
                        UINT filterCount = 0,
                        const wchar_t* defaultName = nullptr);
std::string WideToNarrow(const std::wstring& w);
                        
