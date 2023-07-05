#include "Render_GdiPlus.h"
#include "duilib/Utils/DpiManager.h"
#include "duilib/Core/GlobalManager.h"
#include "duilib/RenderGdiPlus/Pen_Gdiplus.h"
#include "duilib/RenderGdiPlus/Path_Gdiplus.h"
#include "duilib/RenderGdiPlus/Brush_Gdiplus.h"
#include "duilib/RenderGdiPlus/Matrix_Gdiplus.h"
#include "duilib/RenderGdiPlus/Bitmap_GDI.h"
#include "duilib/RenderGdiPlus/GdiPlusDefs.h"
#include "duilib/Render/BitmapAlpha.h"

namespace ui {

static inline void DrawFunction(HDC hDC, bool bTransparent, UiRect rcDest, HDC hdcSrc, UiRect rcSrc, bool bAlphaChannel, int uFade)
{
	if (bTransparent || bAlphaChannel || uFade < 255 
		|| (rcSrc.GetWidth() == rcDest.GetWidth() && rcSrc.GetHeight() == rcDest.GetHeight())) {
		BLENDFUNCTION ftn = { AC_SRC_OVER, 0, static_cast<uint8_t>(uFade), AC_SRC_ALPHA };
		::AlphaBlend(hDC, rcDest.left, rcDest.top, rcDest.GetWidth(), rcDest.GetHeight(),
			hdcSrc, rcSrc.left, rcSrc.top, rcSrc.GetWidth(), rcSrc.GetHeight(), ftn);
	}
	else {
		::StretchBlt(hDC, rcDest.left, rcDest.top, rcDest.GetWidth(), rcDest.GetHeight(),
			hdcSrc, rcSrc.left, rcSrc.top, rcSrc.GetWidth(), rcSrc.GetHeight(), SRCCOPY);
	}
}

Render_GdiPlus::Render_GdiPlus()
	: m_hDC(nullptr)
	, m_saveDC(0)
	, m_bTransparent(false)
	, m_hOldBitmap(nullptr)
	, m_hBitmap(nullptr)
	, m_pPiexl(nullptr)
	, m_nWidth(0)
	, m_nHeight(0)
{
	HDC hDC = ::GetDC(NULL);
	m_hDC = ::CreateCompatibleDC(hDC);
	::ReleaseDC(NULL, hDC);
	ASSERT(m_hDC != nullptr);
}

Render_GdiPlus::~Render_GdiPlus()
{
	if (m_hOldBitmap != nullptr) {
		::SelectObject(m_hDC, m_hOldBitmap);
		m_hOldBitmap = nullptr;
	}
	if (m_hDC != nullptr) {
		::DeleteDC(m_hDC);
		m_hDC = nullptr;
	}
	if (m_hBitmap != nullptr) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}
}

RenderType Render_GdiPlus::GetRenderType() const
{
	return RenderType::kRenderType_GdiPlus;
}

HDC Render_GdiPlus::GetDC()
{
	return m_hDC;
}

void Render_GdiPlus::ReleaseDC(HDC hdc)
{
	ASSERT(hdc == m_hDC);
}

bool Render_GdiPlus::Resize(int width, int height)
{
	ASSERT((width > 0) && (height > 0));
	if ((width <= 0) || (height <= 0)) {
		return false;
	}

	ASSERT(m_hDC != nullptr);
	if ((m_nWidth == width) && (m_nHeight == height)) {
		return true;
	}

	if (m_hOldBitmap != nullptr) {
		::SelectObject(m_hDC, m_hOldBitmap);
		m_hOldBitmap = nullptr;
	}
	if (m_hBitmap != nullptr) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}
	m_pPiexl = nullptr;
	m_nWidth = width;
	m_nHeight = height;
	m_hBitmap = Bitmap_GDI::CreateBitmap(width, height, true, (LPVOID*)&m_pPiexl);
	ASSERT(m_pPiexl != nullptr);
	if (m_pPiexl == nullptr) {
		if (m_hBitmap != nullptr) {
			::DeleteObject(m_hBitmap);
			m_hBitmap = nullptr;
		}
	}
	if (m_hBitmap == nullptr) {
		m_pPiexl = nullptr;
		m_nWidth = 0;
		m_nHeight = 0;
		return false;
	}	
	m_hOldBitmap = (HBITMAP)::SelectObject(m_hDC, m_hBitmap);
	return true;
}

void Render_GdiPlus::Clear()
{
	//将位图数据清零
	if ((m_pPiexl != nullptr) && (m_nWidth > 0) && (m_nHeight > 0)) {
		::memset(m_pPiexl, 0, m_nWidth * m_nHeight * 4);
	}
}

std::unique_ptr<ui::IRender> Render_GdiPlus::Clone()
{
	std::unique_ptr<ui::IRender> pClone = std::make_unique<ui::Render_GdiPlus>();
	pClone->Resize(GetWidth(), GetHeight());
	pClone->BitBlt(0, 0, GetWidth(), GetHeight(), this, 0, 0, RopMode::kSrcCopy);
	return pClone;
}

IBitmap* Render_GdiPlus::DetachBitmap()
{
	ASSERT(m_hOldBitmap != nullptr);
	ASSERT(GetHeight() > 0 && GetWidth() > 0);
	if (m_hOldBitmap != nullptr) {
		::SelectObject(m_hDC, m_hOldBitmap);
		m_hOldBitmap = nullptr;
	}
	if (m_hBitmap == nullptr) {
		return nullptr;
	}
	IBitmap* pBitmap = nullptr;	
	HBITMAP hBitmap = m_hBitmap;
	if (hBitmap != nullptr) {
		pBitmap = new Bitmap_GDI(hBitmap, true);
	}
	m_hBitmap = nullptr;
	m_pPiexl = nullptr;
	m_nWidth = 0;
	m_nHeight = 0;
	return pBitmap;
}

int Render_GdiPlus::GetWidth()
{
	return m_nWidth;
}

int Render_GdiPlus::GetHeight()
{
	return m_nHeight;
}

void Render_GdiPlus::ClearAlpha(const UiRect& rcDirty, uint8_t alpha)
{
	HBITMAP hBitmap = m_hBitmap;
	ASSERT(hBitmap != nullptr);
	if (hBitmap != nullptr) {
		BITMAP bm = { 0 };
		::GetObject(hBitmap, sizeof(bm), &bm);
		BitmapAlpha bitmapAlpha((uint8_t*)bm.bmBits, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel / 8);
		bitmapAlpha.ClearAlpha(rcDirty, alpha);
	}
}

