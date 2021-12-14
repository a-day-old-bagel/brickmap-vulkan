#pragma warning( disable : 4100 ) // unreferenced formal parameter

// types
#include <limits>
#include <string>
#include <cctype>
#include <sstream>
#include <optional>
#include <cstdint>
#include <fstream>

// containers
#include <iterator>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <array>
#include <deque>

// utilities
#include <iostream>
#include <cassert>
#include <cmath>
#include <memory>
#include <functional>
#include <algorithm>
#include <utility>
#include <tuple>
#include <stdexcept>
#include <format>

// concurrence
#include <mutex>

// math
#define GLM_FORCE_RADIANS
//#define GLM_FORCE_LEFT_HANDED 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// vulkan
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

// profile
#include <Tracy.hpp>
#include <TracyVulkan.hpp>

// logging
#include <spdlog/spdlog.h>

#include "vulkan/initializers.h"

#define VMA_CHECK(x)                                            \
	do                                                          \
	{                                                           \
		VkResult result{ x };                                   \
		if ( result != VK_SUCCESS )                             \
		{                                                       \
			spdlog::critical( "VMA Error: {}", result );        \
		}                                                       \
	} while (0)

#define VK_CHECK(x)                                                     	\
	do                                                                  	\
	{                                                                   	\
		vk::Result result{ x };                                             \
		if ( result != vk::Result::eSuccess )                               \
		{                                                               	\
			spdlog::critical( "Detected Vulkan error: {}", result );        \
		}                                                               	\
	} while (0)
