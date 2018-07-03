//
//  GravityWavesApp.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#include "GravityWavesApp.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "cinder/Timeline.h"
#include "CinderImGui.h"

#include <Common/Constants.h>

using namespace ci;
using namespace ci::app;
using namespace std;

static float kSlopeFieldOverallPressedness = 0.0f;
static std::vector<float> kBlackholeVelocities;
static std::vector<float> kBlackholeXPositions;

void GravityWavesApp::setup()
{
    randSeed(clock());
    ui::initialize();
    
    #ifdef CINDER_MSW
    // Disable crash dialog
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    #endif

    hideCursor();
    
    _rippleShader = gl::GlslProg::create( loadAsset ( "Shaders/Passthrough.vs.glsl" ), loadAsset ( "Shaders/Ripple.fs.glsl" ) );
    _particles = std::make_unique<ParticleSystem>();
    _heatMask = Channel8u::create( loadImage ( loadAsset( "Textures/HeatMask.png" ) ) );
    
    {
        auto tFmt = gl::Texture::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).magFilter(GL_LINEAR).wrap(GL_MIRRORED_REPEAT).internalFormat(GL_RGB16F);
        
        auto fmt = gl::Fbo::Format().samples(8).disableDepth().colorTexture( tFmt );
        _rippleBuffer = gl::Fbo::create( getWindowWidth(), getWindowHeight(), fmt );
        
        gl::ScopedFramebuffer buffer { _rippleBuffer };
        gl::clear();
    }
    
    _depthCamera = Video::CreateDepthCamera();
    _depthCamera->OnFrame.connect( [=] ( const Video::DepthFrame& frame )
    {
#ifdef CINDER_MSW
        auto format = GL_R16F;
#else
        auto format = GL_R32F;
#endif
        _slopeGenerator->Generate( gl::Texture::create ( *frame.FrameData, gl::Texture::Format().internalFormat(format) ) );
        _slopeGenerator->Swap();
    } );
    _depthCamera->Start();
    _slopeGenerator = std::make_unique<Video::SlopeFieldGenerator> ( ivec2 ( 512, 424 ) );

    {
        auto tFmt = gl::Texture::Format().minFilter(GL_LINEAR).magFilter(GL_LINEAR).wrap(GL_MIRRORED_REPEAT);
        _backgroundImage = gl::Texture::create(loadImage(app::loadAsset("Textures/galaxy3.png")), tFmt);
        _galaxyBuffer = gl::Fbo::create(app::getWindowWidth(), app::getWindowHeight(), gl::Fbo::Format().samples(0).colorTexture(tFmt));
    }

    _stars = std::make_shared<Video::VideoPlayer>();
    _stars->Loops(true);
    _stars->Play ( app::getAssetPath("Video/Space.mp4").string() );
    
    _blackholes.resize ( K::MaxBlackHoles );
    
    kBlackholeVelocities.resize ( K::MaxBlackHoles );
    kBlackholeXPositions.resize ( K::MaxBlackHoles );

    {
        auto fmt = gl::GlslProg::Format().define ( "MAX_BLACK_HOLES", std::to_string ( _blackholes.size() ) );
        fmt.vertex ( app::loadAsset ( "Shaders/BlackHole.vs.glsl" ) );
        fmt.fragment ( app::loadAsset ( "Shaders/BlackHole.fs.glsl" ) );
        fmt.fragDataLocation ( 0, "FinalColor" );
        
        _blackHoleShader = gl::GlslProg::create ( fmt );
        _blackHoleShader->uniform ( "uAspectRatio", 1.0f / app::getWindowAspectRatio() );
    }
    
    static const std::string kDefaultFileName = "Default.json";
    
    if ( fs::exists( getAssetDirectories().front() / kDefaultFileName ) )
    {
        try
        {
            JsonTree tree { loadAsset ( kDefaultFileName ) };
            Marshal ( tree );
        }catch ( const std::exception& e )
        {
            std::cout << e.what() << std::endl;
        }
    }
    
    K::OSCPrefix = "/bp/cosmic/" + std::to_string ( K::ScreenID ) + "/";
}

