#include "ExperimentImages.h"

#include "RenderFiles.h"
#include "stbimage/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {

namespace fs = std::filesystem;
constexpr int tile_width = 240;
constexpr int image_height = 148;
constexpr int label_height = 18;
constexpr int maximum_label_lines = 4;
constexpr int gutter = 8;
constexpr int page_columns = 4;
constexpr int page_cells = 24;
constexpr int maximum_dimension = 4096;
constexpr std::int64_t maximum_pixels = 8 * 1024 * 1024;

PreviewImage thumbnail(const PreviewImage &source)
{
	if (!source.isValid()) return {};
	const double scale = std::min(static_cast<double>(tile_width) / source.width(),
	                              static_cast<double>(image_height) / source.height());
	const int width = std::max(1, static_cast<int>(source.width() * scale));
	const int height = std::max(1, static_cast<int>(source.height() * scale));
	std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 3);
	for (int row = 0; row < height; ++row) {
		for (int column = 0; column < width; ++column) {
			const int x = static_cast<int>((column + 0.5) * source.width() / width);
			const int y = static_cast<int>((row + 0.5) * source.height() / height);
			const auto offset = (static_cast<std::size_t>(y) * source.width() + x) * 3;
			const auto target = (static_cast<std::size_t>(row) * width + column) * 3;
			std::copy_n(source.rgbPixels().data() + offset, 3, pixels.data() + target);
		}
	}
	return {width, height, std::move(pixels)};
}

class ContactSheetCanvas {
public:
	ContactSheetCanvas(int width, int height) : width_(width), height_(height)
	{
		BITMAPINFO bitmap{};
		bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmap.bmiHeader.biWidth = width_;
		bitmap.bmiHeader.biHeight = -height_;
		bitmap.bmiHeader.biPlanes = 1;
		bitmap.bmiHeader.biBitCount = 32;
		bitmap.bmiHeader.biCompression = BI_RGB;
		context_ = CreateCompatibleDC(nullptr);
		void *bits = nullptr;
		if (context_) surface_ = CreateDIBSection(context_, &bitmap, DIB_RGB_COLORS, &bits, nullptr, 0);
		pixels_ = static_cast<std::uint32_t *>(bits);
		if (!surface_ || !pixels_) {
			release();
			throw std::runtime_error("Cannot create a Windows contact-sheet canvas.");
		}
		previous_surface_ = SelectObject(context_, surface_);
		font_ = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
		if (font_) previous_font_ = SelectObject(context_, font_);
		std::fill_n(pixels_, static_cast<std::size_t>(width_) * height_, 0x101820u);
		SetBkMode(context_, TRANSPARENT);
	}

	~ContactSheetCanvas() { release(); }
	ContactSheetCanvas(const ContactSheetCanvas &) = delete;
	ContactSheetCanvas &operator=(const ContactSheetCanvas &) = delete;

	void drawCell(int left, int top, int tile_height, const PreviewImage &image, const std::string &label)
	{
		for (int row = top; row < top + tile_height; ++row) {
			std::fill_n(pixels_ + static_cast<std::size_t>(row) * width_ + left, tile_width, 0x24313bu);
		}
		if (image.isValid()) {
			const int x = left + (tile_width - image.width()) / 2;
			const int y = top + (image_height - image.height()) / 2;
			for (int row = 0; row < image.height(); ++row) {
				for (int column = 0; column < image.width(); ++column) {
					const auto *rgb = image.rgbPixels().data() + (static_cast<std::size_t>(row) * image.width() + column) * 3;
					pixels_[static_cast<std::size_t>(y + row) * width_ + x + column] =
						(static_cast<std::uint32_t>(rgb[0]) << 16) | (static_cast<std::uint32_t>(rgb[1]) << 8) | rgb[2];
				}
			}
		} else {
			drawText({left + 10, top + 50, left + tile_width - 10, top + image_height}, L"Render unavailable", RGB(255, 172, 142));
		}
		std::size_t start = 0;
		int line_top = top + image_height + 4;
		while (start < label.size() && line_top + label_height <= top + tile_height) {
			const auto newline = label.find('\n', start);
			const auto line = label.substr(start, newline == std::string::npos ? newline : newline - start);
			drawText({left + 8, line_top, left + tile_width - 8, line_top + label_height},
			         {line.begin(), line.end()}, RGB(238, 245, 250));
			if (newline == std::string::npos) break;
			start = newline + 1;
			line_top += label_height;
		}
		// GDI and direct writes share this DIB. Flush before writing another cell.
		GdiFlush();
	}

