//
//  KinectDepthCamera.h
//  GravityWavesV2
//
//  Created by Andrew Wright on 27/11/17.
//

#ifndef KinectDepthCamera_h
#define KinectDepthCamera_h

#include <Video/DepthCamera.h>
#include "Kinect2.h"

namespace Video
{
	class KinectDepthCamera : public DepthCamera
	{
	public:

		KinectDepthCamera			( const Format& format );

		const char *                GetName			( ) const override { return "KinectDepthCamera"; }
		
	protected:

		bool                        PrivateInit		( ) override;
		void                        PrivateTick		( ) override;
		bool                        PrivateStart	( ) override;
		void                        PrivateStop		( ) override;
		void                        PrivateShutdown	( ) override;

		Kinect2::DeviceRef			_device;
	};
}

#endif