void Render_GdiPlus::RestoreAlpha(const UiRect& rcDirty, const UiRect& rcShadowPadding, uint8_t alpha)
{
	HBITMAP hBitmap = m_hBitmap;
	ASSERT(hBitmap != nullptr);
	if (hBitmap != nullptr) {
		BITMAP bm = { 0 };
		::GetObject(hBitmap, sizeof(bm), &bm);
		BitmapAlpha bitmapAlpha((uint8_t*)bm.bmBits, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel / 8);
		bitmapAlpha.RestoreAlpha(rcDirty, rcShadowPadding, alpha);
	}
}

void Render_GdiPlus::RestoreAlpha(const UiRect& rcDirty, const UiRect& rcShadowPadding /*= UiRect()*/)
{
	HBITMAP hBitmap = m_hBitmap;
	ASSERT(hBitmap != nullptr);
	if (hBitmap != nullptr) {
		BITMAP bm = { 0 };
		::GetObject(hBitmap, sizeof(bm), &bm);
		BitmapAlpha bitmapAlpha((uint8_t*)bm.bmBits, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel / 8);
		bitmapAlpha.RestoreAlpha(rcDirty, rcShadowPadding);
	}
}

bool Render_GdiPlus::IsRenderTransparent() const
{
	return m_bTransparent;
}

bool Render_GdiPlus::SetRenderTransparent(bool bTransparent)
{
	bool oldValue = m_bTransparent;
	m_bTransparent = bTransparent;
	return oldValue;
}

UiPoint Render_GdiPlus::OffsetWindowOrg(UiPoint ptOffset)
{
	UiPoint ptOldWindowOrg;
	::GetWindowOrgEx(m_hDC, &ptOldWindowOrg);
	ptOffset.Offset(ptOldWindowOrg.x, ptOldWindowOrg.y);
	::SetWindowOrgEx(m_hDC, ptOffset.x, ptOffset.y, NULL);
	return ptOldWindowOrg;
}

UiPoint Render_GdiPlus::SetWindowOrg(UiPoint ptOffset)
{
	UiPoint ptOldWindowOrg;
	::GetWindowOrgEx(m_hDC, &ptOldWindowOrg);
	::SetWindowOrgEx(m_hDC, ptOffset.x, ptOffset.y, NULL);
	return ptOldWindowOrg;
}

UiPoint Render_GdiPlus::GetWindowOrg() const
{
	UiPoint ptWindowOrg;
	::GetWindowOrgEx(m_hDC, &ptWindowOrg);
	return ptWindowOrg;
}

void Render_GdiPlus::SaveClip(int& nState)
{
	m_saveDC = ::SaveDC(m_hDC);
	nState = m_saveDC;
}

void Render_GdiPlus::RestoreClip(int nState)
{
	ASSERT(m_saveDC == nState);
	if (nState != m_saveDC) {
		return;
	}
	if (m_saveDC != 0) {
		::RestoreDC(m_hDC, m_saveDC);
		m_saveDC = 0;
	}
	else {
		::RestoreDC(m_hDC, -1);
	}
}

void Render_GdiPlus::SetClip(const UiRect& rc)
{
	HDC hDC = m_hDC;
	UiRect rcItem = rc;

	UiPoint ptWinOrg;
	::GetWindowOrgEx(hDC, &ptWinOrg);	
	rcItem.Offset(-ptWinOrg.x, -ptWinOrg.y);
	HRGN hRgn = ::CreateRectRgnIndirect(&rcItem);
	::SaveDC(hDC);
	::ExtSelectClipRgn(hDC, hRgn, RGN_AND);
	::DeleteObject(hRgn);
}

void Render_GdiPlus::SetRoundClip(const UiRect& rc, int width, int height)
{
	HDC hDC = m_hDC;
	UiRect rcItem = rc;

	UiPoint ptWinOrg;
	::GetWindowOrgEx(hDC, &ptWinOrg);
	rcItem.Offset(-ptWinOrg.x, -ptWinOrg.y);

	HRGN hRgn = ::CreateRoundRectRgn(rcItem.left, rcItem.top, rcItem.right + 1, rcItem.bottom + 1, width, height);
	::SaveDC(hDC);
	::ExtSelectClipRgn(hDC, hRgn, RGN_AND);
	::DeleteObject(hRgn);
}

void Render_GdiPlus::ClearClip()
{
	HDC hDC = m_hDC;
	ASSERT(::GetObjectType(hDC) == OBJ_DC || ::GetObjectType(hDC) == OBJ_MEMDC);
	::RestoreDC(hDC, -1);
}

DWORD Render_GdiPlus::GetRopMode(RopMode rop) const
{
	DWORD ropMode = SRCCOPY;
	switch (rop) {
	case RopMode::kSrcCopy:
		ropMode = SRCCOPY;
		break;
	case RopMode::kDstInvert:
		ropMode = DSTINVERT;
		break;
	case RopMode::kSrcInvert:
		ropMode = SRCINVERT;
		break;
	case RopMode::kSrcAnd:
		ropMode = SRCAND;
		break;
	default:
		break;
	}
	return ropMode;
}

bool Render_GdiPlus::BitBlt(int x, int y, int cx, int cy, IBitmap* pSrcBitmap, int xSrc, int ySrc, RopMode rop)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	ASSERT(pSrcBitmap != nullptr);
	if (pSrcBitmap == nullptr) {
		return false;
	}
	Bitmap_GDI* gdiBitmap = dynamic_cast<Bitmap_GDI*>(pSrcBitmap);
	ASSERT(gdiBitmap != nullptr);
	if (gdiBitmap == nullptr) {
		return false;
	}
	HBITMAP hBitmap = gdiBitmap->GetHBitmap();
	ASSERT(hBitmap != nullptr);
	if (hBitmap == nullptr) {
		return false;
	}
	ASSERT(::GetObjectType(m_hDC) == OBJ_DC || ::GetObjectType(m_hDC) == OBJ_MEMDC);

	HDC hCloneDC = ::CreateCompatibleDC(m_hDC);
	HBITMAP hOldBitmap = (HBITMAP) ::SelectObject(hCloneDC, hBitmap);
	bool bResult = ::BitBlt(m_hDC, x, y, cx, cy, hCloneDC, xSrc, ySrc, GetRopMode(rop)) != FALSE;
	::SelectObject(hCloneDC, hOldBitmap);
	::DeleteDC(hCloneDC);
	return bResult;
}