	PreviewImage toImage() const
	{
		const auto count = static_cast<std::size_t>(width_) * height_;
		std::vector<std::uint8_t> rgb(count * 3);
		for (std::size_t index = 0; index < count; ++index) {
			rgb[index * 3] = static_cast<std::uint8_t>(pixels_[index] >> 16);
			rgb[index * 3 + 1] = static_cast<std::uint8_t>(pixels_[index] >> 8);
			rgb[index * 3 + 2] = static_cast<std::uint8_t>(pixels_[index]);
		}
		return {width_, height_, std::move(rgb)};
	}

private:
	void drawText(RECT box, const std::wstring &text, COLORREF color)
	{
		SetTextColor(context_, color);
		DrawTextW(context_, text.c_str(), static_cast<int>(text.size()), &box,
		          DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
	}

	void release()
	{
		if (previous_font_) SelectObject(context_, previous_font_);
		if (previous_surface_) SelectObject(context_, previous_surface_);
		if (font_) DeleteObject(font_);
		if (surface_) DeleteObject(surface_);
		if (context_) DeleteDC(context_);
	}

	int width_;
	int height_;
	HDC context_ = nullptr;
	HBITMAP surface_ = nullptr;
	HFONT font_ = nullptr;
	HGDIOBJ previous_surface_ = nullptr;
	HGDIOBJ previous_font_ = nullptr;
	std::uint32_t *pixels_ = nullptr;
};

}

PreviewImage read_experiment_image(const fs::path &path)
{
	const auto bytes = render_files::read_text(path);
	if (bytes.empty() || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
		throw std::runtime_error("Cannot read image " + path.u8string());
	}
	const auto *buffer = reinterpret_cast<const stbi_uc *>(bytes.data());
	const auto length = static_cast<int>(bytes.size());
	int width = 0, height = 0, channels = 0;
	if (!stbi_info_from_memory(buffer, length, &width, &height, &channels) ||
	    width < 1 || height < 1 || width > maximum_dimension || height > maximum_dimension ||
	    static_cast<std::int64_t>(width) * height > maximum_pixels) {
		throw std::runtime_error("Unsupported reference image or dimensions: " + path.u8string());
	}
	const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
		stbi_load_from_memory(buffer, length, &width, &height, &channels, STBI_rgb), stbi_image_free);
	if (!pixels) throw std::runtime_error("Cannot decode reference image " + path.u8string());
	const auto size = static_cast<std::size_t>(width) * height * 3;
	return {width, height, {pixels.get(), pixels.get() + size}};
}

void write_experiment_png(const fs::path &path, const PreviewImage &image)
{
	if (!image.isValid()) throw std::runtime_error("Invalid experiment image.");
	std::ofstream stream(path, std::ios::binary);
	if (!stream) throw std::runtime_error("Cannot write image " + path.u8string());
	const auto write = [](void *context, void *data, int length) {
		static_cast<std::ofstream *>(context)->write(static_cast<const char *>(data), length);
	};
	const int encoded = stbi_write_png_to_func(write, &stream, image.width(), image.height(), 3,
	                                           image.rgbPixels().data(), image.width() * 3);
	stream.close();
	if (!encoded || !stream) throw std::runtime_error("Cannot encode image " + path.u8string());
}

ExperimentOutput::ExperimentOutput(const fs::path &destination)
{
	const auto absolute = fs::absolute(destination).lexically_normal();
	if (fs::exists(fs::symlink_status(absolute))) {
		throw std::runtime_error("Choose a new experiment output directory: " + absolute.u8string());
	}
	fs::create_directories(absolute.parent_path());
	const auto parent = fs::canonical(absolute.parent_path());
	destination_ = parent / absolute.filename();
	staging_ = parent / (L".progen3d-experiment-" + render_files::unique_suffix() + L".tmp");
	if (!fs::create_directory(staging_)) throw std::runtime_error("Cannot stage the experiment directory.");
}

ExperimentOutput::~ExperimentOutput()
{
	if (staging_.empty()) return;
	std::error_code ignored;
	const auto actual = fs::weakly_canonical(staging_, ignored);
	if (!ignored && actual == staging_ && actual.parent_path() == destination_.parent_path()) {
		fs::remove_all(actual, ignored);
	}
}

void ExperimentOutput::publish()
{
	render_files::move_into_place(staging_, destination_);
	staging_.clear();
}

ExperimentContactSheet::ExperimentContactSheet(fs::path directory) : directory_(std::move(directory))
{
}

void ExperimentContactSheet::add(std::string label, const PreviewImage &image)
{
	page_.push_back({std::move(label), thumbnail(image)});
	if (page_.size() == page_cells) writePage();
}

std::vector<std::string> ExperimentContactSheet::finish()
{
	if (!page_.empty()) writePage();
	return files_;
}

void ExperimentContactSheet::writePage()
{
	const int count = static_cast<int>(page_.size());
	const int columns = std::min(count, page_columns);
	const int rows = (count + columns - 1) / columns;
	int label_lines = 1;
	for (const auto &cell : page_) {
		label_lines = std::max(label_lines, 1 + static_cast<int>(std::count(cell.label.begin(), cell.label.end(), '\n')));
	}
	const int tile_height = image_height + std::min(label_lines, maximum_label_lines) * label_height + 8;
	ContactSheetCanvas canvas(columns * (tile_width + gutter) + gutter, rows * (tile_height + gutter) + gutter);
	for (int index = 0; index < count; ++index) {
		const auto &cell = page_[index];
		canvas.drawCell(gutter + (index % columns) * (tile_width + gutter),
		                gutter + (index / columns) * (tile_height + gutter), tile_height, cell.thumbnail, cell.label);
	}
	const std::string name = files_.empty() ? "contact-sheet.png" : "contact-sheet-" + std::to_string(files_.size() + 1) + ".png";
	write_experiment_png(directory_ / name, canvas.toImage());
	files_.push_back(name);
	page_.clear();
}
