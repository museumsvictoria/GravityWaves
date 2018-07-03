//
//  KinectDepthCamera.cxx
//  GravityWavesV2
//
//  Created by Andrew Wright on 27/11/17.
//

#include <Video/Win32/KinectDepthCamera.h>

using namespace ci;

namespace Video
{
	KinectDepthCamera::KinectDepthCamera(const Format& format)
	: DepthCamera(format)
	{

	}

	bool KinectDepthCamera::PrivateInit()
	{
		_device = Kinect2::Device::create();
		return true;
	}

	void KinectDepthCamera::PrivateTick()
	{

	}

	bool KinectDepthCamera::PrivateStart()
	{
		_device->connectDepthEventHandler([&](const Kinect2::DepthFrame& frame)
		{
			if (!_device) return;
			
			DepthFrame d;
			if ( false )
			{ 
				u32 border = 32;
				auto bounds = frame.getChannel()->getBounds();
				bounds.x1 += border; bounds.x2 -= border;
				bounds.y1 += border; bounds.y2 -= border;

				d.FrameData = Channel16u::create(frame.getChannel()->clone(bounds, true));
			}
			else
			{
				d.FrameData = frame.getChannel();
			}

			NotifyFrame(d);
		});

		_device->start();
		return true;
	}

	void KinectDepthCamera::PrivateStop()
	{
		_device->disconnectDepthEventHandler();
		_device->stop();
	}

	void KinectDepthCamera::PrivateShutdown()
	{
		
	}
}