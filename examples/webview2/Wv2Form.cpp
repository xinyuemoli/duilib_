#include "Wv2Form.h"
#include "ui_components/wv2_control/wv2_control.h"

Wv2Form::Wv2Form()
{
}

Wv2Form::~Wv2Form()
{
}

DString Wv2Form::GetSkinFolder()
{
    return _T("wv2");
}

DString Wv2Form::GetSkinFile()
{
    return _T("wv2.xml");
}

void Wv2Form::OnInitWindow()
{
    BaseClass::OnInitWindow();
    ui::Wv2* web = (ui::Wv2*)FindControl(L"web");
    web->Navigate(L"https://www.baidu.com");
}

ui::Control* Wv2Form::CreateControl(const DString& strClass)
{
    if (strClass == L"Wv2")
    {
        return new ui::Wv2(this,NULL);
    }

    return BaseClass::CreateControl(strClass);
}
