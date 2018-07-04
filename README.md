# Gravity Waves Application

A cross-platform, gavity waves simulation visualisation developed by
[Snepo Research](https://www.snepo.com) using the [Cinder Framework](https://cinder.org). This software was commissioned by Museums Victoria for an interactive digital installation at *Beyond Perception: Seeing the Unseen*, a permanent exhibition at Scienceworks in Melbourne, Australia which opened in May 2018.

![Gravity Waves Screenshot](https://scienceworks.s3.amazonaws.com/documentation/gravitywaves.png)

The application was designed to run on a high performance Windows 10 PC connected to a HD projector. 
Features of the application include:

- Depth sensing camera detects touch/pressure on the fabric which doubles as projection surface
- Simulation is optimised and tested on NVidia GTX 1080 Graphics card
- Network OSC messages are sent to the Spacial Audio Server which manages sound output
- All simulation parameters are exposed via an admin GUI that can be accessed using a mouse
- The simulation uses gravity to model black holes attracting each other on a 2D plane
- When black holes are close to each other for a period they lock into a spin and waves are emitted

## Building App

C++ is difficult to compile (for legacy reasons) and has no official build system. Snepo has made a platform specific project for mac and windows. To successfully build the app check out cinder, check out the project, open the visual studio solution on windows or Xcode project on mac and press build. 

Cinder 0.9.1 needs to be at the same folder level as the project. 

```
git clone --recursive https://github.com/cinder/Cinder.git
cd cinder
git checkout release_v0.9.1
```

The folder structure will look like this, where the cinder folder has been checked out using the above steps. 

![Build Folders](https://scienceworks.s3.amazonaws.com/documentation/build-folders.png)

On windows Snepo used visual studio community 2017, but it will probably work with 2015 too. (Platform toolset v140+, c++11/14)

## Museum Setup

### Prerequisites

All required external software is in the Dropbox/cosmicevents/Install These folder. The two dlls must live alongside the main executable (GravityWavesV3.exe), and the KinectRuntime-x64.msi runtime needs to be installed prior to running. The VC redistributable may be required too, depending on what’s preinstalled on the machine.


### Process Management

We use StayUp to monitor running processes Fluid.exe. I’ve written a small batch file for each that goes into the windows startup folder. Its job is to make sure that dropbox is started and to launch and monitor the app’s process. StayUp.exe needs to be set as “run as administrator” as it uses some of the windows system events to track things like heap allocations and memory working set, etc. This can be disabled in a pinch. The main invocation of StayUp looks like this.

```
start /D %HOMEDRIVE%%HOMEPATH%\Dropbox\scienceworks-build\Gravity StayUp.exe "Gavity.exe" -e -i 3600 -t 10     
      ^ "Working directory" to launch from                                     ^            ^  ^       ^
                                                            Process to monitor +            |  |       |
                                                                            Event logging  -+  |       |
                                                                     Logging interval (secs) -+|       |
                                                             Seconds to wait for a process to respond -+
                                                            before considering it dead and restarting
```

## Technical Overview

### Configuration 

The configruation file is located at assets/Config.json

Each of the configuration options is documented in the marked up example below. 

```
{
   "AllowPhysicalCollisions" : true,                     // Do blackholes have a hard "core" that prevents them from overlapping
   "AnnabelPest" : false,                                // Determines how to render the Slope map as a debug overlay (windowed or fullscreen)
   "CoolRate" : 0.99000000953674316,                     // The rate at which a black hole "cools" when not being collided with. Must be < 1 (multiplicative operation)
   "DrawHeatMask" : false,                               // Debug draw the heat mask
   "DrawPointAttractors" : false,                        // Debug draw point attractors
   "DrawSlopeField" : false,                             // Debug draw the slope field vectors
   "DrawSlopeMap" : false,                               // Debug draw the generated slope map/texture
   "EndgameParticleEmissionRate" : 0,                    // How many particles per frame to emit when playing "endgame"
   "HeatRate" : 1.1000000238418579,                      // The rate at which a black hole "heats" when colliding with another. Must be > 1 (multiplicative operation)
   "MaxBlackHoles" : 3,                                  // How many black holes to spawn
   "MaxBlackholeMass" : 20000,                           // Maximum mass of a black hole
   "MaxBlackholeRadius" : 100,                           // Maximum radius of a black hole
   "MinBlackholeMass" : 15000,                           // Minimum mass of a black hole
   "MinBlackholeRadius" : 70,                            // Minimum radius of a black hole
   "OSCEndpoint" : "136.154.24.245",                     // Endpoint of the audio machine receiving OSC packets
   "OSCPort" : 9000,                                     // Port for the audio machine 
   "OSCSendAllPacked" : true,                            // Pack all audio state into single message or not? (See network communication)
   "OSCSendFrameDelay" : 1,                              // Send the audio packets every {OSCSendDelay} frames
   "ParticleEmissionRate" : 0,                           // Rate to emit particles from blackholes
   "ParticleNoiseMultiplier" : 0.10000000149011612,      // How much is the particle trajectory affected by perlin noise
   "ParticleSpawnThreshold" : 5,                         // Blackhole must be travelling more than SpawnThreshold pixels per frame to emit particles
   "PeakAttractorSampleMass" : 10000,                    // Mass of faux attractors generated at peaks of the SlopeMap
   "PeakAttractorSampleSpacing" : 31.000001907348633,    // Pixels at which to sample the SlopeMap for peaks. Visualise with DrawPointAttractors
   "PressednessThreshold" : 0.090000003576278687,        // Amount the fabric needs to be distorted / pressed to be considered to be "engaged"
   "PullConstant" : 1.3999999761581421,                  // Magic number for the motion of blackholes
   "RadialOverlapAllowance" : 0.60200005769729614,       // If AllowPhysicalCollisions, how much (in percentage/100.0) can blackholes overlap before considered colliding
   "ReactToSlopeField" : true,                           // If true, black holes will be affected by the distortion of the fabric
   "RigidCollisionResponse" : true,                      // Rather than forcing non overlapping of blackholes, they can collide more elastically. Looks terrible
   "RotationPower" : 0,                                  // How much to radially distort the Space image 
   "ScreenID" : 0,                                       // The ID of the screen. Sent to the audio machine via OSC
   "SlopeGenMaxDepth" : 1,                               // Filtering of the depth image. Anything > SlopeGenMaxDepth is ignored
   "SlopeGenMinDepth" : 0.01900000125169754,             // Filtering of the depth image. Anything < SlopeGenMinDepth is ignored
   "SlopeGenPeakThreshold" : 0.01900000125169754,        // Filtering of depth when generating the slope map. Anything > SlopeGenPeakThreshold is ignored
   "SlopeGenVelocityMultiplier" : 0.19800001382827759    // How much is a black hole's velocity affected by the slope map
}
```

### Network Communication

**Packed Mode**
In order to keep network traffic down, the gravity waves app can pack all values into a single OSC message. Whether or not this is used is determined by ${Config.OSCSendAllPacked}. N.B The alternative may no longer support non-packed messages but that is outside of our control All data is sent to the following OSC address: 

```
kOSCEndpoint = "/bp/cosmic/${Config.ScreenID}/blackholes”;
```

The arguments sent to the endpoint are alternating velocities and positions of the blackholes. The velocities are sent in the range 0 to 1, and the positions are sent in the range -1 to 1, with -1 being all the way to the left of screen and 1 being the rightmost edge. 

**Greedy Mode (should be considered deprecated)**
Packets are sent as individual messages over the OSC transport. They are sent in the same ranges as in packed mode, but with individual addresses for each value. These addresses are constructed as follows:

```
kBHVelocityAddress = "/bp/cosmic/${Config.ScreenID}/blackhole${BlackholeIndex}"
kBHPositionAddress = "/bp/cosmic/${Config.ScreenID}/blackhole${BlackholeIndex}_x”
```

Where BlackHole index goes from 1 to ${Config.MaxBlackHoles}. So to control the x position of the first black hole on the first screen, the full address would be:

```
kFirstBHPositionAddress = "/bp/cosmic/0/blackhole1_x"
                            Screen ID ^          ^
                                  Black hole ID -+
```

Separately, a message is also sent to the address "/bp/cosmic/${Config.ScreenID}/collision” when two black holes collide.

### Admin Panels

Always accessible via the ` key. (tilde / backquote). Requires a mouse to be interacted with, however.

**Admin Panels**

These map 1 to 1 with the settings in the config file (explained above).

![Admin Panel](https://scienceworks.s3.amazonaws.com/documentation/gravity-admin.png)

