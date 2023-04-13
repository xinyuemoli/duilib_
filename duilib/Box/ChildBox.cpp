#include "ChildBox.h"
#include "duilib/Core/Define.h"
#include "duilib/Core/WindowBuilder.h"
#include "duilib/Core/GlobalManager.h"
#include "duilib/Utils/WinImplBase.h"

namespace ui
{

ChildBox::ChildBox()
{

}

std::wstring ChildBox::GetType() const { return DUI_CTR_CHILDBOX; }

void ChildBox::Init()
{
	if (!m_strXMLFile.empty())
	{
		CreateControlCallback callback = CreateControlCallback();
		auto pBaseWindow = dynamic_cast<WindowImplBase*>(m_pWindow);
		if (pBaseWindow != nullptr)
			callback = std::bind(&WindowImplBase::CreateControl, pBaseWindow, std::placeholders::_1);

		Box* pChildWindow = static_cast<Box*>(GlobalManager::CreateBoxWithCache(m_strXMLFile.c_str(), callback));
		if (pChildWindow) {
			this->Add(pChildWindow);
		}
		else {
			ASSERT(FALSE);
			this->RemoveAll();
		}
	}
}

void ChildBox::SetAttribute( const::std::wstring& strName, const std::wstring& strValue )
{
	if (strName == L"xmlfile") {
		SetChildLayoutXML(strValue);
	}
	else {
		Box::SetAttribute(strName, strValue);
	}
}

void ChildBox::SetChildLayoutXML(const std::wstring& strXML )
{
	m_strXMLFile = strXML;
}

const std::wstring& ChildBox::GetChildLayoutXML()
{
	return m_strXMLFile;
}

} // namespace ui
