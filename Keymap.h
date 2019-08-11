#ifndef KEYMAP_H
#define KEYMAP_H

#include <map>
#include <vector>
#include <string>

namespace Input
{
	struct Keymap
	{
		std::vector<std::pair<std::string, int>> keys;
		
		Keymap();
	};
}

#endif