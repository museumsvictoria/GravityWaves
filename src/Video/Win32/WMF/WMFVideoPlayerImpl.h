//
//  WMFVideoPlayerImpl.h
//  SFMoma
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#ifndef SFMoma_WMFVideoPlayerImpl_h
#define SFMoma_WMFVideoPlayerImpl_h

#include <Windows.h>
#include <Video/VideoPlayer.h>


namespace Video
{
	namespace
	{
		inline void Error(const std::string& error)
		{
			std::cout << "[WMFVideoPlayerImpl::Error] => " << error << std::endl;
		}

		inline void Info(const std::string& error)
		{
			std::cout << "[WMFVideoPlayerImpl::Info] => " << error << std::endl;
		}

		inline void Warn(const std::string& error)
		{
			std::cout << "[WMFVideoPlayerImpl::Warn] => " << error << std::endl;
		}
	}

    using WMFVideoPlayerImplRef = std::shared_ptr<class WMFVideoPlayerImpl>;
    class WMFVideoPlayerImpl : public VideoPlayer
    {
    public:
        
        WMFVideoPlayerImpl              ( const std::string& audioDevice = "" );
        ~WMFVideoPlayerImpl             ( );

        void                            Init                ( ) override;
        void                            Update              ( ) override;
        void                            Draw                ( const ci::Rectf& bounds ) override;
        void                            DrawWithShader      ( const ci::Rectf& bounds, const ci::gl::GlslProgRef& shader ) override;
        
		void							BindTexture			( ) override;
		void							UnbindTexture		( ) override;

        void                            Play                ( const std::string& path ) override;
		void                            Stop				( bool complete = false ) override;

		void							Loops				( bool loops ) override;
		bool							Loops				( ) const override { return _loops; }
        
        void                            Pause               ( ) override;
        void                            Resume              ( ) override;
        
        bool                            IsPlaying           ( ) const override;
        bool                            IsPaused            ( ) const override;
        
        bool                            SupportsCapability  ( Capability capability ) const override;
        
        ci::gl::Texture2dRef            Texture             ( ) const override;
        
        u32                             CurrentFrame        ( ) const override;
        u32                             TotalFrames         ( ) const override;
        
        ci::vec2                        Size                ( ) const override;
        ci::Area                        Bounds              ( ) const override;
        
        float                           Time                ( ) const override;
        float                           Duration            ( ) const override;
        
        float                           PercentComplete     ( ) const override;
        
        void                            SeekToFrame         ( u32 frame ) const override;
        void                            SeekToTime          ( float time ) const override;
        
        void                            PlayReversed        ( ) override;
        void                            PlaybackRate        ( float rate ) override;
        float                           PlaybackRate        ( ) const override;
        
        void                            Volume              ( float volume ) override;
        float                           Volume              ( ) const override;
        
        void                            OnComplete          ( CompleteFn handler ) override;
        void                            OnError             ( ErrorFn handler ) override;
        
		HWND							GetPlayerWindow		( ) const;
        LRESULT							WndProc				( HWND window, UINT message, WPARAM wParam, LPARAM lParam );

    protected:

		// Win32 / WMF Methods

		bool							InitInstance    	( );
		void							OnPlayerEvent		( HWND window, WPARAM param );

        class WMFVideoPlayerPrivateImpl* _player;
        float                           _rate{1.0f};
		float							_volume{ 1.0f };
        bool                            _isPlaying{false};

		HWND							_playerWindow;
		bool							_shouldRepaintClient{ false };

		ci::ivec2						_size;
		bool							_waitForLoadedToPlay{ false };
		bool							_sharedTextureCreated{ false };

        u32                             _id{0};
		ci::gl::Texture2dRef			_texture;
        std::string                     _audioDevice;
    };
}


#endif /* SFMoma_WMFVideoPlayerImpl_h */
