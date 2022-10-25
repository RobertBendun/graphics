#include "graphics.hh"
#include <array>
#include <iostream>

constexpr unsigned Size = 600;
constexpr unsigned Cell = 40;

int main()
{
	using namespace graphics::math::operators;

	graphics::canvas canvas { std::array<uint32_t, Size * Size>{}, Size, Size };

	canvas.fill(graphics::colors::gruvbox::bg);

	for (unsigned j = 0; j < Size / Cell - 1; ++j) {
		for (unsigned i = 0; i < Size / Cell - 1; ++i) {
			auto p1 = std::array{i, j} * Cell;
			auto p2 = (std::array{i, j} + 1) * Cell;
			canvas.subcanvas_view(p1, p2).fill(
				(i + j) % 2 == 0
					? graphics::colors::gruvbox::bg
					: graphics::colors::gruvbox::fg
			);
		}
	}

	canvas.save_as_ppm("result.ppm", graphics::decode_rgb);
}
