#include "BasicForm.h"
#include "ui_components/wv2_control/wv2_control.h"

BasicForm::BasicForm()
{
}

BasicForm::~BasicForm()
{
}

DString BasicForm::GetSkinFolder()
{
    return _T("basic");
}

DString BasicForm::GetSkinFile()
{
    return _T("basic.xml");
}

void BasicForm::OnInitWindow()
{
    BaseClass::OnInitWindow();
    ui::Wv2* web = (ui::Wv2*)FindControl(L"web");
    web->Navigate(L"https://www.baidu.com");
}

ui::Control* BasicForm::CreateControl(const DString& strClass)
{
    if (strClass == L"Wv2")
    {
        return new ui::Wv2(this);
    }

    return BaseClass::CreateControl(strClass);
}
