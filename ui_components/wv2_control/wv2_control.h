#pragma once
#ifndef  __WV2_CONTROL_H
#define __WV2_CONTROL_H
#include <include/wv2.h>
#include "duilib/Core/Control.h"
namespace ui
{
	class UILIB_API Wv2:public Control
	{
	public:
		Wv2(ui::Window* pWindow);
		~Wv2();

		//事件处理类
		class EventHandle
		{
			//处理事件
		public:
			//历史记录改变
			virtual void HistoryChanged(bool canGoBack, bool canGoForward);
			//开始导航
			virtual bool NavigationStarting(LPCWSTR uri);
			virtual void NavigationCompleted(wv2navigationCompletedEventArgs_t args);
			virtual void DomContentLoaded(wv2domContentLoadedEventArgs_t args);
			virtual bool WindowCloseRequested();
			virtual void WebMessageReceived(LPCWSTR message);
			virtual void IsMutedChanged();
			virtual void IsDocumentPlayingAudioChanged();
			virtual void NewWindowRequested(wv2newWindowRequestedEventArgs_t args);
			virtual void DocumentTitleChanged();
			virtual void ContentLoading(wv2contentLoadingEventArgs_t args);
			virtual void ScriptDialogOpening(wv2scriptDialogOpeningEventArgs_t args);
			virtual void DownloadStarting(wv2downloadStartingEventArgs_t args);
			virtual void WebResourceRequested(wv2webResourceRequestedEventArgs_t args);
		};
		void SetHandler(EventHandle* handler);

        virtual void Init();
		virtual void SetPos(UiRect rc) override;

		//获取当前url
		std::wstring GetCurlUrl();
		//返回
		bool GoBack();
		//前进
		bool GoForward();

		//导航到
		bool Navigate(const std::wstring& url);
		//重新加载
		bool Reload();
		//刷新大小
		bool Resize(int x, int y, int w, int h);
		//向页面发送消息
		bool PostJsonMessage(const std::wstring& json_msg);
		
		static std::map<wv2_t, EventHandle*> g_handlers;
		EventHandle* delegate_ = nullptr;
	private:
		void RegisterHandler();
		/*ATOM MyRegisterClass(HINSTANCE hInstance);*/
		wv2_t webview_ = nullptr;
        wv2envOpts_t options_ = nullptr;
		
		/*std::wstring host_wnd_cls_name_ = L"wv2_cls_host";
		HWND host_wnd_ = NULL;

		static LRESULT CALLBACK __WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);*/
	};
}
#endif // ! __WV2_H
