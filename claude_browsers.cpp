#define INITGUID
#include <windows.h>
#include <cstdio>   //  sprintf, needed for double
#include <cstdlib>  //  _MAX_PATH
#include <sys/stat.h>
#include <string>
#include <shlobj.h>
#include <shobjidl.h>   // IFileDialog, IShellItem

#include "common.h"
#include "commonw.h"
#include "claude_browsers.h"

/*  examples of filters for Claude Browsers
// static TCHAR szPalFilter[]  = TEXT ("Palette Files (*.PLT)\0*.plt\0")  \
//                               TEXT ("All Files (*.*)\0*.*\0\0") ;
// static TCHAR szExecFilter[] = TEXT ("Executable Files (*.EXE)\0*.exe\0")  \
//                               TEXT ("All Files (*.*)\0*.*\0\0") ;
// static TCHAR szDirFilter[]  = TEXT ("All Files (*.*)\0*.*\0\0") ;

const COMDLG_FILTERSPEC PalFilter[] = {
 { L"Palette Files (*.plt)", L"*.plt" },
 { L"All files (*.*)",       L"*.*"   }
};

const COMDLG_FILTERSPEC ExecFilter[] = {
 { L"Executable Files (*.exe)", L"*.exe" },
 { L"All files (*.*)",          L"*.*"   }
};

*/

//************************************************************************
// Opens the modern file-picker dialog (IFileDialog, no FOS_PICKFOLDERS).
// hwndOwner may be nullptr. filters/filterCount describe the file-type
// dropdown (pass nullptr/0 for "all files"). On success, writes the chosen
// file path into outPath and returns true. On cancel or failure, outPath
// is left untouched and the function returns false. Handles COM init/
// uninit internally, same as BrowseForFolder().
//************************************************************************
bool BrowseForFile(HWND hwndOwner, std::wstring& outPath,
                    const COMDLG_FILTERSPEC* filters,
                    UINT filterCount)
{
    bool weInitializedCom = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (hr == S_OK || hr == S_FALSE) {
        weInitializedCom = true;
    }
    else if (hr != RPC_E_CHANGED_MODE) {
        return false;
    }

    bool result = false;
    IFileDialog* pfd = nullptr;

    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                           IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        if (filters != nullptr && filterCount > 0) {
            pfd->SetFileTypes(filterCount, filters);
            pfd->SetFileTypeIndex(1); // 1-based; first filter selected by default
        }

        hr = pfd->Show(hwndOwner);
        if (SUCCEEDED(hr)) {
            IShellItem* psi = nullptr;
            hr = pfd->GetResult(&psi);
            if (SUCCEEDED(hr)) {
                PWSTR pszPath = nullptr;
                hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr)) {
                    outPath = pszPath;
                    CoTaskMemFree(pszPath);
                    result = true;
                }
                psi->Release();
            }
        }
        // hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) just means the user hit Cancel.

        pfd->Release();
    }

    if (weInitializedCom) {
        CoUninitialize();
    }

    return result;
}

//************************************************************************
// Opens the modern folder-picker dialog (IFileDialog + FOS_PICKFOLDERS).
// hwndOwner may be nullptr. On success, writes the chosen folder path into
// outPath and returns true. On cancel or failure, outPath is left untouched
// and the function returns false. Handles COM init/uninit internally, so
// it's safe to call even if the caller hasn't already called CoInitialize.
//************************************************************************
bool BrowseForFolder(HWND hwndOwner, std::wstring& outPath)
{
    bool weInitializedCom = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (hr == S_OK || hr == S_FALSE) {
        weInitializedCom = true;
    }
    else if (hr != RPC_E_CHANGED_MODE) {
        // Some other real failure initializing COM.
        return false;
    }

    bool result = false;
    IFileDialog* pfd = nullptr;

    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        DWORD options = 0;
        hr = pfd->GetOptions(&options);
        if (SUCCEEDED(hr)) {
            hr = pfd->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }

        if (SUCCEEDED(hr)) {
            hr = pfd->Show(hwndOwner);
            if (SUCCEEDED(hr)) {
                IShellItem* psi = nullptr;
                hr = pfd->GetResult(&psi);
                if (SUCCEEDED(hr)) {
                    PWSTR pszPath = nullptr;
                    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr)) {
                        outPath = pszPath;
                        CoTaskMemFree(pszPath);
                        result = true;
                    }
                    psi->Release();
                }
            }
            // hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) just means the user hit Cancel.
        }

        pfd->Release();
    }

    if (weInitializedCom) {
        CoUninitialize();
    }

    return result;
}

