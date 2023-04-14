#include "SvgUtil.h"
#include "duilib/Utils/StringUtil.h"
#include "duilib/Utils/DpiManager.h"
#include "duilib/Render/Bitmap.h"
#include "duilib/Core/Image.h"

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "duilib/Render/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "duilib/Render/nanosvgrast.h"

namespace ui
{

class SvgDeleter
{
public:
	inline void operator()(NSVGimage * x) const { nsvgDelete(x); }
};

class RasterizerDeleter
{
public:
	inline void operator()(NSVGrasterizer * x) const { nsvgDeleteRasterizer(x); }
};

bool SvgUtil::IsSvgFile(const std::wstring& strImageFullPath)
{
	auto len = strImageFullPath.length();
	if (len < 4)
		return false;
	
	size_t pos = strImageFullPath.rfind(L".");
	if (pos != std::wstring::npos) {
		std::wstring ext = strImageFullPath.substr(pos, std::wstring::npos);
		return StringHelper::MakeLowerString(ext) == L".svg";
	}

	return false;
}

std::unique_ptr<ui::ImageInfo> SvgUtil::LoadSvg(const std::wstring& strImageFullPath)
{
	std::string strOut;
	StringHelper::UnicodeToMBCS(strImageFullPath, strOut);

	NSVGimage *svg = nsvgParseFromFile(strOut.c_str(), "px", 96.0f);
	return LoadImageBySvg(svg, strImageFullPath);
}

std::unique_ptr<ui::ImageInfo> SvgUtil::LoadSvg(std::vector<unsigned char>& file_data, const std::wstring& strImageFullPath)
{
	if (file_data.empty()) {
		return nullptr;
	}
	bool has_appended = false;
	if (file_data.back() != '\0') {
		//确保是含尾0的字符串，避免越界访问内存
		file_data.push_back('\0');
		has_appended = true;
	}
	char* pData = (char*)file_data.data();
	NSVGimage* svg = nsvgParse(pData, "px", 96.0f);
	if (has_appended) {
		file_data.pop_back();
	}
	return LoadImageBySvg(svg, strImageFullPath);	
}

std::unique_ptr<ui::ImageInfo> SvgUtil::LoadImageBySvg(void *data, const std::wstring& strImageFullPath)
{
	std::unique_ptr<NSVGimage, SvgDeleter> svg((NSVGimage*)data);
	if (!svg)
		return nullptr;

	int w = (int)svg->width;
	int h = (int)svg->height;
	std::unique_ptr<NSVGrasterizer, RasterizerDeleter> rast(nsvgCreateRasterizer());
	if (w == 0 || h == 0 || rast == NULL)
		return nullptr;

	float scale = (float)DpiManager::GetInstance()->GetScale() / 100;
	w = static_cast<int>(w*scale);
	h = static_cast<int>(h*scale);

	unsigned char* pBmpBits = NULL;
	HDC hdc = GetDC(NULL);
	HBITMAP hBitmap = GdiBitmap::CreateDIBBitmap(hdc, w, h, true, (LPVOID*)&pBmpBits);
	ReleaseDC(NULL, hdc);

	if (!hBitmap) {
		//DWORD err = GetLastError();
		ASSERT(FALSE);
		return nullptr;
	}
	nsvgRasterize(rast.get(), svg.get(), 0, 0, scale, pBmpBits, w, h, w * 4);

	// nanosvg内部已经做过alpha预乘，这里只做R和B的交换
	for (int y = 0; y < h; y++) {
		unsigned char *row = &pBmpBits[y*w*4];
		for (int x = 0; x < w; x++) {
			int r = row[0], g = row[1], b = row[2], a = row[3];
			(void)a;
			(void)g;
// 			if (a < 255) {
// 				row[0] = (unsigned char)(b * a / 255);
// 				row[1] = (unsigned char)(g * a / 255);
// 				row[2] = (unsigned char)(r * a / 255);
// 			}
// 			else {
				row[0] = static_cast<unsigned char>(b);
				//row[1] = g;
				row[2] = static_cast<unsigned char>(r);
//			}
			row += 4;
		}
	}

	std::unique_ptr<ImageInfo> imageInfo(new ImageInfo);
	imageInfo->nX = w;
	imageInfo->nY = h;
	imageInfo->sImageFullPath = strImageFullPath;
	imageInfo->SetAlpha(true);
	imageInfo->SetSvg(true);
	imageInfo->PushBackHBitmap(hBitmap);
	return imageInfo;
}

} // namespace nbase
