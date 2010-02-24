#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include "common.h"
#include "engine_class.h"
#include "util.h"


// forward decls
class Texture;
class Material;
class ShaderProg;
class Mesh;
class Skeleton;
class SkelAnim;
class LightProps;

namespace rsrc {
template< typename Type > class Container;
}


/**
 * Every class that it is considered resource should be derivative of this one. This step is not
 * necessary because of the Container template but ensures that loading will be made by the
 * resource manager and not the class itself
 */
class Resource
{
	PROPERTY_R( string, path, getPath );
	PROPERTY_R( string, name, getName );
	PROPERTY_R( uint, usersNum, getUsersNum );

	// friends
	friend class rsrc::Container<Texture>;
	friend class rsrc::Container<Material>;
	friend class rsrc::Container<ShaderProg>;
	friend class rsrc::Container<Skeleton>;
	friend class rsrc::Container<Mesh>;
	friend class rsrc::Container<SkelAnim>;
	friend class rsrc::Container<LightProps>;
	friend class ShaderProg;

	public:
		virtual bool load( const char* ) = 0;
		virtual void unload() = 0;

		Resource(): usersNum(0) {}
		virtual ~Resource() {};
};


/// resource namespace
namespace rsrc {


extern Container<Texture>     textures;
extern Container<ShaderProg> shaders;
extern Container<Material>    materials;
extern Container<Mesh>        meshes;
extern Container<Skeleton>    skeletons;
extern Container<SkelAnim>   skel_anims;
extern Container<LightProps> light_props;


/// resource container template class
template<typename Type> class Container: public Vec<Type*>
{
	private:
		typedef typename Container<Type>::iterator Iterator; ///< Just to save me time from typing
		typedef Vec<Type*> BaseClass;

		/**
		 * Search inside the container by name
		 * @param name The name of the resource
		 * @return The iterator of the content end of vector if not found
		 */
		Iterator findByName( const char* name )
		{
			Iterator it = BaseClass::begin();
			while( it != BaseClass::end() )
			{
				if( (*it).name == name )
					return it;
				++it;
			}

			return it;
		}


		/**
		 * Search inside the container by name and path
		 * @param name The name of the resource
		 * @param path The path of the resource
		 * @return The iterator of the content end of vector if not found
		 */
		Iterator findByNameAndPath( const char* name, const char* path )
		{
			Iterator it = BaseClass::begin();
			while( it != BaseClass::end() )
			{
				if( (*it)->name == name && (*it)->path == path )
					return it;
				++it;
			}

			return it;
		}


		/**
	   * Search inside the container by pointer
		 * @param name The name of the resource object
		 * @return The iterator of the content end of vector if not found
		 */
		Iterator findByPtr( Type* ptr )
		{
			Iterator it = BaseClass::begin();
			while( it != BaseClass::end() )
			{
				if( ptr == (*it) )
					return it;
				++it;
			}

			return it;
		}

		public:
		/**
		 * load an object and register it. If its allready loaded return its pointer
		 * @param fname The filename that initializes the object
		 * @return A pointer of a new resource or NULL on fail
		 */
		Type* load( const char* fname )
		{
			char* name = util::CutPath( fname );
			string path = util::GetPath( fname );
			Iterator it = findByNameAndPath( name, path.c_str() );

			// if allready loaded then inc the users and return the pointer
			if( it != BaseClass::end() )
			{
				++ (*it)->usersNum;
				return (*it);
			}

			// else create new, loaded and update the container
			Type* new_instance = new Type();
			new_instance->name = name;
			new_instance->path = path;
			new_instance->usersNum = 1;

			if( !new_instance->load( fname ) )
			{
				ERROR( "Cannot load \"" << fname << '\"' );
				delete new_instance;
				return NULL;
			}
			BaseClass::push_back( new_instance );

			return new_instance;
		}


		/**
		 * unload item. If nobody else uses it then delete it completely
		 * @param x Pointer to the instance we want to unload
		 */
		void unload( Type* x )
		{
			Iterator it = findByPtr( x );
			if( it == BaseClass::end() )
			{
				ERROR( "Cannot find resource with pointer 0x" << x );
				return;
			}

			Type* del_ = (*it);
			DEBUG_ERR( del_->usersNum < 1 ); // WTF?

			--del_->usersNum;

			// if no other users then call unload and update the container
			if( del_->usersNum == 0 )
			{
				del_->unload();
				delete del_;
				BaseClass::erase( it );
			}
		}
}; // end class Container


} // end namespace
#endif