bool Render_GdiPlus::BitBlt(int x, int y, int cx, int cy, IRender* pSrcRender, int xSrc, int ySrc, RopMode rop)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	ASSERT(pSrcRender != nullptr);
	bool bResult = false;
	if (pSrcRender != nullptr) {
		HDC hdcSrc = pSrcRender->GetDC();
		ASSERT(hdcSrc != nullptr);
		bResult = ::BitBlt(m_hDC, x, y, cx, cy, hdcSrc, xSrc, ySrc, GetRopMode(rop)) != FALSE;
		pSrcRender->ReleaseDC(hdcSrc);
	}
	return bResult;
}

bool Render_GdiPlus::StretchBlt(int xDest, int yDest, int widthDest, int heightDest, IRender* pSrcRender, int xSrc, int ySrc, int widthSrc, int heightSrc, RopMode rop)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	ASSERT(pSrcRender != nullptr);
	bool bResult = false;
	if (pSrcRender != nullptr) {
		HDC hdcSrc = pSrcRender->GetDC();
		ASSERT(hdcSrc != nullptr);
		int stretchBltMode = ::SetStretchBltMode(m_hDC, HALFTONE);
		bResult = ::StretchBlt(m_hDC, xDest, yDest, widthDest, heightDest,
							   hdcSrc, xSrc, ySrc, widthSrc, heightSrc, GetRopMode(rop)) != FALSE;
		::SetStretchBltMode(m_hDC, stretchBltMode);
		pSrcRender->ReleaseDC(hdcSrc);
	}
	return bResult;
}

bool Render_GdiPlus::AlphaBlend(int xDest, int yDest, int widthDest, int heightDest, IRender* pSrcRender, int xSrc, int ySrc, int widthSrc, int heightSrc, uint8_t alpha)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	ASSERT(pSrcRender != nullptr);
	bool bResult = false;
	if (pSrcRender != nullptr) {
		BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };
		HDC hdcSrc = pSrcRender->GetDC();
		ASSERT(hdcSrc != nullptr);
		bResult =::AlphaBlend(m_hDC, xDest, yDest, widthDest, heightDest, hdcSrc, xSrc, ySrc, widthSrc, heightSrc, bf) != FALSE;
		pSrcRender->ReleaseDC(hdcSrc);
	}
	return bResult;
}