float GravityWavesApp::HeatMultiplierAt ( const ci::vec2& position ) const
{
    vec2 n = position / vec2 ( getWindowSize() );
    return (float)_heatMask->getValue( n * vec2 ( _heatMask->getSize() ) ) / 255.0f;
}

void GravityWavesApp::keyDown ( KeyEvent event )
{
    switch ( event.getChar() )
    {
        case '`' :
        {
            _renderUI = !_renderUI;
            if ( !_renderUI )
            {
                hideCursor();
            }else
            {
                showCursor();
            }
            break;
        }
    }
}

void GravityWavesApp::update()
{
    static double kBefore = getElapsedSeconds();
    double now = getElapsedSeconds();
    double dt = now - kBefore;
    kBefore = now;
    dt = 1.0f;
    
    _stars->Update();
    _particles->Update ( 1.0 / 60.0f );
    
    for ( int i = 0; i < _blackholes.size(); i++ )
    {
        auto& a = _blackholes[i];
        a.Heat *= K::CoolRate;
        a.Heat = glm::clamp(a.Heat, 0.01f, 1.0f);
        
        if ( glm::length( a.Velocity ) > _particles->SpawnThreshold )
        {
            for ( auto& p : a.History )
            {
                _particles->Emit( _particles->EmissionRate, p, vec2(0) );
            }
        }
        
        for ( int j = i + 1; j < _blackholes.size(); j++ )
        {
            auto& b = _blackholes[j];
            a.Interact(b, dt);
        }
    }

    for ( auto& a : _attractors )
    {
        for ( auto& b : _blackholes )
        {
            b.Interact(a, dt);
        }
    }
    
    if ( K::AllowCollisions )
    {
        for ( int i = 0; i < _blackholes.size(); i++ )
        {
            for ( int j = i + 1; j < _blackholes.size(); j++ )
            {
                auto& a = _blackholes[i];
                auto& b = _blackholes[j];
                
                if ( a.IsInEndGame || b.IsInEndGame ) continue;
                
                float d = glm::distance(a.Position, b.Position);
                float inner = ( ( a.Radius + b.Radius ) * K::CollisionAllowance );
                if ( d < inner )
                {
                    a.Heat *= K::HeatRate;
                    b.Heat *= K::HeatRate;
                    
                    a.Heat *= HeatMultiplierAt ( a.Position );
                    b.Heat *= HeatMultiplierAt ( b.Position );
                    
                    float maxHeat = std::max ( a.Heat, b.Heat );
                    
                    a.Heat = maxHeat;
                    b.Heat = maxHeat;
                    
                    float overlap = inner - d;

                    vec2 delta = a.Position - b.Position;
                    float angle = std::atan2(delta.y, delta.x);

                    if ( K::HardCollisions )
                    {
                        a.Position.x += std::cos ( angle ) * overlap;
                        a.Position.y += std::sin ( angle ) * overlap;

                        b.Position.x -= std::cos ( angle ) * overlap;
                        b.Position.y -= std::sin ( angle ) * overlap;
                    }else
                    {
                        a.Velocity.x += std::cos ( angle ) * overlap * (100.0f / b.Mass);
                        a.Velocity.y += std::sin ( angle ) * overlap * (100.0f / b.Mass);

                        b.Velocity.x -= std::cos ( angle ) * overlap * (100.0f / a.Mass);
                        b.Velocity.y -= std::sin ( angle ) * overlap * (100.0f / a.Mass);
                    }
                }
            }
        }
    }
    
    
    for ( auto& b : _blackholes )
    {
        b.Tick ( dt );
        
        if ( b.Heat >= 1.0f )
        {
            CreateEndgame ( b );
        }
    }
    
    if ( _endgame )
    {
        for ( auto& b : _endgame->Blackholes )
        {
            _particles->Emit( K::EmissionRateDuringEndgame, b->Position, vec2(0) );
        }
        
        _endgame->Tick( 1.0 / 60.0f );
        
        if ( _endgame->Dead() )
        {
            _endgame->Explode();
            _endgame = nullptr;
        }
    }
}

