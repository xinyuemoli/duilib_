#include "wv2_control.h"
#include "duilib/Core/Window.h"

namespace ui
{
    std::map<wv2_t, Wv2::EventHandle*> Wv2::g_handlers = {};

    static void OnHistroyChanged(wv2_t sender, bool canGoBack, bool canGoForward) {
        Wv2::g_handlers[sender]->HistoryChanged(canGoBack, canGoForward);
    }

    static bool OnNavigationStarting(wv2_t sender, LPCWSTR uri) {
        return Wv2::g_handlers[sender]->NavigationStarting(uri);
    }

    static void OnNavigationCompleted(wv2_t sender, wv2navigationCompletedEventArgs_t args)
    {
        Wv2::g_handlers[sender]->NavigationCompleted(args);
    }

    static void OnDomContentLoaded(wv2_t sender, wv2domContentLoadedEventArgs_t args) {
        Wv2::g_handlers[sender]->DomContentLoaded(args);
    }

    static bool OnWindowCloseRequested(wv2_t sender) {
        return Wv2::g_handlers[sender]->WindowCloseRequested();
    }

    static void OnWebMessageReceived(wv2_t sender, LPCWSTR message) {
        Wv2::g_handlers[sender]->WebMessageReceived(message);
    }

    static void OnIsMutedChanged(wv2_t sender){
        Wv2::g_handlers[sender]->IsMutedChanged();
    }

    static void OnIsDocumentPlayingAudioChanged(wv2_t sender) {
        Wv2::g_handlers[sender]->IsDocumentPlayingAudioChanged();
    }

    static void OnNewWindowRequested(wv2_t sender, wv2newWindowRequestedEventArgs_t args) {
        Wv2::g_handlers[sender]->NewWindowRequested(args);
    }

    static void OnDocumentTitleChanged(wv2_t sender) {
        Wv2::g_handlers[sender]->DocumentTitleChanged();
    }

    static void OnContentLoading(wv2_t sender, wv2contentLoadingEventArgs_t args) {
        Wv2::g_handlers[sender]->ContentLoading(args);
    }

    static void OnScriptDialogOpening(wv2_t sender, wv2scriptDialogOpeningEventArgs_t args) {
        Wv2::g_handlers[sender]->ScriptDialogOpening(args);
    }

    static void OnDownloadStarting(wv2_t sender, wv2downloadStartingEventArgs_t args) {
        Wv2::g_handlers[sender]->DownloadStarting(args);
    }

    static void OnWebResourceRequested(wv2_t sender, wv2webResourceRequestedEventArgs_t args) {
        Wv2::g_handlers[sender]->WebResourceRequested(args);
    }

    Wv2::Wv2(ui::Window* pWindow):Control(pWindow)
    {
        options_ = wv2envOptsCreate();
    }

    Wv2::~Wv2()
    {
        if (webview_)
        {
            wv2destroy(&webview_);
            webview_ = nullptr;
        }
    }
    void Wv2::SetHandler(EventHandle* handler)
    {
        delegate_ = handler;
    }
    void Wv2::Init()
    {
        __super::Init();
        //MyRegisterClass();
        ////创建宿主窗口
        //host_wnd_ = CreateWindowW(host_wnd_cls_name_.c_str(), nullptr, UI_WNDSTYLE_DIALOG,
        //    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, ::GetModuleHandle(NULL), this);
        webview_ = wv2createSync2(nullptr, nullptr, options_,GetWindow()->NativeWnd()->GetHWND());
        if (webview_)
        {
            //设置事件处理？
            RegisterHandler();
        }
    }
    void Wv2::SetPos(UiRect rc)
    {
        //这里的rc 是dpi 传唤后的rc？
        __super::SetPos(rc);
        ////设置窗口的位置
        //if (host_wnd_)
        //{
        //    //将窗口坐标值转换成屏幕坐标.
        //    POINT pt = { rc.left,rc.top };
        //    ClientToScreen(m_pWindow->GetHWND(), &pt);
        //    SetWindowPos(host_wnd_, 0, pt.x, pt.y, rc.GetWidth(), rc.GetHeight(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE |
        //        SWP_FRAMECHANGED);
        //}
        Resize(rc.left, rc.top, rc.Width(), rc.Height());
    }
    std::wstring Wv2::GetCurlUrl()
    {
        return wv2getSource(webview_);
    }
    bool Wv2::GoBack()
    {
        return wv2goBack(webview_);
    }
    bool Wv2::GoForward()
    {
        return wv2goForward(webview_);
    }
    bool Wv2::Navigate(const std::wstring& url)
    {
        return wv2navigate(webview_,url.c_str());
    }
    bool Wv2::Reload()
    {
        return wv2reload(webview_);
    }
    bool Wv2::Resize(int x, int y, int w, int h)
    {
        return wv2resize(webview_,x,y,w,h);
    }
    bool Wv2::PostJsonMessage(const std::wstring& json_msg)
    {
        return wv2postWebMessageAsJson(webview_,json_msg.c_str());
    }
    void Wv2::RegisterHandler()
    {
        if (delegate_)
        {
            g_handlers[webview_] = delegate_;

            wv2setHistoryChangedHandler(webview_, OnHistroyChanged);
            wv2setNavigationStartingHandler(webview_, OnNavigationStarting);
            wv2setNavigationCompletedHandler(webview_, OnNavigationCompleted);
            wv2setDomContentLoadedHandler(webview_, OnDomContentLoaded);
            wv2setWindowCloseRequestedHandler(webview_, OnWindowCloseRequested);
            wv2setWebMessageReceivedHandler(webview_, OnWebMessageReceived);
            wv2setIsMutedChangedHandler(webview_, OnIsMutedChanged);
            wv2setIsDocumentPlayingAudioChangedHandler(webview_, OnIsDocumentPlayingAudioChanged);
            wv2setNewWindowRequestedHandler(webview_, OnNewWindowRequested);
            wv2setDocumentTitleChangedHandler(webview_, OnDocumentTitleChanged);
            wv2setContentLoadingHandler(webview_, OnContentLoading);
            wv2setScriptDialogOpningHandler(webview_, OnScriptDialogOpening);
            wv2setDownloadStartingHandler(webview_, OnDownloadStarting);
            wv2setWebResourceRequestedHandler(webview_, OnWebResourceRequested);
        }
    }
    //ATOM Wv2::MyRegisterClass(HINSTANCE hInstance)
    //{
    //    WNDCLASSEXW wcex;