void Render_GdiPlus::DrawImage(const UiRect& rcPaint, 
							   IBitmap* pBitmap, 						       
						       const UiRect& rcImageDest, 
							   UiRect rcImageSource, 
						       UiRect rcImageCorners,
						       bool bBitmapDpiScaled,
						       uint8_t uFade,
						       bool xtiled, 
						       bool ytiled, 
						       bool fullxtiled, 
						       bool fullytiled, 
						       int nTiledMargin)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	UiRect rcTestTemp;
	if (!::IntersectRect(&rcTestTemp, &rcImageDest, &rcPaint)) {
		return;
	}

	ASSERT(pBitmap != nullptr);
	if (pBitmap == nullptr) {
		return;
	}
	Bitmap_GDI* gdiBitmap = dynamic_cast<Bitmap_GDI*>(pBitmap);
	ASSERT(gdiBitmap != nullptr);
	if (gdiBitmap == nullptr) {
		return;
	}
	HBITMAP hBitmap = gdiBitmap->GetHBitmap();
	ASSERT(hBitmap != nullptr);
	if (hBitmap == nullptr) {
		return;
	}

	ASSERT(::GetObjectType(m_hDC) == OBJ_DC || ::GetObjectType(m_hDC) == OBJ_MEMDC);

	HDC hCloneDC = ::CreateCompatibleDC(m_hDC);
	HBITMAP hOldBitmap = (HBITMAP) ::SelectObject(hCloneDC, hBitmap);
	int stretchBltMode = ::SetStretchBltMode(m_hDC, HALFTONE);

	//对边角值进行容错处理
	if ((rcImageCorners.left < 0)  ||
		(rcImageCorners.top < 0)   ||
		(rcImageCorners.right < 0) ||
		(rcImageCorners.bottom < 0)) {
		rcImageCorners.left = 0;
		rcImageCorners.top = 0;
		rcImageCorners.right = 0;
		rcImageCorners.bottom = 0;
	}

	UiRect rcTemp;
	UiRect rcSource;
	UiRect rcDest;
	UiRect rcDpiCorner = rcImageCorners;
	DpiManager::GetInstance()->ScaleRect(rcDpiCorner);
	// 如果源位图已经按照DPI缩放过，那么对应的rcImageCorners也需要缩放
	if (bBitmapDpiScaled) {
		rcImageCorners = rcDpiCorner;
	}
	// 如果源位图已经按照DPI缩放过，那么对应的rcImageSource也需要缩放
	if ((rcImageSource.left < 0)  ||
		(rcImageSource.top < 0)   ||
		(rcImageSource.right < 0) ||
		(rcImageSource.bottom < 0)) {
		//如果是无效值，则重置为整个图片大小
		rcImageSource.left = 0;
		rcImageSource.top = 0;
		rcImageSource.right = pBitmap->GetWidth();
		rcImageSource.bottom = pBitmap->GetHeight();
	}
	else if (bBitmapDpiScaled) {
		//如果外部设置此值，做DPI自适应处理
		DpiManager::GetInstance()->ScaleRect(rcImageSource);
	}
	//图片源容错处理
	if (rcImageSource.left < 0) {
		rcImageSource.left = 0;
	}
	if (rcImageSource.top < 0) {
		rcImageSource.top = 0;
	}
	if (rcImageSource.right > (LONG)pBitmap->GetWidth()) {
		rcImageSource.right = pBitmap->GetWidth();
	}
	if (rcImageSource.bottom > (LONG)pBitmap->GetHeight()) {
		rcImageSource.bottom = pBitmap->GetHeight();
	}

	bool bAlphaChannel = pBitmap->IsAlphaBitmap();

	// middle
	rcDest.left = rcImageDest.left + rcDpiCorner.left;
	rcDest.top = rcImageDest.top + rcDpiCorner.top;
	rcDest.right = rcImageDest.right - rcDpiCorner.right;
	rcDest.bottom = rcImageDest.bottom - rcDpiCorner.bottom;

	rcSource.left = rcImageSource.left + rcImageCorners.left;
	rcSource.top = rcImageSource.top + rcImageCorners.top;
	rcSource.right = rcImageSource.right - rcImageCorners.right;
	rcSource.bottom = rcImageSource.bottom - rcImageCorners.bottom;
	if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
		if (!xtiled && !ytiled) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
		else if (xtiled && ytiled) {
			const LONG imageDrawWidth = rcImageSource.right - rcImageSource.left - rcImageCorners.left - rcImageCorners.right;
			const LONG imageDrawHeight = rcImageSource.bottom - rcImageSource.top - rcImageCorners.top - rcImageCorners.bottom;
			int iTimesX = (rcDest.right - rcDest.left) / (imageDrawWidth + nTiledMargin);
			if (!fullxtiled) {
				if ((rcDest.right - rcDest.left) % (imageDrawWidth + nTiledMargin) > 0) {
					iTimesX += 1;
				}
			}
			int iTimesY = (rcDest.bottom - rcDest.top) / (imageDrawHeight + nTiledMargin);
			if (!fullytiled) {
				if ((rcDest.bottom - rcDest.top) % (imageDrawHeight + nTiledMargin) > 0) {
					iTimesY += 1;
				}
			}
			for (int j = 0; j < iTimesY; ++j) {
				LONG lDestTop = rcDest.top + j * imageDrawHeight + j * nTiledMargin;
				LONG lDestBottom = lDestTop + imageDrawHeight;
				LONG lDrawHeight = imageDrawHeight;
				if (lDestBottom > rcDest.bottom) {
					lDrawHeight -= lDestBottom - rcDest.bottom;
					lDestBottom = rcDest.bottom;
				}
				for (int i = 0; i < iTimesX; ++i) {
					LONG lDestLeft = rcDest.left + i * imageDrawWidth + i * nTiledMargin;
					LONG lDestRight = lDestLeft + imageDrawWidth;
					LONG lDrawWidth = imageDrawWidth;
					if (lDestRight > rcDest.right) {
						lDrawWidth -= (lDestRight - rcDest.right);
						lDestRight = rcDest.right;
					}

					rcSource.left = rcImageSource.left + rcImageCorners.left;
					rcSource.top = rcImageSource.top + rcImageCorners.top;
					rcSource.right = rcSource.left + lDrawWidth;
					rcSource.bottom = rcSource.top + lDrawHeight;

					UiRect rcDestTemp;
					rcDestTemp.left = lDestLeft;
					rcDestTemp.right = lDestRight;
					rcDestTemp.top = lDestTop;
					rcDestTemp.bottom = lDestBottom;
					
					DrawFunction(m_hDC, m_bTransparent, rcDestTemp, hCloneDC, rcSource, bAlphaChannel, uFade);
				}
			}
		}
		else if (xtiled) { // supp
			const LONG imageDrawWidth = rcImageSource.right - rcImageSource.left - rcImageCorners.left - rcImageCorners.right;
			int iTimes = iTimes = (rcDest.right - rcDest.left) / (imageDrawWidth + nTiledMargin);
			if (!fullxtiled) {
				if ((rcDest.right - rcDest.left) % (imageDrawWidth + nTiledMargin) > 0) {
					iTimes += 1;
				}
			}

			for (int i = 0; i < iTimes; ++i) {
				LONG lDestLeft = rcDest.left + i * imageDrawWidth + i * nTiledMargin;
				LONG lDestRight = lDestLeft + imageDrawWidth;
				LONG lDrawWidth = imageDrawWidth;
				if (lDestRight > rcDest.right) {
					lDrawWidth -= (lDestRight - rcDest.right);
					lDestRight = rcDest.right;
				}

				//源区域：如果设置了边角，则仅包含中间区域
				rcSource.left = rcImageSource.left + rcImageCorners.left;
				rcSource.top = rcImageSource.top + rcImageCorners.top;
				rcSource.right = rcSource.left + lDrawWidth;
				rcSource.bottom = rcImageSource.bottom - rcImageCorners.bottom;

				UiRect rcDestTemp = rcDest;
				rcDestTemp.top = rcDest.top;
				rcDestTemp.bottom = rcDest.top + rcSource.GetHeight();
				rcDestTemp.left = lDestLeft;
				rcDestTemp.right = lDestRight;

				DrawFunction(m_hDC, m_bTransparent, rcDestTemp, hCloneDC, rcSource, bAlphaChannel, uFade);
			}
		}
		else { // ytiled
			const LONG imageDrawHeight = rcImageSource.bottom - rcImageSource.top - rcImageCorners.top - rcImageCorners.bottom;
			int iTimes = (rcDest.bottom - rcDest.top) / (imageDrawHeight + nTiledMargin);
			if (!fullytiled) {
				if ((rcDest.bottom - rcDest.top) % (imageDrawHeight + nTiledMargin) > 0) {
					iTimes += 1;
				}
			}

			UiRect rcDestTemp;
			rcDestTemp.left = rcDest.left;			

			for (int i = 0; i < iTimes; ++i) {
				LONG lDestTop = rcDest.top + i * imageDrawHeight + i*nTiledMargin;
				LONG lDestBottom = lDestTop + imageDrawHeight;
				LONG lDrawHeight = imageDrawHeight;
				if (lDestBottom > rcDest.bottom) {
					lDrawHeight -= lDestBottom - rcDest.bottom;
					lDestBottom = rcDest.bottom;
				}

				rcSource.left = rcImageSource.left + rcImageCorners.left;
				rcSource.top = rcImageSource.top + rcImageCorners.top;
				rcSource.right = rcImageSource.right - rcImageCorners.right;
				rcSource.bottom = rcSource.top + lDrawHeight;

				rcDestTemp.right = rcDest.left + rcSource.GetWidth();
				rcDestTemp.top = lDestTop;
				rcDestTemp.bottom = lDestBottom;
				
				DrawFunction(m_hDC, m_bTransparent, rcDestTemp, hCloneDC, rcSource, bAlphaChannel, uFade);
			}
		}
	}

	// left-top
	if (rcImageCorners.left > 0 && rcImageCorners.top > 0) {
		rcDest.left = rcImageDest.left;
		rcDest.top = rcImageDest.top;
		rcDest.right = rcImageDest.left + rcDpiCorner.left;
		rcDest.bottom = rcImageDest.top + rcDpiCorner.top;
		rcSource.left = rcImageSource.left;
		rcSource.top = rcImageSource.top;
		rcSource.right = rcImageSource.left + rcImageCorners.left;
		rcSource.bottom = rcImageSource.top + rcImageCorners.top;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// top
	if (rcImageCorners.top > 0) {
		rcDest.left = rcImageDest.left + rcDpiCorner.left;
		rcDest.top = rcImageDest.top;
		rcDest.right = rcImageDest.right - rcDpiCorner.right;
		rcDest.bottom = rcImageDest.top + rcDpiCorner.top;
		rcSource.left = rcImageSource.left + rcImageCorners.left;
		rcSource.top = rcImageSource.top;
		rcSource.right = rcImageSource.right - rcImageCorners.right;
		rcSource.bottom = rcImageSource.top + rcImageCorners.top;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// right-top
	if (rcImageCorners.right > 0 && rcImageCorners.top > 0) {
		rcDest.left = rcImageDest.right - rcDpiCorner.right;
		rcDest.top = rcImageDest.top;
		rcDest.right = rcImageDest.right;
		rcDest.bottom = rcImageDest.top + rcDpiCorner.top;
		rcSource.left = rcImageSource.right - rcImageCorners.right;
		rcSource.top = rcImageSource.top;
		rcSource.right = rcImageSource.right;
		rcSource.bottom = rcImageSource.top + rcImageCorners.top;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// left
	if (rcImageCorners.left > 0) {
		rcDest.left = rcImageDest.left;
		rcDest.top = rcImageDest.top + rcDpiCorner.top;
		rcDest.right = rcImageDest.left + rcDpiCorner.left;
		rcDest.bottom = rcImageDest.bottom - rcDpiCorner.bottom;
		rcSource.left = rcImageSource.left;
		rcSource.top = rcImageSource.top + rcImageCorners.top;
		rcSource.right = rcImageSource.left + rcImageCorners.left;
		rcSource.bottom = rcImageSource.bottom - rcImageCorners.bottom;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// right
	if (rcImageCorners.right > 0) {
		rcDest.left = rcImageDest.right - rcDpiCorner.right;
		rcDest.top = rcImageDest.top + rcDpiCorner.top;
		rcDest.right = rcImageDest.right;
		rcDest.bottom = rcImageDest.bottom - rcDpiCorner.bottom;
		rcSource.left = rcImageSource.right - rcImageCorners.right;
		rcSource.top = rcImageSource.top + rcImageCorners.top;
		rcSource.right = rcImageSource.right;
		rcSource.bottom = rcImageSource.bottom - rcImageCorners.bottom;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// left-bottom
	if (rcImageCorners.left > 0 && rcImageCorners.bottom > 0) {
		rcDest.left = rcImageDest.left;
		rcDest.top = rcImageDest.bottom - rcDpiCorner.bottom;
		rcDest.right = rcImageDest.left + rcDpiCorner.left;
		rcDest.bottom = rcImageDest.bottom;
		rcSource.left = rcImageSource.left;
		rcSource.top = rcImageSource.bottom - rcImageCorners.bottom;
		rcSource.right = rcImageSource.left + rcImageCorners.left;
		rcSource.bottom = rcImageSource.bottom;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// bottom
	if (rcImageCorners.bottom > 0) {
		rcDest.left = rcImageDest.left + rcDpiCorner.left;
		rcDest.top = rcImageDest.bottom - rcDpiCorner.bottom;
		rcDest.right = rcImageDest.right - rcDpiCorner.right;
		rcDest.bottom = rcImageDest.bottom;
		rcSource.left = rcImageSource.left + rcImageCorners.left;
		rcSource.top = rcImageSource.bottom - rcImageCorners.bottom;
		rcSource.right = rcImageSource.right - rcImageCorners.right;
		rcSource.bottom = rcImageSource.bottom;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}
	// right-bottom
	if (rcImageCorners.right > 0 && rcImageCorners.bottom > 0) {
		rcDest.left = rcImageDest.right - rcDpiCorner.right;
		rcDest.top = rcImageDest.bottom - rcDpiCorner.bottom;
		rcDest.right = rcImageDest.right;
		rcDest.bottom = rcImageDest.bottom;
		rcSource.left = rcImageSource.right - rcImageCorners.right;
		rcSource.top = rcImageSource.bottom - rcImageCorners.bottom;
		rcSource.right = rcImageSource.right;
		rcSource.bottom = rcImageSource.bottom;
		if (::IntersectRect(&rcTemp, &rcPaint, &rcDest)) {
			DrawFunction(m_hDC, m_bTransparent, rcDest, hCloneDC, rcSource, bAlphaChannel, uFade);
		}
	}

	::SetStretchBltMode(m_hDC, stretchBltMode);
	::SelectObject(hCloneDC, hOldBitmap);
	::DeleteDC(hCloneDC);
}

void Render_GdiPlus::DrawImageRect(const UiRect& rcPaint,
						           IBitmap* pBitmap, 
						           const UiRect& rcImageDest, 
						           UiRect rcImageSource, 
						           bool bBitmapDpiScaled,
						           uint8_t uFade,
		                           IMatrix* pMatrix)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	if (pMatrix == nullptr) {
		//仅在没有Matrix的情况下判断裁剪区域，
		//因为有Matrix时，实际绘制区域并不是rcImageDest，而是变换过后的位置，需要调整判断方法
		UiRect rcTestTemp;
		if (!::IntersectRect(&rcTestTemp, &rcImageDest, &rcPaint)) {
			return;
		}
	}	

	ASSERT(pBitmap != nullptr);
	if (pBitmap == nullptr) {
		return;
	}
	// 如果源位图已经按照DPI缩放过，那么对应的rcImageSource也需要缩放
	if ((rcImageSource.left < 0) ||
		(rcImageSource.top < 0) ||
		(rcImageSource.right < 0) ||
		(rcImageSource.bottom < 0)) {
		//如果是无效值，则重置为整个图片大小
		rcImageSource.left = 0;
		rcImageSource.top = 0;
		rcImageSource.right = pBitmap->GetWidth();
		rcImageSource.bottom = pBitmap->GetHeight();
	}
	else if (bBitmapDpiScaled) {
		//如果外部设置此值，做DPI自适应处理
		DpiManager::GetInstance()->ScaleRect(rcImageSource);
	}
	//图片源容错处理
	if (rcImageSource.left < 0) {
		rcImageSource.left = 0;
	}
	if (rcImageSource.top < 0) {
		rcImageSource.top = 0;
	}
	if (rcImageSource.right > (LONG)pBitmap->GetWidth()) {
		rcImageSource.right = pBitmap->GetWidth();
	}
	if (rcImageSource.bottom > (LONG)pBitmap->GetHeight()) {
		rcImageSource.bottom = pBitmap->GetHeight();
	}

	Gdiplus::Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	if (pMatrix != nullptr) {
		Matrix_Gdiplus* pGdiplusMatrix = dynamic_cast<Matrix_Gdiplus*>(pMatrix);
		if (pGdiplusMatrix != nullptr) {
			graphics.SetTransform(pGdiplusMatrix->GetMatrix());
		}
	}
		
	Gdiplus::RectF rectf;
	rectf.X = static_cast<Gdiplus::REAL>(rcImageDest.left);
	rectf.Y = static_cast<Gdiplus::REAL>(rcImageDest.top);
	rectf.Width = static_cast<Gdiplus::REAL>(rcImageDest.GetWidth());
	rectf.Height = static_cast<Gdiplus::REAL>(rcImageDest.GetHeight());

	Gdiplus::REAL srcx = (Gdiplus::REAL)rcImageSource.left;
	Gdiplus::REAL srcy = (Gdiplus::REAL)rcImageSource.top;
	Gdiplus::REAL srcWidth = (Gdiplus::REAL)rcImageSource.GetWidth();
	Gdiplus::REAL srcHeight = (Gdiplus::REAL)rcImageSource.GetHeight();
	Gdiplus::Unit srcUnit = Gdiplus::UnitPixel;

	void* data = pBitmap->LockPixelBits();
	Gdiplus::Bitmap gdiplusBitmap(pBitmap->GetWidth(),
							      pBitmap->GetHeight(), 
		                          pBitmap->GetWidth() * 4,
		                          PixelFormat32bppARGB,
		                          (BYTE*)data);
	if (uFade < 255) {
		//设置了透明度
		float alpha = (float)uFade / 255;
		Gdiplus::ColorMatrix cm = { 1,0,0,0,0,
									0,1,0,0,0,
									0,0,1,0,0,
									0,0,0,alpha,0,
									0,0,0,0,1 };
		Gdiplus::ImageAttributes imageAttr;
		imageAttr.SetColorMatrix(&cm);
		graphics.DrawImage(&gdiplusBitmap, rectf, srcx, srcy, srcWidth, srcHeight, srcUnit, &imageAttr);
	}
	else {
		graphics.DrawImage(&gdiplusBitmap, rectf, srcx, srcy, srcWidth, srcHeight, srcUnit);
	}	
	pBitmap->UnLockPixelBits();
}

void Render_GdiPlus::FillRect(const UiRect& rc, UiColor dwColor, uint8_t uFade)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	UiColor::ARGB dwNewColor = dwColor.GetARGB();
	if (uFade < 255) {
		//在原来颜色值的透明度基础上，在做一次透明度计算（uFade是在原来基础上在设置透明度）
		int alpha = dwColor.GetARGB() >> 24;
		dwNewColor = dwColor.GetARGB() & 0xffffff;
		alpha = static_cast<int>(static_cast<double>(alpha) * static_cast<double>(uFade) / 255.0);
		dwNewColor += alpha << 24;
	}

	Gdiplus::Graphics graphics(m_hDC);
	Gdiplus::Color color(dwNewColor);
	Gdiplus::SolidBrush brush(color);
	Gdiplus::RectF rcFill(static_cast<Gdiplus::REAL>(rc.left), static_cast<Gdiplus::REAL>(rc.top), 
		                  static_cast<Gdiplus::REAL>(rc.GetWidth()), static_cast<Gdiplus::REAL>(rc.GetHeight()));
	graphics.FillRectangle(&brush, rcFill);
}

void Render_GdiPlus::DrawLine(const UiPoint& pt1, const UiPoint& pt2, UiColor penColor, int nWidth)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	Gdiplus::Graphics graphics(m_hDC);
	Gdiplus::Pen pen(Gdiplus::Color(penColor.GetARGB()), (Gdiplus::REAL)nWidth);
	graphics.DrawLine(&pen, Gdiplus::Point(pt1.x, pt1.y), Gdiplus::Point(pt2.x, pt2.y));
}

void Render_GdiPlus::DrawRect(const UiRect& rc, UiColor penColor, int nWidth)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	Gdiplus::Graphics graphics(m_hDC);
	Gdiplus::Pen pen(Gdiplus::Color(penColor.GetARGB()), (Gdiplus::REAL)nWidth);
	graphics.DrawRectangle(&pen, rc.left, rc.top, rc.GetWidth(), rc.GetHeight());
}

void Render_GdiPlus::DrawRoundRect(const UiRect& rc, const UiSize& roundSize, UiColor penColor, int nWidth)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	Gdiplus::Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	Gdiplus::Pen pen(Gdiplus::Color(penColor.GetARGB()), (Gdiplus::REAL)nWidth);

	Gdiplus::GraphicsPath pPath;
	pPath.AddArc((INT)rc.left, rc.top, roundSize.cx, roundSize.cy, 180, 90);
	pPath.AddLine(rc.left + roundSize.cx, (INT)rc.top, rc.right - roundSize.cx, rc.top);
	pPath.AddArc(rc.right - roundSize.cx, (INT)rc.top, roundSize.cx, roundSize.cy, 270, 90);
	pPath.AddLine((INT)rc.right, rc.top + roundSize.cy, rc.right, rc.bottom - roundSize.cy);
	pPath.AddArc(rc.right - roundSize.cx, rc.bottom - roundSize.cy, (INT)roundSize.cx, roundSize.cy, 0, 90);
	pPath.AddLine(rc.right - roundSize.cx, (INT)rc.bottom, rc.left + roundSize.cx, rc.bottom);
	pPath.AddArc((INT)rc.left, rc.bottom - roundSize.cy, roundSize.cx, roundSize.cy, 90, 90);
	pPath.AddLine((INT)rc.left, rc.bottom - roundSize.cy, rc.left, rc.top + roundSize.cy);
	pPath.CloseFigure();

	graphics.DrawPath(&pen, &pPath);
}

