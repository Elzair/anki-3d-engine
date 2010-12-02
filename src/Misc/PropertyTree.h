#ifndef PROPERTY_TREE_H
#define PROPERTY_TREE_H

#include <boost/optional.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include "Math.h"


namespace PropertyTree {

/// Get a bool from a ptree or throw exception if not found or incorrect.
/// Get something like this: @code<tag>true</tag>@endcode
/// @param[in] pt The ptree
/// @param[in] tag The name of tha tag
extern bool getBool(const boost::property_tree::ptree& pt, const char* tag);

/// Optionaly get a bool from a ptree and throw exception incorrect.
/// Get something like this: @code<tag>true</tag>@endcode
/// @param[in] pt The ptree
/// @param[in] tag The name of tha tag
extern boost::optional<bool> getBoolOptional(const boost::property_tree::ptree& pt, const char* tag);

/// Get a @code<float>0.0</float>@endcode
extern float getFloat(const boost::property_tree::ptree& pt);

/// Get a @code<vec3><x>0.0</x><y>0.0</y><z>0.0</z></vec3>@endcode
extern Vec3 getVec3(const boost::property_tree::ptree& pt);

} // end namespace

#endif