template <typename T>
inline void Write_ ( JsonTree& tree, const std::string& name, const T& p )
{
    tree.pushBack ( JsonTree ( name, p ) );
}

void GravityWavesApp::Serialize ( JsonTree& tree )
{
    K::Save ( tree );
    
    #define W(n, t) Write_(tree, n, t)
    
    W("SlopeGenMinDepth", _slopeGenerator->MinDepth);
    W("SlopeGenMaxDepth", _slopeGenerator->MaxDepth);
    W("SlopeGenPeakThreshold", _slopeGenerator->PeakThreshold);
    
    W("ParticleEmissionRate", _particles->EmissionRate);
    W("ParticleSpawnThreshold", _particles->SpawnThreshold);
    W("ParticleNoiseMultiplier", _particles->NoiseMultiplier);
    
    #undef W
}

void GravityWavesApp::Marshal ( const JsonTree& tree )
{
    K::Load ( tree );
    
    #define R(n, v) if(tree.hasChild(n)) v = tree[n].getValue<decltype(v)>()
    
    R("SlopeGenMinDepth", _slopeGenerator->MinDepth);
    R("SlopeGenMaxDepth", _slopeGenerator->MaxDepth);
    R("SlopeGenPeakThreshold", _slopeGenerator->PeakThreshold);
    
    R("ParticleEmissionRate", _particles->EmissionRate);
    R("ParticleSpawnThreshold", _particles->SpawnThreshold);
    R("ParticleNoiseMultiplier", _particles->NoiseMultiplier);
    
    _oscSender = std::make_unique<OscSender>( K::OSCPort, K::OSCEndpoint );
    
    #undef R
}

void GravityWavesApp::CreateEndgame ( const E::Blackhole& master )
{
    if ( _endgame ) return;
    
    if ( _oscSender )
    {
        _oscSender->Send( K::OSCPrefix + "collision" );
    }
    
    std::vector<E::Blackhole *> capture;
    vec2 v;
    
    int maxIndex = 0;
    float maxVelocityMag = 0.0f;
    vec2 maxVelocity;
    
    for ( auto& a : _blackholes )
    {
        float d = glm::distance ( a.Position, master.Position );
        if ( d < K::EndgameCaptureRadius )
        {
            v += a.Position;
            capture.push_back ( &a );
            a.BaseRadius = a.Radius;
            a.IsInEndGame = true;
            
            float l = glm::length2( a.Velocity );
            if ( l > maxVelocityMag )
            {
                maxVelocity = a.Velocity;
                maxVelocityMag = l;
                maxIndex = capture.size() - 1;
            }
            
            a.Velocity = vec2(0);
        }
    }
    
    float spinDir = 1.0f;
    
    {
        auto n = glm::normalize(maxVelocity);
        float rotationAngle = 0;
        if ( std::abs ( n.x ) > std::abs ( n.y ) )
        {
            rotationAngle = n.x;
        }else
        {
            rotationAngle = -n.y;
        }
        
        if ( rotationAngle < 0 )
        {
            spinDir = -1.0f;
        }else
        {
            spinDir = 1.0f;
        }
    }
    
    v /= (float)capture.size();
    
    for ( auto& b : capture )
    {
        vec2 d = b->Position - v;
        b->BaseDistance = glm::distance( b->Position, v );
        b->Angle = std::atan2(d.y, d.x);
    }
    
    _endgame = std::make_unique<E::Endgame>(capture);
    _endgame->Position = v;
    _endgame->SpinDirection = spinDir;
    
    K::RippleAlpha.stop();
    K::RippleAmplitude.stop();
    K::RippleSpeed.stop();
    
    K::RippleAlpha = 0.0f;
    K::RippleAmplitude = 0.0f;
    K::RippleSpeed = 0.0f;
    
    app::timeline().apply( &K::RippleAlpha, 2.0f, 10.0f, EaseOutQuint() ).delay(3.0f);
    app::timeline().apply( &K::RippleAmplitude, 70.0f, 6.0f, EaseOutQuint() ).delay(3.0f);
    app::timeline().apply( &K::RippleSpeed, 20.0f, 8.0f, EaseOutQuint() ).delay(3.0f);
}

