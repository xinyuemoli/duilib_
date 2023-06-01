#include "TreeView.h"
#include "duilib/Control/ScrollBar.h"

namespace ui
{

TreeNode::TreeNode() :
	m_bExpand(true),
	m_pTreeView(nullptr),
	m_pParentTreeNode(nullptr),
	m_iDepth(ROOT_NODE_DEPTH),
	m_aTreeNodes()
{
	
}

std::wstring TreeNode::GetType() const { return DUI_CTR_TREENODE; }

void TreeNode::SetTreeView(TreeView* pTreeView)
{
    m_pTreeView = pTreeView;
}

bool TreeNode::OnClickItem(const EventArgs& param)
{
    TreeNode* pItem = dynamic_cast<TreeNode*>(param.pSender);
	ASSERT(pItem != nullptr);
	if (pItem) {
		pItem->SetExpand(!pItem->IsExpand(), true);
	}    
    return true;
}

bool TreeNode::IsVisible() const
{
	if (!ListBoxElement::IsVisible()) {
		return false;
	}
	if (m_pParentTreeNode != nullptr) {
		if (!m_pParentTreeNode->IsExpand() || !m_pParentTreeNode->IsVisible()) {
			return false;
		}
	}
	return true;
}

void TreeNode::SetWindow(Window* pManager, Box* pParent, bool bInit)
{
	ListBoxElement::SetWindow(pManager, pParent, bInit);
}

TreeNode* TreeNode::GetParentNode()
{
	return m_pParentTreeNode;
}

void TreeNode::SetParentNode(TreeNode* pParentTreeNode)
{
	m_pParentTreeNode = pParentTreeNode;
}

bool TreeNode::AddChildNode(TreeNode* pTreeNode)
{
	return AddChildNodeAt(pTreeNode, GetChildNodeCount());
}

bool TreeNode::AddChildNodeAt(TreeNode* pTreeNode, size_t iIndex)
{
	ASSERT(pTreeNode != nullptr);
	if (pTreeNode == nullptr) {
		return false;
	}
	if (iIndex > m_aTreeNodes.size()) {
		return false;
	}
	m_aTreeNodes.insert(m_aTreeNodes.begin() + iIndex, pTreeNode);
		
	pTreeNode->m_iDepth = m_iDepth + 1;
	pTreeNode->SetParentNode(this);
	pTreeNode->SetTreeView(m_pTreeView);
	if (GetWindow() != nullptr) {
		GetWindow()->InitControls(pTreeNode, nullptr);
	}
	pTreeNode->AttachEvent(kEventClick, nbase::Bind(&TreeNode::OnClickItem, this, std::placeholders::_1));

	UiRect padding = GetLayout()->GetPadding();
	size_t nodeIndex = 0;
	if (m_iDepth != ROOT_NODE_DEPTH) {
		nodeIndex = GetIndex() + 1;
		padding.left += m_pTreeView->GetIndent();
	}
	pTreeNode->GetLayout()->SetPadding(padding, true);

	size_t nGlobalIndex = iIndex;
	for (size_t i = 0; i < iIndex; i++)	{
		nGlobalIndex += ((TreeNode*)m_aTreeNodes[i])->GetDescendantNodeCount();
	}

	return m_pTreeView->ListBox::AddItemAt(pTreeNode, nodeIndex + nGlobalIndex);
}

bool TreeNode::RemoveChildNodeAt(size_t iIndex)
{
	if (iIndex >= m_aTreeNodes.size()) {
		return false;
	}

	TreeNode* pTreeNode = ((TreeNode*)m_aTreeNodes[iIndex]);
	m_aTreeNodes.erase(m_aTreeNodes.begin() + iIndex);
	if (pTreeNode) {
		return pTreeNode->RemoveSelf();
	}
	return false;
}

bool TreeNode::RemoveChildNode(TreeNode* pTreeNode)
{
	auto it = std::find(m_aTreeNodes.begin(), m_aTreeNodes.end(), pTreeNode);
	if (it == m_aTreeNodes.end()) {
		return false;
	}
		
	int iIndex = static_cast<int>(it - m_aTreeNodes.begin());
	return RemoveChildNodeAt(iIndex);
}
	
void TreeNode::RemoveAllChildNode()
{
	while (m_aTreeNodes.size() > 0)
	{
		RemoveChildNodeAt(0);
	}
}

bool TreeNode::RemoveSelf()
{
	for( auto it = m_aTreeNodes.begin(); it != m_aTreeNodes.end(); it++ ) 
	{
		(*it)->RemoveSelf();
	}
	m_aTreeNodes.clear();

	if (m_iDepth != ROOT_NODE_DEPTH) {
		return m_pTreeView->ListBox::RemoveItemAt(GetIndex());
	}

	return false;
}

int TreeNode::GetDescendantNodeCount()
{
	int nodeCount = (int)GetChildNodeCount();
	for( auto it = m_aTreeNodes.begin(); it != m_aTreeNodes.end(); it++ )
	{
		nodeCount += (*it)->GetDescendantNodeCount();
	}

	return nodeCount;
}

size_t TreeNode::GetChildNodeCount()
{
	return m_aTreeNodes.size();
}
	
TreeNode* TreeNode::GetChildNode(size_t iIndex)
{
	if (iIndex >= m_aTreeNodes.size()) {
		return nullptr;
	}
	return m_aTreeNodes[iIndex];
}
	
int TreeNode::GetChildNodeIndex(TreeNode* pTreeNode)
{
	auto it = std::find(m_aTreeNodes.begin(), m_aTreeNodes.end(), pTreeNode);
	if (it == m_aTreeNodes.end()) {
		return -1;
	}
	return static_cast<int>(it - m_aTreeNodes.begin());
}

bool TreeNode::IsExpand() const
{
	return m_bExpand;
}

void TreeNode::SetExpand(bool bExpand, bool bTriggerEvent)
{
	if(m_bExpand == bExpand) {
		return;
	}
	m_bExpand = bExpand;

	if (bTriggerEvent) {
		SendEvent(m_bExpand ? kEventExpand : kEventUnExpand);
	}
	if (m_pTreeView != nullptr) {
		m_pTreeView->Arrange();
	}	
}

int TreeNode::GetDepth() const
{
	return m_iDepth;
}

TreeView::TreeView() :
	ListBox(new VLayout),
	m_iIndent(0),
	m_rootNode()
{
	m_rootNode.reset(new TreeNode());
	m_rootNode->SetTreeView(this);
}

std::wstring TreeView::GetType() const { return DUI_CTR_TREEVIEW; }

void TreeView::SetAttribute(const std::wstring& strName, const std::wstring& strValue)
{
	if( strName == L"indent" ) {
		SetIndent(_wtoi(strValue.c_str()));
	}
	else {
		ListBox::SetAttribute(strName, strValue);
	}
}

bool TreeView::AddItem(Control* /*pControl*/)
{
	ASSERT(FALSE);
	return true;
}

bool TreeView::AddItemAt(Control* /*pControl*/, size_t /*iIndex*/)
{
	ASSERT(FALSE);
	return true;
}

bool TreeView::RemoveItem(Control* /*pControl*/)
{
	ASSERT(FALSE);
	return true;
}

bool TreeView::RemoveItemAt(size_t /*iIndex*/)
{
	ASSERT(FALSE);
	return true;
}

void TreeView::RemoveAllItems()
{
	ASSERT(FALSE);
}

void TreeView::SetWindow(Window* pManager, Box* pParent, bool bInit)
{
	ListBox::SetWindow(pManager, pParent, bInit);
	m_rootNode->SetWindow(pManager, pParent, bInit);
}

}