void Render_GdiPlus::FillRoundRect(const UiRect& rc, const UiSize& roundSize, UiColor dwColor, uint8_t uFade)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	Gdiplus::Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	UiColor::ARGB dwNewColor = dwColor.GetARGB();
	if (uFade < 255) {
		//在原来颜色值的透明度基础上，在做一次透明度计算（uFade是在原来基础上在设置透明度）
		int alpha = dwColor.GetARGB() >> 24;
		dwNewColor = dwColor.GetARGB() & 0xffffff;
		alpha = static_cast<int>(static_cast<double>(alpha) * static_cast<double>(uFade) / 255.0);
		dwNewColor += alpha << 24;
	}
	Gdiplus::Color color(dwNewColor);
	Gdiplus::SolidBrush brush(color);

	Gdiplus::GraphicsPath pPath;
	pPath.AddArc((INT)rc.left, rc.top, roundSize.cx, roundSize.cy, 180, 90);
	pPath.AddLine(rc.left + roundSize.cx, (INT)rc.top, rc.right - roundSize.cx, rc.top);
	pPath.AddArc(rc.right - roundSize.cx, (INT)rc.top, roundSize.cx, roundSize.cy, 270, 90);
	pPath.AddLine((INT)rc.right, rc.top + roundSize.cy, rc.right, rc.bottom - roundSize.cy);
	pPath.AddArc(rc.right - roundSize.cx, rc.bottom - roundSize.cy, (INT)roundSize.cx, roundSize.cy, 0, 90);
	pPath.AddLine(rc.right - roundSize.cx, (INT)rc.bottom, rc.left + roundSize.cx, rc.bottom);
	pPath.AddArc((INT)rc.left, rc.bottom - roundSize.cy, roundSize.cx, roundSize.cy, 90, 90);
	pPath.AddLine((INT)rc.left, rc.bottom - roundSize.cy, rc.left, rc.top + roundSize.cy);
	pPath.CloseFigure();

	graphics.FillPath(&brush, &pPath);
}

