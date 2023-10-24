#ifndef UI_CONTROL_SPLIT_H_
#define UI_CONTROL_SPLIT_H_

#pragma once

#include "duilib/Core/Control.h"
#include "duilib/Core/Box.h"

namespace ui
{

/** 分割条控件，可以通过拖动分割条改变左右或者上下两个控件的宽度或者高度
*   应用：如果放在横向布局（HLayout）中，则左右拖动
*        如果放在纵向布局（VLayout）中，则上下拖动
*   注意事项：如果两个控件都设置为拉伸类型的，则分割条无法正常工作。
*/
template<typename InheritType = Control>
class UILIB_API SplitTemplate : public InheritType
{
public:
    SplitTemplate();

    virtual std::wstring GetType() const override;
 
    /** 是否可以拖动
    */
    bool IsSplitDragValid();

    /** 停止拖动
    */
    void StopSplitDrag();

    /** 监听拖动的事件
     * @param [in] callback 被选择时触发的回调函数
     */
    void AttachSplitDraged(const EventCallback& callback) { this->AttachEvent(kEventSplitDraged, callback); }

protected:
    virtual bool MouseEnter(const EventArgs& msg) override;
    virtual bool ButtonDown(const EventArgs& msg) override;
    virtual bool ButtonUp(const EventArgs& msg) override;
    virtual bool MouseMove(const EventArgs& msg) override;

private:
    //是否为垂直布局的容器
    bool IsVLayout(Control* pControl) const;

    //是否为水平布局的容器
    bool IsHLayout(Control* pControl) const;

    /** 调整两个控件中，其中一个（pFirst）控件的宽度或者高度
    * @param [in] bHLayout true表示横向布局，false表示纵向布局
    * @param [in] nTotal 两个控件的总体宽度或者高度
    * @param [in] nOffset 鼠标的宽度或者高度偏移量
    * @param [in] pFirst 第一个控件接口，该函数会调整该接口对应控件的宽度或者高度
    * @param [in] nFirstFixedInt 第一个控件的原始宽度或者高度值（在鼠标按下时记录）
    * @param [in] pSecond 第二个控件接口，读取其最小值，避免调整第一个控件时未给留下足够最小空间
    * @return 返回第一个控件的宽度或者高度
    */
    int32_t CalculateControlPos(bool bHLayout, const int32_t nTotal, const int32_t nOffset,
                                Control* pFirst, const UiFixedInt& nFirstFixedInt,
                                const Control* pSecond) const;

private:
    //左侧（左右拖动）或者上侧（上下拖动）的控件接口
    Control* m_pLeftTop;

    //右侧（左右拖动）或者下侧（上下拖动）的控件接口
    Control* m_pRightBottom;

    //左侧（左右拖动）或者上侧（上下拖动）的控件的宽度（左右拖动）或者高度（上下拖动）
    UiFixedInt m_nLeftUpFixedValue;

    //右侧（左右拖动）或者下侧（上下拖动）的控件的宽度（左右拖动）或者高度（上下拖动）
    UiFixedInt m_nRightBottomFixedValue;

    //开始拖动时的鼠标位置
    UiPoint m_ptStart;

