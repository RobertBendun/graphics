#ifndef GRAPHICS_HH
#define GRAPHICS_HH

#include <algorithm>
#include <span>
#include <tuple>
#include <fstream>
#include <ostream>

namespace graphics
{
	using identity = decltype([](auto &&value) -> decltype(auto) { return value; });

	constexpr uint32_t encode_rgb(uint8_t r, uint8_t g, uint8_t b)
	{
		return r | (g << 8) | (b << (2 * 8)) | 0xff'00'00'00;
	}

	std::tuple<uint8_t, uint8_t, uint8_t> decode_rgb(uint32_t color)
	{
		return {
			(color & 0x000000FF) >> (8*0),
			(color & 0x0000FF00) >> (8*1),
			(color & 0x00FF0000) >> (8*2),
		};
	}

	namespace math
	{
		template<typename Vec>
		concept vector = requires (Vec const& vec)
		{
			{ std::get<0>(vec) };
		};

		template<typename S>
		concept scalar = std::is_arithmetic_v<S>;


		template<typename ...T>
		constexpr unsigned vector_min_size = std::min({
			std::conditional_t<
				vector<T>,
				std::tuple_size<T>,
				std::integral_constant<std::size_t, (~std::size_t{})>
			>::value...
		});

		template<typename binary_operator, typename Lhs, typename Rhs>
		auto reduce(Lhs const& lhs, Rhs const& rhs) requires (vector<Lhs>) || (vector<Rhs>)
		{
			constexpr auto N = vector_min_size<Lhs, Rhs>;

			return [&]<std::size_t ...I>(std::index_sequence<I...>)
			{
				if constexpr (vector<Lhs> && !vector<Rhs>) {
					return std::array { binary_operator{}(std::get<I>(lhs), rhs)... };
				} else if constexpr (!vector<Lhs> && vector<Rhs>) {
					return std::array { binary_operator{}(lhs, std::get<I>(rhs))... };
				} else {
					return std::array { binary_operator{}(std::get<I>(lhs), std::get<I>(rhs))... };
				}
			}(std::make_index_sequence<N>{});
		}

		namespace operators
		{
			template<typename Lhs, typename Rhs>
			auto operator+(Lhs const& lhs, Rhs const& rhs) requires (vector<Lhs>) || (vector<Rhs>)
			{
				return reduce<std::plus<void>>(lhs, rhs);
			}

			template<typename Lhs, typename Rhs>
			auto operator-(Lhs const& lhs, Rhs const& rhs) requires (vector<Lhs>) || (vector<Rhs>)
			{
				return reduce<std::minus<void>>(lhs, rhs);
			}

			template<typename Lhs, typename Rhs>
			auto operator*(Lhs const& lhs, Rhs const& rhs) requires (vector<Lhs>) || (vector<Rhs>)
			{
				return reduce<std::multiplies<void>>(lhs, rhs);
			}

			template<typename Lhs, typename Rhs>
			auto operator/(Lhs const& lhs, Rhs const& rhs) requires (vector<Lhs>) || (vector<Rhs>)
			{
				return reduce<std::divides<void>>(lhs, rhs);
			}

			template<vector Vector>
			std::ostream& operator<<(std::ostream& os, Vector const& vec)
			{
				return [&]<std::size_t ...I>(std::index_sequence<I...>) -> decltype(auto)
				{
					((void)(os << (I > 0 ? ", " : "(") << std::get<I>(vec)), ...);
					return os << ")";
				}(std::make_index_sequence<std::tuple_size_v<Vector>>{});
			}
		}
	}

	template<typename Data>
	struct canvas
	{
		using value_type = std::decay_t<decltype(std::declval<Data>()[0])>;

		Data data;
		unsigned width, height, stride = width;

		void fill(value_type const& new_value)
		{
			for (auto cursor = 0u, h = height; --h < height; cursor += stride) {
				std::fill(&data[cursor], &data[cursor] + width, new_value);
			}
		}

		template<typename Point, typename Functor = identity>
		auto subcanvas_view(Point &&p1, Point &&p2, Functor getxy = Functor{}) -> canvas<std::span<value_type>>
		requires (std::tuple_size_v<std::decay_t<decltype(getxy(p1))>> == 2)
			    && (std::convertible_to<unsigned, std::tuple_element_t<0, std::decay_t<decltype(getxy(p1))>>>)
			    && (std::convertible_to<unsigned, std::tuple_element_t<1, std::decay_t<decltype(getxy(p1))>>>)
		{
			auto [x1, y1, x2, y2] = std::tuple_cat(getxy(p1), getxy(p2));
			if (x1 == x2 && y1 == y2) return {};
			if (x1 > x2) std::swap(x1, x2);
			if (y1 > y2) std::swap(y1, y2);

			return {
				{ &data[y1 * stride + x1], &data[0] + height * stride  },
				unsigned(x2 - x1 + 1),
				unsigned(y2 - y1 + 1),
				stride
			};
		}

		void save_as_ppm(auto const& filename, auto &&rgb = identity{}) const
		requires (std::tuple_size_v<std::decay_t<decltype(rgb(std::declval<value_type const&>()))>> == 3)
			    && (std::same_as<uint8_t, std::tuple_element_t<0, std::decay_t<decltype(rgb(std::declval<value_type const&>()))>>>)
			    && (std::same_as<uint8_t, std::tuple_element_t<1, std::decay_t<decltype(rgb(std::declval<value_type const&>()))>>>)
			    && (std::same_as<uint8_t, std::tuple_element_t<2, std::decay_t<decltype(rgb(std::declval<value_type const&>()))>>>)
		{
			std::ofstream out(filename, std::ios_base::out | std::ios_base::binary);
			out << "P6\n" << width << ' ' << height << "\n255\n";

			for (auto y = 0u; y < height; ++y) {
				for (auto x = 0u; x < width; ++x) {
					auto [r, g, b] = rgb(data[y * stride + x]);
					out.put(r).put(g).put(b);
				}
			}
		}
	};

	namespace colors
	{
		constexpr uint32_t red   = encode_rgb(255,   0,   0);
		constexpr uint32_t green = encode_rgb(0,   255,   0);
		constexpr uint32_t blue  = encode_rgb(0,     0, 255);

		namespace gruvbox
		{
			inline namespace dark
			{
				constexpr uint32_t bg = 0xff'282828;
				constexpr uint32_t fg = 0xff'b2dbeb;

				constexpr uint32_t red    = 0xff'3449fb;
				constexpr uint32_t green  = 0xff'26bbb8;
				constexpr uint32_t yellow = 0xff'2fbdfa;
				constexpr uint32_t blue   = 0xff'98a583;
				constexpr uint32_t purple = 0xff'9b86d3;
				constexpr uint32_t aqua   = 0xff'7cc08e;
				constexpr uint32_t gray   = 0xff'748392;
				constexpr uint32_t orange = 0xff'1980fe;
			}
		}
	}
}

#endif