void Render_GdiPlus::DrawArc(const UiRect& rc, float startAngle, float sweepAngle, bool /*useCenter*/, 
							 const IPen* pen,
	                         UiColor* gradientColor, const UiRect* gradientRect)
{
	ASSERT(pen != nullptr);
	if (pen == nullptr) {
		return;
	}
	Gdiplus::Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	Gdiplus::RectF rect((Gdiplus::REAL)rc.left, (Gdiplus::REAL)rc.top, (Gdiplus::REAL)(rc.right - rc.left), (Gdiplus::REAL)(rc.bottom - rc.top));
	
	if ((gradientColor == nullptr) || (gradientRect == nullptr)){
		graphics.DrawArc(((Pen_GdiPlus*)pen)->GetPen(), rect, startAngle, sweepAngle);
	}
	else {
		Gdiplus::RectF gradientRectF((Gdiplus::REAL)gradientRect->left, 
								     (Gdiplus::REAL)gradientRect->top,
								     (Gdiplus::REAL)(gradientRect->right - gradientRect->left),
									 (Gdiplus::REAL)(gradientRect->bottom - gradientRect->top));
		Gdiplus::LinearGradientBrush lgbrush(gradientRectF, pen->GetColor().GetARGB(), gradientColor->GetARGB(), Gdiplus::LinearGradientModeVertical);

		Gdiplus::REAL factors[4] = { 0.0f, 0.4f, 0.6f, 1.0f };
		Gdiplus::REAL positions[4] = { 0.0f, 0.2f, 0.8f, 1.0f };
		lgbrush.SetBlend(factors, positions, 4);

		Gdiplus::Pen fgPen(&lgbrush, static_cast<Gdiplus::REAL>(pen->GetWidth()));
		graphics.DrawArc(&fgPen, rect, startAngle, sweepAngle);
	}
}