    //布局方向: true表示横向布局，false表示纵向布局
    bool m_bHLayout;
};

template<typename InheritType>
SplitTemplate<InheritType>::SplitTemplate():
    m_pLeftTop(nullptr),
    m_pRightBottom(nullptr),
    m_bHLayout(true)
{
    this->SetMouseEnabled(true);
}

template<typename InheritType>
inline std::wstring SplitTemplate<InheritType>::GetType() const { return DUI_CTR_SPLIT; }

template<>
inline std::wstring SplitTemplate<Box>::GetType() const { return DUI_CTR_SPLITBOX; }

template<typename InheritType>
bool SplitTemplate<InheritType>::IsSplitDragValid() 
{
    return (m_pLeftTop != nullptr) && (m_pRightBottom != nullptr);
}

template<typename InheritType>
void SplitTemplate<InheritType>::StopSplitDrag() 
{
    m_pLeftTop = nullptr; 
    m_pRightBottom = nullptr;
}

template<typename InheritType>
bool SplitTemplate<InheritType>::IsVLayout(Control* pControl) const
{
    Box* pBox = dynamic_cast<Box*>(pControl);
    if (pBox == nullptr) {
        return false;
    }
    Layout* pLayout = pBox->GetLayout();
    if (pLayout == nullptr) {
        return false;
    }
    LayoutType layoutType = pLayout->GetLayoutType();
    if (layoutType == LayoutType::VLayout) {
        return true;
    }
    return false;
}

template<typename InheritType>
bool SplitTemplate<InheritType>::IsHLayout(Control* pControl) const
{
    Box* pBox = dynamic_cast<Box*>(pControl);
    if (pBox == nullptr) {
        return false;
    }
    Layout* pLayout = pBox->GetLayout();
    if (pLayout == nullptr) {
        return false;
    }
    LayoutType layoutType = pLayout->GetLayoutType();
    if (layoutType == LayoutType::HLayout) {
        return true;
    }
    return false;
}

template<typename InheritType>
bool SplitTemplate<InheritType>::MouseEnter(const EventArgs& msg)
{
    if (IsHLayout(this->GetParent())) {
        //横向布局
        if (this->GetCursorType() != kCursorSizeWE) {
            this->SetCursorType(kCursorSizeWE);
        }
    }
    else if (IsVLayout(this->GetParent())) {
        //纵向布局
        if (this->GetCursorType() != kCursorSizeNS) {
            this->SetCursorType(kCursorSizeNS);
        }
    }
    return __super::MouseEnter(msg);
}

template<typename InheritType>
bool SplitTemplate<InheritType>::ButtonDown(const EventArgs& msg)
{
    bool bRet = __super::ButtonDown(msg);
    Box* pParent = this->GetParent();
    if (!this->IsEnabled() || (pParent == nullptr)) {
        return bRet;
    }
    size_t nChildCount = pParent->GetItemCount();
    Control* pPrev = nullptr;
    Control* pNext = nullptr;
    Control* pThis = nullptr;
    for (size_t i = 0; i < nChildCount; ++i) {
        Control* pControl = pParent->GetItemAt(i);
        if (!pControl->IsVisible() || pControl->IsFloat()) {
            continue;
        }
        if (pThis) {
            pNext = pControl;
            break;
        }
        if (pControl == this) {
            pThis = pControl;
        }
        else {
            pPrev = pControl;
        }
    }
    if (pPrev && pNext && pThis) {
        if (IsVLayout(pParent)) {
            m_bHLayout = false;
            m_pLeftTop = pPrev;
            m_pRightBottom = pNext;
            m_ptStart = msg.ptMouse;
            m_nLeftUpFixedValue = m_pLeftTop->GetFixedHeight();
            if (!m_nLeftUpFixedValue.IsStretch()) {
                m_nLeftUpFixedValue.SetInt32(m_pLeftTop->GetHeight());
            }
            m_nRightBottomFixedValue = m_pRightBottom->GetFixedHeight();
            if (!m_nRightBottomFixedValue.IsStretch()) {
                m_nRightBottomFixedValue.SetInt32(m_pRightBottom->GetHeight());
            }
        }
        else if (IsHLayout(pParent)) {
            m_bHLayout = true;
            m_pLeftTop = pPrev;
            m_pRightBottom = pNext;
            m_ptStart = msg.ptMouse;
            m_nLeftUpFixedValue = m_pLeftTop->GetFixedWidth();
            if (!m_nLeftUpFixedValue.IsStretch()) {
                m_nLeftUpFixedValue.SetInt32(m_pLeftTop->GetWidth());
            }
            m_nRightBottomFixedValue = m_pRightBottom->GetFixedWidth();
            if (!m_nRightBottomFixedValue.IsStretch()) {
                m_nRightBottomFixedValue.SetInt32(m_pRightBottom->GetWidth());
            }
        }
    }
    if (!IsSplitDragValid()) {
        StopSplitDrag();
    }
    this->Invalidate();
    return bRet;
}

template<typename InheritType>
bool SplitTemplate<InheritType>::ButtonUp(const EventArgs& msg)
{
    bool bRet = __super::ButtonUp(msg);
    StopSplitDrag();
    return bRet;
}

template<typename InheritType>
int32_t SplitTemplate<InheritType>::CalculateControlPos(bool bHLayout, const int32_t nTotal, const int32_t nOffset,
                                                        Control* pFirst, const UiFixedInt& nFirstFixedInt,
                                                        const Control* pSecond) const
{
    int32_t nNewValue = nFirstFixedInt.GetInt32() + nOffset; //新值，可以是正数，也可以是负数
    if (nNewValue < 0) {
        nNewValue = 0;
    }

    int32_t nMin = 0; //最小值
    if (bHLayout) {
        nMin = pFirst->GetMinWidth();
    }
    else {
        nMin = pFirst->GetMinHeight();
    }    
    //控制不小于最小值
    if (nNewValue < nMin) {
        nNewValue = nMin;
    }

    int32_t nMax = 0; //最大值, 并且按照另一侧的最小值设置，留出最小空间
    if (bHLayout) {
        nMax = pFirst->GetMaxWidth();
        if (pSecond->GetMinWidth() > 0) {
            int32_t nNewMax = nTotal - pSecond->GetMinWidth();
            if ((nNewMax > 0) && (nMax > nNewMax)) {
                nMax = nNewMax;
            }
        }
    }
    else {
        nMax = pFirst->GetMaxHeight();
        if (pSecond->GetMinHeight() > 0) {
            int32_t nNewMax = nTotal - pSecond->GetMinHeight();
            if ((nNewMax > 0) && (nMax > nNewMax)) {
                nMax = nNewMax;
            }
        }
    }
    //控制不大于最大值
    if (nNewValue > nMax) {
        nNewValue = nMax;
    }
    return nNewValue;    
}

template<typename InheritType>
bool SplitTemplate<InheritType>::MouseMove(const EventArgs& msg)
{
    bool bRet = __super::MouseMove(msg);
    if (!IsSplitDragValid()) {
        return bRet;
    }
    
    int32_t nOffset = 0; //拖动的范围（宽度或者高度的偏移量, 可以是正数，也可以是负数）
    if (m_bHLayout) {
        nOffset = msg.ptMouse.x - m_ptStart.x;
    }
    else {
        nOffset = msg.ptMouse.y - m_ptStart.y;
    }

    //拖动时，不能超过总的宽度值或者总的高度值
    if (nOffset > 0) {
        if (!m_nRightBottomFixedValue.IsStretch()) {
            if (nOffset > m_nRightBottomFixedValue.GetInt32()) {
                return bRet;
            }
        }
    }
    else if(nOffset < 0){
        if (!m_nLeftUpFixedValue.IsStretch()) {
            if (-nOffset > m_nLeftUpFixedValue.GetInt32()) {
                return bRet;
            }
        }
    }
    else {
        return bRet;
    }
    
    int32_t nTotal = 0; //总的宽度值或者总的高度值
    if (m_nLeftUpFixedValue.IsInt32() && m_nRightBottomFixedValue.IsInt32()) {
        nTotal = m_nLeftUpFixedValue.GetInt32() + m_nRightBottomFixedValue.GetInt32();
    }
    else {
        if (m_bHLayout) {
            nTotal = m_pLeftTop->GetWidth() + m_pRightBottom->GetWidth();
        }
        else {
            nTotal = m_pLeftTop->GetHeight() + m_pRightBottom->GetHeight();
        }
    }

    //对于拉伸类型的控件，不调整，交给父容器自动调整（如果两个控件都是拉伸类型的，分割条就无法工作了）
    Control* pControl1 = nullptr;
    Control* pControl2 = nullptr;
    int32_t nNewValue1 = 0;
    int32_t nNewValue2 = 0;
    if (!m_nLeftUpFixedValue.IsStretch()) {
        pControl1 = m_pLeftTop;
        nNewValue1 = CalculateControlPos(m_bHLayout, nTotal, nOffset, m_pLeftTop, m_nLeftUpFixedValue, m_pRightBottom);
    }
    if (!m_nRightBottomFixedValue.IsStretch()) {
        pControl2 = m_pRightBottom;
        nNewValue2 = CalculateControlPos(m_bHLayout, nTotal, -nOffset, m_pRightBottom, m_nRightBottomFixedValue, m_pLeftTop);
    }
    if ((nNewValue1 + nNewValue2) != nTotal) {
        //超过限制，不调整
        return bRet;
    }
    bool bAdjusted = false;
    if ((pControl1 != nullptr) && (nNewValue1 >= 0)) {
        bAdjusted = true;
        if (m_bHLayout) {
            pControl1->SetFixedWidth(UiFixedInt(nNewValue1), true, false);
        }
        else {
            pControl1->SetFixedHeight(UiFixedInt(nNewValue1), true, false);
        }
    }
    if ((pControl2 != nullptr) && (nNewValue2 >= 0)) {
        bAdjusted = true;
        if (m_bHLayout) {
            pControl2->SetFixedWidth(UiFixedInt(nNewValue2), true, false);
        }
        else {
            pControl2->SetFixedHeight(UiFixedInt(nNewValue2), true, false);
        }
    }

    if (bAdjusted && (pControl1 != nullptr) || (pControl2 != nullptr)) {
        this->SendEvent(kEventSplitDraged, (WPARAM)pControl1, (LPARAM)pControl2);
    }
    return bRet;
}

typedef SplitTemplate<Control> Split;
typedef SplitTemplate<Box> SplitBox;

}//name space ui

#endif // UI_CONTROL_SPLIT_H_