//************************************************************************
// Opens the modern save-file dialog (IFileDialog via CLSID_FileSaveDialog).
// hwndOwner may be nullptr. filters/filterCount describe the file-type
// dropdown (pass nullptr/0 for "all files"). defaultName, if non-null,
// pre-fills the filename edit box (e.g. L"untitled.cfg"). Unlike
// BrowseForFile(), the returned path need not already exist -- that's
// the point. On success, writes the path into outPath and returns true.
// On cancel or failure, outPath is left untouched and the function
// returns false. Handles COM init/uninit internally, same as the others.
//************************************************************************
bool BrowseForFileSave(HWND hwndOwner, std::wstring& outPath,
                        const COMDLG_FILTERSPEC* filters,
                        UINT filterCount,
                        const wchar_t* defaultName)
{
    bool weInitializedCom = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (hr == S_OK || hr == S_FALSE) {
        weInitializedCom = true;
    }
    else if (hr != RPC_E_CHANGED_MODE) {
        return false;
    }

    bool result = false;
    IFileDialog* pfd = nullptr;

    hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        if (filters != nullptr && filterCount > 0) {
            pfd->SetFileTypes(filterCount, filters);
            pfd->SetFileTypeIndex(1); // 1-based; first filter selected by default
        }

        if (defaultName != nullptr) {
            pfd->SetFileName(defaultName);
        }

        hr = pfd->Show(hwndOwner);
        if (SUCCEEDED(hr)) {
            IShellItem* psi = nullptr;
            hr = pfd->GetResult(&psi);
            if (SUCCEEDED(hr)) {
                PWSTR pszPath = nullptr;
                hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr)) {
                    outPath = pszPath;
                    CoTaskMemFree(pszPath);
                    result = true;
                }
                psi->Release();
            }
        }
        // hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) just means the user hit Cancel.

        pfd->Release();
    }

    if (weInitializedCom) {
        CoUninitialize();
    }

    return result;
}

//**************************************************************************
//  Claude: convert the wchar_t file/folder names to char strings 
//**************************************************************************
std::string WideToNarrow(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(),
                                   nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(),
                         s.data(), len, nullptr, nullptr);
    return s;
}

//**************************************************************************
//  Claude: example usage for BrowseForFile()
//**************************************************************************
#define  HIDE_EXAMPLE_CODE

#ifndef  HIDE_EXAMPLE_CODE
void file_search_usage_examples(HWND hwndMain)
{
   // return select_starting_directory(hwndMain);
   std::wstring folder;
   if (BrowseForFolder(hwndMain, folder)) {
      std::string folderA = WideToNarrow(folder);
      //  do something with the selected string
      // strncpy(starting_path, folderA.c_str(), sizeof(starting_path)) ;
   }

   //  usage, no filters (all files):
   std::wstring file;
   if (BrowseForFile(hwndMain, file)) {
      // use file
   }
   
   // Usage, with a filter (e.g. your program's own config/data files):
   const COMDLG_FILTERSPEC cfilters[] = {
      { L"Config files (*.cfg)", L"*.cfg" },
      { L"All files (*.*)",      L"*.*"   }
   };

   std::wstring file2;
   if (BrowseForFile(hwndMain, file2, cfilters, ARRAYSIZE(cfilters))) {
      // use file
   }
   
   // examples for 
   const COMDLG_FILTERSPEC sfilters[] = {
      { L"Config files (*.cfg)", L"*.cfg" },
      { L"All files (*.*)",      L"*.*"   }
   };

   std::wstring file3;
   if (BrowseForFileSave(hwndMain, file3, sfilters, ARRAYSIZE(sfilters), L"untitled.cfg")) {
      // use file — may not exist on disk yet
   }   
}
#endif