void GravityWavesApp::draw()
{
    gl::clear(Color(0, 0, 0));
    vec2 windowSize = getWindowSize();

    {
        gl::ScopedFramebuffer fbo{ _galaxyBuffer };
        gl::clear();

        gl::ScopedColor color{ ColorAf::white() };
        gl::draw(_backgroundImage, _galaxyBuffer->getBounds());

        if (_stars)
        {
            gl::ScopedBlendAdditive blend;
            _stars->Draw (_galaxyBuffer->getBounds());
        }
    }

    #pragma mark region blackhole rendering
    {
        gl::ScopedFramebuffer buffer { _rippleBuffer };
        {
            gl::draw ( _galaxyBuffer->getColorTexture(), app::getWindowBounds() );
            
            gl::ScopedGlslProg shader { _blackHoleShader };
            gl::ScopedTextureBind tex0 { _galaxyBuffer->getColorTexture(), 0 };

            _blackHoleShader->uniform ( "uRotationPower", K::RotationPower );
            _blackHoleShader->uniform ( "uPullConstant", K::PullConstant );

            int ctr = 0;
            for ( auto& b : _blackholes )
            {
                vec2 offset = vec2 ( b.Position.x, b.Position.y );
                vec2 position = offset / vec2(_galaxyBuffer->getSize());
                position.y = 1.0f - position.y;
                float radius = b.Radius / (float)_galaxyBuffer->getWidth() * 7.0f;

                std::string name = "uBlackHoles[" + std::to_string(ctr) + "].";

                _blackHoleShader->uniform ( name + "Radius", radius );
                _blackHoleShader->uniform ( name + "Strength", b.Strength );
                _blackHoleShader->uniform ( name + "Position", position );

                ctr++;
            }

            gl::drawSolidRect ( app::getWindowBounds() );
        }
    }
    
    gl::draw ( _rippleBuffer->getColorTexture(), getWindowBounds() );
    Surface slopeSurface = _slopeGenerator->GetSlope()->createSource();
    
    {
        gl::ScopedGlslProg shader { _rippleShader };
        gl::ScopedTextureBind tex0 { _rippleBuffer->getColorTexture() };
        
        if ( _endgame )
        {
            float t = ( _endgame->Life() );
            t = lerp ( 3.0f, 0.0f, t );

            vec2 uv = _endgame->Position / windowSize;
            uv.y = 1.0f - uv.y;
            _rippleShader->uniform( "uEpicenter", uv );
            _rippleShader->uniform( "uTime", t );
            _rippleShader->uniform( "uAlpha", 1.0f );
            _rippleShader->uniform( "uRadiusOfEffect", K::RippleAlpha() );
            
        }else
        {
            _rippleShader->uniform( "uEpicenter", vec2 ( 0.5f ) );
            _rippleShader->uniform( "uTime", 0.0f );
            _rippleShader->uniform( "uAlpha", 0.0f );
            _rippleShader->uniform( "uRadiusOfEffect", 0.0f );
        }
        
        gl::drawSolidRect( getWindowBounds() );
    }
    
    #pragma mark region particle system
    _particles->Draw();

    auto biasUV = [](const vec2& uv)
    {
        return uv;
        vec2 r = uv;
        float scale = lerp(1.0f, 3.0f, easeOutQuad(r.y));
        r.y = std::pow(r.y, scale);
        return r;
    };
    
    if ( K::ReactToVectorField )
    {
        for ( auto& b : _blackholes )
        {
            vec2 normalisedPos = ( b.Position ) / windowSize;
            normalisedPos = biasUV ( normalisedPos );
            
            ivec2 p = ivec2 ( normalisedPos * vec2 ( slopeSurface.getSize() ) );
            ColorAf c = slopeSurface.getPixel( p );
            
            vec2 v { c.r, c.g };
            v = v * 2.0f - 1.0f;
            
            b.Velocity += v * K::VelocityMultiplier;
        }
        
        if ( _endgame )
        {
            vec2 normalisedPos = ( _endgame->Position ) / windowSize;
            normalisedPos = biasUV ( normalisedPos );
            
            ivec2 p = ivec2 ( normalisedPos * vec2 ( slopeSurface.getSize() ) );
            ColorAf c = slopeSurface.getPixel( p );
            
            vec2 v { c.r, c.g };
            v = v * 2.0f - 1.0f;
            
            _endgame->Velocity += v * K::VelocityMultiplier;
        }

        if ( K::DrawField )
        {
            gl::VertBatch batch ( GL_LINES );
            gl::VertBatch b2 ( GL_POINTS );
            gl::pointSize(3.0f);
            
            for ( int x = 0; x < slopeSurface.getWidth(); x += 5 )
            {
                for ( int y = 0; y < slopeSurface.getHeight(); y += 5 )
                {
                    vec2 na = vec2 ( x, y ) / vec2( slopeSurface.getSize() ) ;
                    na = biasUV ( na );
                    
                    ColorAf c = slopeSurface.getPixel( na * vec2 ( slopeSurface.getSize() ) );
                    
                    vec2 v { c.r, c.g };
                    v = v * 2.0f - 1.0f;
                    
                    vec2 a = na * windowSize;
                    vec2 b = a + v * 10.0f;
                    
                    b2.vertex ( a );
                    
                    batch.vertex( a );
                    batch.vertex( b );
                }
            }
            
            batch.draw();
            gl::ScopedColor c { Colorf ( 1, 0, 0 ) };
            b2.draw();
        }
    }
    
    {
        int pad = 80;
        
        std::vector<E::Attractor> attractors;
        
        for ( int x = pad; x < slopeSurface.getWidth() - pad; x += (int)K::AttractorSampleSpacing )
        {
            for ( int y = pad; y < slopeSurface.getHeight() - pad; y += (int)K::AttractorSampleSpacing )
            {
                vec2 na = vec2 ( x, y ) / vec2( slopeSurface.getSize() ) ;
                na = biasUV ( na );
                
                ColorAf c = slopeSurface.getPixel( na * vec2 ( slopeSurface.getSize() ) );
                
                if ( c.b > 0.7f )
                {
                    E::Attractor a;
                    a.Position = na * windowSize;
                    a.Mass = K::AttractorSampleMass;
                    
                    attractors.push_back( a );
                }
            }
        }

        gl::ScopedColor c { Colorf ( 0, 0, 1 ) };
        for ( auto& a : attractors )
        {
            if ( K::DrawPointAttractors ) gl::drawSolidCircle( a.Position, 4.0f );
            for ( auto& b : _blackholes )
            {
                b.Interact(a, 1.0f);
            }
        }
    }
    
    if ( K::DrawSlopeMap )
    {
        if ( K::AnnabelBeingAPest )
        {
            vec2 size = _slopeGenerator->Size();
            float aspectRatio = size.y / size.x;
            
            Rectf r { 0, 0, 360.0f, 360.0f * aspectRatio };
            r += vec2 ( 540, 0 );
            
            gl::ScopedColor c { ColorAf::white() };
            gl::ScopedBlendAlpha blend;
            gl::draw ( _slopeGenerator->GetSlope(), r );
            
            r += vec2 ( r.getWidth(), 0 );
        }else
        {
            gl::ScopedColor c { ColorAf ( 1, 1, 1, 0.3f ) };
            gl::draw ( _slopeGenerator->GetSlope(), getWindowBounds() );
        }
    }
    
    if ( K::DrawHeatMask )
    {
        static gl::TextureRef kTexture = gl::Texture::create ( *_heatMask );
        gl::ScopedBlendAlpha blend;
        gl::ScopedColor c { ColorAf ( 1, 1, 1, 0.3f ) };
        gl::draw ( kTexture, getWindowBounds() );
    }
    
    if ( K::DebugDrawMasses )
    {
        for ( auto& b : _blackholes )
        {
            b.Draw();
        }
        
        for ( auto& b : _blackholes )
        {
            b.DebugDraw();
        }
        
        for ( auto& a : _attractors )
        {
            gl::ScopedColor color{ ColorAf(1, 1, 1, 0.4) };
            gl::drawSolidCircle(a.Position, 10.0f);
        }
    }

    if ( _renderUI )
    {
        bool doSave = false;
        bool doLoad = false;
        {
            ui::ScopedWindow window { "Settings" };
            ui::Text ( "%.2f FPS | %d Black Holes", getAverageFps(), (int)_blackholes.size() );
            
            if ( ui::Button ( "Save Session" ) )
            {
                doSave = true;               
            }
            ui::SameLine();
            if ( ui::Button("Load Session" ) )
            {
                doLoad = true;
            }
            
            if ( ui::CollapsingHeader ( "Audio OSC" ) )
            {
                if ( ui::InputInt ( "Screen ID", &K::ScreenID ) )
                {
                    K::OSCPrefix = "/bp/cosmic/" + std::to_string(K::ScreenID) + "/";
                }
                
                ui::InputText ( "OSC Endpoint", &K::OSCEndpoint );
                ui::InputInt ( "OSC Port", &K::OSCPort );
                ui::DragInt ( "OSC Send Frame Delay", &K::OSCSendDelay, 0.0f, 1, 600 );
                ui::Checkbox ( "OSC Send Messages Packed", &K::OSCSendAllPacked );
                
                if ( ui::Button( "Reconnect" ) )
                {
                    _oscSender = std::make_unique<OscSender>( K::OSCPort, K::OSCEndpoint );
                }
                
                ui::DragFloat ( "Pressedness Threshold", &K::UserPressednessThreshold, 0.01f, 0.0f, 1.0f );
                
                ui::Text ( "%s : %.2f", "Overall Pressedness", kSlopeFieldOverallPressedness );
                ui::Text ( "%s : %.2f / %.2f / %.2f", "Normalized Blackhole Velocities", kBlackholeVelocities[0], kBlackholeVelocities[1], kBlackholeVelocities[2] );
                ui::Text ( "%s : %.2f / %.2f / %.2f", "Blackhole Positions", kBlackholeXPositions[0], kBlackholeXPositions[1], kBlackholeXPositions[2] );
            }
            
            K::Inspect();
            
            ui::DragFloat ( "Min Depth", &_slopeGenerator->MinDepth, 0.0001f, 0.0f, 1.0f );
            ui::DragFloat ( "Max Depth", &_slopeGenerator->MaxDepth, 0.0001f, 0.0f, 1.0f );
            ui::DragFloat ( "Peak Threshold", &_slopeGenerator->PeakThreshold, 0.0001f, 0.0f, 1.0f );
            
            _particles->Inspect();
            
            ui::DragInt( "Emission Rate During Endgame", &K::EmissionRateDuringEndgame, 0, 0, 16 );
        }

        if (doSave)
        {
            auto p = getSaveFilePath();
            if (!p.empty())
            {
                JsonTree tree;
                Serialize(tree);
                tree.write(p);
            }
        }

        if (doLoad)
        {
            try
            {
                auto p = getOpenFilePath();
                if (!p.empty())
                {
                    JsonTree tree{ loadFile(p) };
                    Marshal(tree);

                    _blackholes.resize ( K::MaxBlackHoles );

                    {
                        auto fmt = gl::GlslProg::Format().define("MAX_BLACK_HOLES", std::to_string(_blackholes.size()));
                        fmt.vertex(app::loadAsset("Shaders/BlackHole.vs.glsl"));
                        fmt.fragment(app::loadAsset("Shaders/BlackHole.fs.glsl"));
                        fmt.fragDataLocation(0, "FinalColor");

                        _blackHoleShader = gl::GlslProg::create(fmt);
                        _blackHoleShader->uniform("uAspectRatio", 1.0f / app::getWindowAspectRatio());
                    }

                }
            }
            catch (const std::exception& e)
            {
                std::cout << "Error marshalling session: " << e.what() << std::endl;
            }
        }
    }
    
    HandleOSCValues ( slopeSurface );
    
    _rippleShader->uniform( "uAmplitude", K::RippleAmplitude() );
    _rippleShader->uniform( "uSpeed", K::RippleSpeed() );
}

