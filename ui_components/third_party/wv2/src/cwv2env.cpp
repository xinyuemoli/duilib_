#include "cwv2env.h"
#include "cwv2types.h"
#include "cwv2webResourceResponse.h"

/*
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
*/

void cwv2env::Release() {

	if(env2_) {
		env2_.Release();
	}

	if(env5_) {
		browserProcessExitedHandler_.remove(env5_);
		env5_.Release();
	}
}

HRESULT cwv2env::createCoreWebView2EnvironmentCompleted(ICoreWebView2Environment* env) {

	// CoreWebView2Environment2 ȹ��, 2022.12.13 kim,jk
	HRESULT hr = env->QueryInterface(IID_ICoreWebView2Environment2,
		(void**)&env2_);

	if (FAILED(hr)) {
		return hr;
	}

	// CoreWebView2Environment5 ȹ��, 2024.04.30
	hr = env->QueryInterface(IID_ICoreWebView2Environment5,
		(void**)&env5_);

	if(SUCCEEDED(hr)) {
		browserProcessExitedHandler_.add(env5_);
	}

	return hr;
}

wv2bool cwv2env::setBrowserProcessExitedHandler(browserProcessExited handler) {
	wv2bool r = wv2boolNotSupported();
	if(!env5_) {
		return r;
	}

	browserProcessExitedHandler_.bind(handler, this);
	r.value = true;
	return r;
}

wv2webResourceResponse* cwv2env::createWebResourceResponse(
	IStream* content, int32_t statusCode, LPCWSTR reasonPhrase, LPCWSTR headers) {
	if (!env2_) {
		return nullptr;
	}

	CComPtr<ICoreWebView2WebResourceResponse> response;
	const HRESULT hr = env2_->CreateWebResourceResponse(content, statusCode, reasonPhrase, headers, &response);
	if (FAILED(hr)) return nullptr;
	return new cwv2webResourceResponse(response);
}