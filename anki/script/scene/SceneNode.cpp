#include "anki/script/ScriptCommon.h"
#include "anki/scene/SceneNode.h"


WRAP(SceneNode)
{
	class_<SceneNode, noncopyable>("SceneNode", no_init)
		.def("getSceneNodeName", &SceneNode::getSceneNodeName,
			return_value_policy<copy_const_reference>())
	;
}