void GravityWavesApp::HandleOSCValues ( const Surface &slopeSurface )
{
    kSlopeFieldOverallPressedness = 0.0f;
    
    {
        float maxValue = static_cast<float> ( ( slopeSurface.getWidth() / 5 ) * ( slopeSurface.getHeight() / 5 ) );
        float runningValue = 0.0f;
        
        for ( int x = 0; x < slopeSurface.getWidth(); x += 5 )
        {
            for ( int y = 0; y < slopeSurface.getHeight(); y += 5 )
            {
                vec2 na = vec2 ( x, y ) / vec2( slopeSurface.getSize() ) ;
                ColorAf c = slopeSurface.getPixel( na * vec2 ( slopeSurface.getSize() ) );
                
                vec2 v { c.r, c.g };
                v = v * 2.0f - 1.0f;
                
                if ( glm::length ( v ) > 0.3f ) runningValue++;
            }
        }
        
        kSlopeFieldOverallPressedness = runningValue / maxValue;
    }
    {
        for ( int i = 0; i < kBlackholeVelocities.size(); i++ )
        {
            auto& b = _blackholes[i];
            float vel = glm::clamp ( glm::length( b.Velocity ) / 32.0f, 0.0f, 1.0f );
            
            float xPos = glm::clamp(b.Position.x / (float)getWindowWidth(), 0.0f, 1.0f);;

            kBlackholeVelocities[i] = vel;
            kBlackholeXPositions[i] = lmap( xPos, 0.0f, 1.0f, -1.0f, 1.0f );
        }
    }
    
    if ( _oscSender )
    {
        if ( app::getElapsedFrames() % K::OSCSendDelay == 0 )
        {
            std::vector<std::pair<std::string, float>> messages;
            messages.emplace_back ( K::OSCPrefix + "slopefield", kSlopeFieldOverallPressedness );
                
            if ( kSlopeFieldOverallPressedness > K::UserPressednessThreshold )
            {
                for ( int i = 0; i < kBlackholeXPositions.size(); i++ )
                {
                    std::stringstream xName;
                    std::stringstream vName;
                    vName << K::OSCPrefix + "blackhole" << (i + 1);
                    xName << K::OSCPrefix + "blackhole" << (i + 1) << "_x";
            
                    messages.emplace_back( vName.str(), kBlackholeVelocities[i] );
                    messages.emplace_back( xName.str(), kBlackholeXPositions[i]);
                }
            }

            if ( K::OSCSendAllPacked )
            {
                _oscSender->SendPacked ( K::OSCPrefix, messages );
            }else
            {
                _oscSender->Send ( messages );
            }
        }
    }
}

void Init(App::Settings * settings)
{
#ifdef CINDER_MSW
#ifndef NDEBUG
    settings->setWindowSize(1920, 1200);
#else
    settings->setFullScreen();
#endif
#else
    settings->setWindowSize(1920, 1080);
#endif
}

CINDER_APP ( GravityWavesApp, RendererGl ( RendererGl::Options().msaa(0) ), Init );