    //    wcex.cbSize = sizeof(WNDCLASSEX);

    //    wcex.style = UI_CLASSSTYLE_DIALOG/*CS_HREDRAW | CS_VREDRAW*/;
    //    wcex.lpfnWndProc = Wv2::__WndProc;
    //    wcex.cbClsExtra = 0;
    //    wcex.cbWndExtra = 0;
    //    wcex.hInstance = hInstance;
    //    wcex.hIcon = NULL;
    //    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    //    wcex.hbrBackground = NULL;
    //    wcex.lpszMenuName = NULL;
    //    wcex.lpszClassName = host_wnd_cls_name_.c_str();
    //    wcex.hIconSm = NULL;

    //    return RegisterClassExW(&wcex);
    //}
    //LRESULT Wv2::__WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    //{
    //    Wv2* pThis = NULL;
    //    if (uMsg == WM_NCCREATE)
    //    {
    //        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
    //        pThis = static_cast<Wv2*>(lpcs->lpCreateParams);
    //        pThis->host_wnd_ = hWnd;
    //        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pThis));
    //    }
    //    else
    //    {
    //        pThis = reinterpret_cast<Wv2*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    //        if (pThis)
    //        {
    //            switch (uMsg)
    //            {
    //            case WM_SIZE:
    //            {
    //                if (pThis->webview_)
    //                {
    //                    const int w = LOWORD(lParam);
    //                    const int h = HIWORD(lParam);
    //                    wv2resize(pThis->webview_, w, h);
    //                }
    //                break;
    //            }
    //            case WM_DESTROY: {
    //                if (pThis->webview_)
    //                {
    //                    wv2destroy(&pThis->webview_);
    //                }
    //                break;
    //            }
    //            }
    //        }
    //    }

    //------------------------------------------------事件处理-----------------------------------
    void Wv2::EventHandle::HistoryChanged(bool canGoBack, bool canGoForward)
    {
    }

    bool Wv2::EventHandle::NavigationStarting(LPCWSTR uri)
    {
        return true;
    }

    void Wv2::EventHandle::NavigationCompleted(wv2navigationCompletedEventArgs_t args)
    {
    }

    void Wv2::EventHandle::DomContentLoaded(wv2domContentLoadedEventArgs_t args)
    {
    }

    bool Wv2::EventHandle::WindowCloseRequested()
    {
        return true;
    }

    void Wv2::EventHandle::WebMessageReceived(LPCWSTR message)
    {
    }

    void Wv2::EventHandle::IsMutedChanged()
    {
    }

    void Wv2::EventHandle::IsDocumentPlayingAudioChanged()
    {
    }

    void Wv2::EventHandle::NewWindowRequested(wv2newWindowRequestedEventArgs_t args)
    {
    }

    void Wv2::EventHandle::DocumentTitleChanged()
    {
    }

    void Wv2::EventHandle::ContentLoading(wv2contentLoadingEventArgs_t args)
    {
    }

    void Wv2::EventHandle::ScriptDialogOpening(wv2scriptDialogOpeningEventArgs_t args)
    {
    }

    void Wv2::EventHandle::DownloadStarting(wv2downloadStartingEventArgs_t args)
    {
    }

    void Wv2::EventHandle::WebResourceRequested(wv2webResourceRequestedEventArgs_t args)
    {
    }


    //    
    //    return DefWindowProc(hWnd, uMsg,wParam,lParam);
    //}
}

