#ifndef UI_CONTROL_LIST_CTRL_ICON_VIEW_H_
#define UI_CONTROL_LIST_CTRL_ICON_VIEW_H_

#pragma once

#include "duilib/Box/VirtualListBox.h"
#include "duilib/Control/ListCtrlDefs.h"

namespace ui
{
/** 列表数据显示和布局功能(Icon类型)
*/
class ListCtrl;
class ListCtrlData;
class ListCtrlIconView : public VirtualListBox, public IListCtrlView
{
public:
    /** 设置是否为List模式
    * @param [in] bListMode true表示列表视图，否则为图标视图
    */
    explicit ListCtrlIconView(bool bListMode);
    virtual ~ListCtrlIconView();

    virtual std::wstring GetType() const override { return L"ListCtrlIconView"; }
    virtual void SetAttribute(const std::wstring& strName, const std::wstring& strValue);

public:
    /** 设置ListCtrl控件接口
    */
    void SetListCtrl(ListCtrl* pListCtrl);

    /** 设置数据代理对象
    * @param[in] pProvider 数据代理对象
    */
    virtual void SetDataProvider(VirtualListBoxElement* pProvider) override;

    /** 监听选择项发生变化的事件
     * @param[in] callback 选择子项时的回调函数
     */
    void AttachSelChange(const EventCallback& callback) { AttachEvent(kEventSelChange, callback); }

    /** 发送事件的函数
    */
    virtual void SendEvent(EventType eventType, WPARAM wParam = 0, LPARAM lParam = 0, TCHAR tChar = 0,
                           const UiPoint& mousePos = UiPoint()) override;
    virtual void SendEvent(const EventArgs& event) override;

public:
    /** 获取顶部元素的索引号
    */
    size_t GetTopElementIndex() const;

    /** 判断一个数据项是否可见
    * @param [in] itemIndex 数据项的索引号, 有效范围：[0, GetDataItemCount())
    */
    bool IsDataItemDisplay(size_t itemIndex) const;

    /** 获取当前显示的数据项列表，顺序是从上到下
    * @param [in] itemIndexList 当前显示的数据项索引号列表
    */
    void GetDisplayDataItems(std::vector<size_t>& itemIndexList) const;

    /** 确保数据索引项可见
    * @param [in] itemIndex 数据项的索引号
    * @param [in] bToTop 是否确保在最上方
    */
    bool EnsureDataItemVisible(size_t itemIndex, bool bToTop);

protected:
    /** 控件初始化
    */
    virtual void DoInit() override;

       /** 创建一个数据项
    * @return 返回创建后的数据项指针
    */
    virtual Control* CreateDataItem() override;

    /** 填充指定数据项
    * @param [in] pControl 数据项控件指针
    * @param [in] nElementIndex 数据元素的索引ID，范围：[0, GetElementCount())
    * @param [in] itemData 数据项（代表行的属性）
    * @param [in] subItemList 数据子项（代表每一列的数据, 第1个是列的ID，第2个是列的数据）
    */
    virtual bool FillDataItem(Control* pControl,
                              size_t nElementIndex,
                              const ListCtrlItemData& itemData,
                              const std::vector<ListCtrlSubItemData2Pair>& subItemList) override;


    /** 获取某列的宽度最大值
    * @param [in] subItemList 数据子项（代表每一列的数据）
    * @return 返回该列宽度的最大值，返回的是DPI自适应后的值； 如果失败返回-1
    */
    virtual int32_t GetMaxDataItemWidth(const std::vector<ListCtrlSubItemData2Ptr>& subItemList) override;

private:
    /** ListCtrl 控件接口
    */
    ListCtrl* m_pListCtrl;

    /** 数据接口
    */
    ListCtrlData* m_pData;

    /** 是否为List模式: true表示列表视图，否则为图标视图 
    */
    const bool m_bListMode;
};

}//namespace ui

#endif //UI_CONTROL_LIST_CTRL_ICON_VIEW_H_