void Render_GdiPlus::DrawPath(const IPath* path, const IPen* pen)
{
	ASSERT(path != nullptr);
	ASSERT(pen != nullptr);
	if ((path == nullptr) || (pen == nullptr)) {
		return;
	}
	Gdiplus::Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.DrawPath(((Pen_GdiPlus*)pen)->GetPen(), ((Path_Gdiplus*)path)->GetPath());
}

void Render_GdiPlus::FillPath(const IPath* path, const IBrush* brush)
{
	ASSERT(path != nullptr);
	ASSERT(brush != nullptr);
	if ((path == nullptr) || (brush == nullptr)) {
		return;
	}
	Gdiplus::Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.FillPath(((Brush_Gdiplus*)brush)->GetBrush(), ((Path_Gdiplus*)path)->GetPath());
}

void Render_GdiPlus::DrawString(const UiRect& rc, const std::wstring& strText,
	                            UiColor dwTextColor, const std::wstring& strFontId, uint32_t uFormat, uint8_t uFade /*= 255*/)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));
	ASSERT(::GetObjectType(m_hDC) == OBJ_DC || ::GetObjectType(m_hDC) == OBJ_MEMDC);
	if (strText.empty()) {
		return;
	}

	Gdiplus::Graphics graphics(m_hDC);
	Gdiplus::Font font(m_hDC, GlobalManager::GetFont(strFontId));

	Gdiplus::RectF rcPaint((Gdiplus::REAL)rc.left, (Gdiplus::REAL)rc.top, (Gdiplus::REAL)(rc.right - rc.left), (Gdiplus::REAL)(rc.bottom - rc.top));
	int alpha = dwTextColor.GetARGB() >> 24;
	uFade = static_cast<uint8_t>(uFade * static_cast<double>(alpha) / 255);
	if (uFade == 255) {
		uFade = 254;
	}
	Gdiplus::SolidBrush tBrush(Gdiplus::Color(uFade, dwTextColor.GetR(), dwTextColor.GetG(), dwTextColor.GetB()));

	Gdiplus::StringFormat stringFormat = Gdiplus::StringFormat::GenericTypographic();
	if ((uFormat & TEXT_END_ELLIPSIS) != 0) {
		stringFormat.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
	}
	if ((uFormat & TEXT_PATH_ELLIPSIS) != 0) {
		stringFormat.SetTrimming(Gdiplus::StringTrimmingEllipsisPath);
	}

	int formatFlags = Gdiplus::StringFormatFlagsLineLimit;
	if ((uFormat & TEXT_NOCLIP) != 0) {
		formatFlags |= Gdiplus::StringFormatFlagsNoClip;
	}
	if ((uFormat & TEXT_SINGLELINE) != 0) {
		formatFlags |= Gdiplus::StringFormatFlagsNoWrap;
	}
	stringFormat.SetFormatFlags(formatFlags);

	if ((uFormat & TEXT_CENTER) != 0) {
		stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
	}
	else if ((uFormat & TEXT_RIGHT) != 0) {
		stringFormat.SetAlignment(Gdiplus::StringAlignmentFar);
	}
	else {
		stringFormat.SetAlignment(Gdiplus::StringAlignmentNear);
	}

	if ((uFormat & TEXT_VCENTER) != 0) {
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	}
	else if ((uFormat & TEXT_BOTTOM) != 0) {
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentFar);
	}
	else {
		stringFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);
	}

	// try to fix font issue
#if 1
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSystemDefault);
#else
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
#endif
	graphics.DrawString(strText.c_str(), (int)strText.length(), &font, rcPaint, &stringFormat, &tBrush);
}

