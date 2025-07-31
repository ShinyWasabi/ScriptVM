#pragma once

namespace rage
{
	inline constexpr char ToLower(char c)
	{
		return c >= 'A' && c <= 'Z' ? c | 1 << 5 : c;
	}

	constexpr std::uint32_t Joaat(const std::string_view str)
	{
		std::uint32_t hash = 0;

		for (auto c : str)
		{
			hash += ToLower(c);
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}

		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);

		return hash;
	}
}