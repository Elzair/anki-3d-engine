#ifndef RSRC_ASYNC_LOADING_REQS_HANDLER_H
#define RSRC_ASYNC_LOADING_REQS_HANDLER_H

#include <string>
#include <boost/ptr_container/ptr_list.hpp>
#include "anki/core/AsyncLoader.h"
#include "anki/resource/Image.h"
#include "anki/resource/MeshData.h"
#include "anki/resource/RsrcLoadingRequests.h"


// Dont even think to include these files:
class Texture;
class Mesh;


/// Handles the loading requests on behalf of the resource manager. Its a
/// different class because we want to keep the source of the manager clean
class RsrcAsyncLoadingReqsHandler
{
	public:
		uint getFrameServedRequestsNum() const
		{
			return frameServedRequestsNum;
		}

		/// Send a loading request to an AsyncLoader
		/// @tparam Type It should be Texture or Mesh
		/// @param filename The file to load
		/// @param objToLoad Pointer to a pointer to the object to load
		/// asynchronously
		template<typename Type>
		void sendNewLoadingRequest(const char* filename, Type** objToLoad);
		
		/// Serve the finished requests. This should be called once every loop
		/// of the main loop
		/// @param maxTime The max time to spend serving finished requests. If
		/// for example there are many that need more time than the max the
		/// method will return. The pending requests will be served when it
		/// will be called again.
		/// In seconds
		void postProcessFinishedRequests(float maxTime);

		/// Get the number of total pending requests
		size_t getRequestsNum() const {return requests.size();}
	
	private:
		//AsyncLoader al; ///< Asynchronous loader
		boost::ptr_list<RsrcLoadingRequestBase> requests; ///< Loading requests
		/// The number of served requests for this frame
		uint frameServedRequestsNum;
};


#endif