ui::UiRect Render_GdiPlus::MeasureString(const std::wstring& strText, const std::wstring& strFontId,
	uint32_t uFormat, int width /*= DUI_NOSET_VALUE*/)
{
	Gdiplus::Graphics graphics(m_hDC);
	Gdiplus::Font font(m_hDC, GlobalManager::GetFont(strFontId));
	Gdiplus::RectF bounds;

	Gdiplus::StringFormat stringFormat = Gdiplus::StringFormat::GenericTypographic();
	int formatFlags = 0;
	if ((uFormat & TEXT_SINGLELINE) != 0) {
		formatFlags |= Gdiplus::StringFormatFlagsNoWrap;
	}
	stringFormat.SetFormatFlags(formatFlags);

	if (width == DUI_NOSET_VALUE) {
		graphics.MeasureString(strText.c_str(), (int)strText.length(), &font, Gdiplus::PointF(), &stringFormat, &bounds);
	}
	else {
		Gdiplus::REAL height = 0;
		if ((uFormat & TEXT_SINGLELINE) != 0) {
			Gdiplus::RectF rcEmpty((Gdiplus::REAL)0, (Gdiplus::REAL)0, (Gdiplus::REAL)0, (Gdiplus::REAL)0);
			graphics.MeasureString(L"测试", 2, &font, rcEmpty, &stringFormat, &bounds);
			height = bounds.Height;
		}
		Gdiplus::RectF rcText((Gdiplus::REAL)0, (Gdiplus::REAL)0, (Gdiplus::REAL)width, height);
		graphics.MeasureString(strText.c_str(), (int)strText.length(), &font, rcText, &stringFormat, &bounds);
	}

	UiRect rc(int(bounds.GetLeft()), int(bounds.GetTop()), int(bounds.GetRight() + 1), int(bounds.GetBottom() + 1));
	return rc;
}

void Render_GdiPlus::DrawBoxShadow(const UiRect& rc, 
								   const UiSize& roundSize, 
						     	   const UiPoint& cpOffset, 
								   int nBlurRadius, 
								   int nSpreadRadius,
								   UiColor dwColor)
{
	ASSERT((GetWidth() > 0) && (GetHeight() > 0));

	ASSERT(dwColor.GetARGB() != 0);

	ui::UiRect destRc = rc;
	destRc.Offset(cpOffset);
	destRc.left -= nSpreadRadius;
	destRc.top -= nSpreadRadius;
	destRc.right += nSpreadRadius;
	destRc.bottom += nSpreadRadius;

	Gdiplus::RectF srcRc(0.0f, 0.0f, (Gdiplus::REAL)destRc.GetWidth(), (Gdiplus::REAL)destRc.GetHeight());
	Gdiplus::RectF excludeRc(std::abs(nSpreadRadius) * 1.0f,
							 std::abs(nSpreadRadius) * 1.0f,
		                     srcRc.Width - std::abs(nSpreadRadius) * 2.0f,
		                     srcRc.Height - std::abs(nSpreadRadius) * 2.0f);

	Gdiplus::GraphicsPath shadowPath;
	Gdiplus::GraphicsPath excludePath;

	auto add_roundcorner_path = [](Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, const UiSize& roundSize) {
		path.AddArc(rect.GetLeft(), rect.GetTop(), (Gdiplus::REAL)roundSize.cx, (Gdiplus::REAL)roundSize.cy, 180, 90);
		path.AddLine(rect.GetLeft() + (Gdiplus::REAL)roundSize.cx, rect.GetTop(), rect.GetRight() - (Gdiplus::REAL)roundSize.cx, rect.GetTop());
		path.AddArc(rect.GetRight() - (Gdiplus::REAL)roundSize.cx, rect.GetTop(), (Gdiplus::REAL)roundSize.cx, (Gdiplus::REAL)roundSize.cy, 270, 90);
		path.AddLine(rect.GetRight(), rect.GetTop() + (Gdiplus::REAL)roundSize.cy, rect.GetRight(), rect.GetBottom() - (Gdiplus::REAL)roundSize.cy);
		path.AddArc(rect.GetRight() - (Gdiplus::REAL)roundSize.cx, rect.GetBottom() - (Gdiplus::REAL)roundSize.cy, (Gdiplus::REAL)roundSize.cx, (Gdiplus::REAL)roundSize.cy, 0, 90);
		path.AddLine(rect.GetRight() - (Gdiplus::REAL)roundSize.cx, rect.GetBottom(), rect.GetLeft() + (Gdiplus::REAL)roundSize.cx, rect.GetBottom());
		path.AddArc(rect.GetLeft(), rect.GetBottom() - (Gdiplus::REAL)roundSize.cy, (Gdiplus::REAL)roundSize.cx, (Gdiplus::REAL)roundSize.cy, 90, 90);
		path.AddLine(rect.GetLeft(), rect.GetBottom() - (Gdiplus::REAL)roundSize.cy, rect.GetLeft(), rect.GetTop() + (Gdiplus::REAL)roundSize.cy);
		path.CloseFigure();
	};

	add_roundcorner_path(shadowPath, srcRc, roundSize);	
	add_roundcorner_path(excludePath, excludeRc, roundSize);
		
	
	Gdiplus::PathGradientBrush gradientPathBrush(&shadowPath);
	gradientPathBrush.SetWrapMode(Gdiplus::WrapMode::WrapModeClamp);

	Gdiplus::Color colors[] = { Gdiplus::Color::Transparent,Gdiplus::Color(dwColor.GetARGB()) ,Gdiplus::Color(dwColor.GetARGB()) };
	Gdiplus::REAL pos[] = { 0.0f,0.7f,1.0f };

	gradientPathBrush.SetInterpolationColors(colors, pos, 3);

	Gdiplus::Bitmap tempBitmap(static_cast<INT>(srcRc.Width), static_cast<INT>(srcRc.Height));
	Gdiplus::Graphics tempRender(&tempBitmap);

	Gdiplus::Region excludeRegion(&excludePath);
	tempRender.ExcludeClip(&excludeRegion);
	tempRender.FillPath(&gradientPathBrush, &shadowPath);

	// blur effect
	Gdiplus::BlurParams blurParams;
	blurParams.expandEdge = false;
	blurParams.radius = static_cast<float>(nBlurRadius);

	Gdiplus::Blur blurEffect;
	blurEffect.SetParameters(&blurParams);

	RECT rcBlurEffect{ 2 * nSpreadRadius, 2 * nSpreadRadius,static_cast<LONG>(srcRc.Width) - 2 * nSpreadRadius,static_cast<LONG>(srcRc.Height) - 2 * nSpreadRadius };
	tempBitmap.ApplyEffect(&blurEffect, &rcBlurEffect);

	Gdiplus::Graphics graphics(m_hDC);
	graphics.DrawImage(&tempBitmap,
					   Gdiplus::RectF(static_cast<Gdiplus::REAL>(destRc.left), static_cast<Gdiplus::REAL>(destRc.top), 
									  static_cast<Gdiplus::REAL>(destRc.GetWidth()), static_cast<Gdiplus::REAL>(destRc.GetHeight())),
					   0, 0, srcRc.Width, srcRc.Height,
		               Gdiplus::UnitPixel);
}

} // namespace ui
