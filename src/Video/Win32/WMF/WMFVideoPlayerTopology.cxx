//
//  WMFVideoPlayerTopology.cxx
//  SFMoma
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#include <windows.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <shobjidl.h> 
#include <shlwapi.h>
#include <assert.h>
#include <tchar.h>
#include <strsafe.h>
#include <propvarutil.h>
#include <PropIdl.h>

#include <Video/Win32/WMF/WMFVideoPlayerTopology.h>
#include <Video/Win32/WMF/WMFVideoPlayerPrivateImpl.h>

namespace Video
{   
    HRESULT CreateMediaSinkActivate ( IMFStreamDescriptor *pSourceSD, HWND hVideoWindow, IMFActivate **ppActivate, IMFVideoPresenter *pVideoPresenter, IMFMediaSink **ppMediaSink, const WCHAR *audioDeviceId = 0 )
    {
        IMFMediaTypeHandler *pHandler = nullptr;
        IMFActivate *pActivate = nullptr;
	    IMFMediaSink *pSink = nullptr;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease ( &pHandler );
            SafeRelease ( &pActivate );
	        SafeRelease ( &pSink );
            return r;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);

	    HRESULT result = S_OK;
        result = pSourceSD->GetMediaTypeHandler(&pHandler);
	    CHECK( result );

        GUID guidMajorType;
        result = pHandler->GetMajorType(&guidMajorType);
	    CHECK( result );
 
        if ( guidMajorType == MFMediaType_Audio)
        {
		    HRESULT hr = S_OK;

		    IMMDeviceEnumerator *pEnum = nullptr;
		    IMMDeviceCollection *pDevices = nullptr;
		    IMMDevice *pDevice = nullptr;
		    IMFMediaSink *pSink = nullptr;
		    IPropertyStore *pProps = nullptr;

		    LPWSTR wstrID = nullptr;		    
		    hr = CoCreateInstance ( __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum );

		    if (SUCCEEDED(hr))
		    {
			    hr = pEnum->EnumAudioEndpoints ( eRender, DEVICE_STATE_ACTIVE, &pDevices );
		    }

		    if (SUCCEEDED(hr))
		    {
			    UINT pcDevices = 0;
			    pDevices->GetCount( &pcDevices );			

			    for(UINT i = 0 ; i < pcDevices; i++)
                {
				    hr = pDevices->Item(i, &pDevice);

				    if (SUCCEEDED(hr))
				    {
					    hr = pDevice->GetId(&wstrID);												
				    }

				    if(SUCCEEDED(hr))
				    {
					    hr = pDevice->OpenPropertyStore ( STGM_READ, &pProps );
				    }				

				    PROPVARIANT varName = {0};
				    if(SUCCEEDED(hr))
				    {				
					    PropVariantInit(&varName);
                        hr = pProps->GetValue ( PKEY_Device_FriendlyName, &varName );		

					    if(SUCCEEDED(hr))
					    {
						    WCHAR szName[128];
						    hr = PropVariantToString ( varName, szName, ARRAYSIZE(szName) );
						    if (SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER)
						    {
							    if ( wcscmp ( szName, audioDeviceId) == 0 ) 
                                {
								    hr = pDevices->Item(i, &pDevice);
								    if (SUCCEEDED(hr))
								    {
									    hr = pDevice->GetId(&wstrID);	
									    PropVariantClear(&varName);
									    break;
								    }
							    }	
						    }					
					    }
				    }
 			    }
		    }

		    if (SUCCEEDED(hr))
		    {
			    hr = MFCreateAudioRendererActivate(&pActivate);    
		    }		
		
		    if (SUCCEEDED(hr))
		    {
			    hr = pActivate->SetString( MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, wstrID );
		    }

            *ppActivate = pActivate;
		    (*ppActivate)->AddRef();

            // @TODO(andrew): Check against SAFE_RELEASE
		    SafeRelease(&pEnum);
		    SafeRelease(&pDevices);
		    SafeRelease(&pDevice); 		
		    CoTaskMemFree(wstrID);
        }

        else if ( guidMajorType == MFMediaType_Video )
        {
            result = MFCreateVideoRenderer( __uuidof(IMFMediaSink), (void**)&pSink);
		    CHECK( result );

		    IMFVideoRenderer*  pVideoRenderer = nullptr;
		    result = pSink->QueryInterface(__uuidof(IMFVideoRenderer),(void**) &pVideoRenderer);
		    CHECK( result );

		    result = pVideoRenderer->InitializeRenderer( nullptr, pVideoPresenter ) ;
		    CHECK( result );

		    *ppMediaSink = pSink;
		    (*ppMediaSink)->AddRef();
        }else
        {
            result = E_FAIL;
        }

	    CHECK( result );
        return cleanup(result);
    }

    HRESULT AddSourceNode ( IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, IMFStreamDescriptor *pSD, IMFTopologyNode **ppNode )
    {
        IMFTopologyNode *pNode = nullptr;
        HRESULT result = S_OK;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease( &pNode );
            return r;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);
           
        result = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode);
	    CHECK( result );

        result = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
	    CHECK( result );

        result = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
	    CHECK( result );

        result = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
	    CHECK( result );
    
        result = pTopology->AddNode(pNode);
	    CHECK( result );

        *ppNode = pNode;
        (*ppNode)->AddRef();

        return cleanup(result);
    }

    HRESULT AddOutputNode ( IMFTopology *pTopology,	IMFStreamSink *pStreamSink,	IMFTopologyNode **ppNode )
    {
	    IMFTopologyNode *pNode = nullptr;
	    HRESULT result = S_OK;

	    result = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode);

	    if (SUCCEEDED(result))
	    {
		    result = pNode->SetObject(pStreamSink);
	    }

	    if (SUCCEEDED(result))
	    {
		    result = pTopology->AddNode(pNode);
	    }

	    if (SUCCEEDED(result))
	    {
		    result = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, TRUE);
	    }

	    if (SUCCEEDED(result))
	    {
		    *ppNode = pNode;
		    (*ppNode)->AddRef();
	    }

	    if ( pNode )
	    {
		    pNode->Release();
	    }

	    return result;
    }

    HRESULT AddOutputNode ( IMFTopology *pTopology, IMFActivate *pActivate, DWORD dwId, IMFTopologyNode **ppNode )
    {
        IMFTopologyNode *pNode = nullptr;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease( &pNode );
            return r;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);

	    HRESULT result = S_OK;
        result = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode);
	    CHECK( result );

        result = pNode->SetObject(pActivate);
	    CHECK( result );

        result = pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId);
	    CHECK( result );

        result = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
	    CHECK( result );

        result = pTopology->AddNode(pNode);
	    CHECK( result );
        
        *ppNode = pNode;
        (*ppNode)->AddRef();

        return cleanup(result);
    }

    HRESULT AddBranchToPartialTopology ( IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD,  DWORD iStream, HWND hVideoWnd, IMFVideoPresenter *pVideoPresenter, const WCHAR * audioDeviceId = 0 )
    {
        IMFStreamDescriptor *pSD = nullptr;
        IMFActivate         *pSinkActivate = nullptr;
        IMFTopologyNode     *pSourceNode = nullptr;
        IMFTopologyNode     *pOutputNode = nullptr;
	    IMFMediaSink        *pMediaSink = nullptr;

        BOOL selected       = false;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease(&pSD);
            SafeRelease(&pSinkActivate);
            SafeRelease(&pSourceNode);
            SafeRelease(&pOutputNode);
	
            return r;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);

        HRESULT result = pPD->GetStreamDescriptorByIndex ( iStream, &selected, &pSD );
	    CHECK( result );

        if ( selected )
        {
            result = CreateMediaSinkActivate ( pSD, hVideoWnd, &pSinkActivate,pVideoPresenter,&pMediaSink, audioDeviceId);
		    CHECK( result );

            result = AddSourceNode ( pTopology, pSource, pPD, pSD, &pSourceNode );
		    CHECK( result );

            if ( pSinkActivate )
		    {
			     result = AddOutputNode ( pTopology, pSinkActivate, 0, &pOutputNode );
		    }else if ( pMediaSink )
		    {
			    IMFStreamSink  * pStreamSink = nullptr;
			    DWORD streamCount;

			    pMediaSink->GetStreamSinkCount( &streamCount ) ;
			    pMediaSink->GetStreamSinkByIndex( 0, &pStreamSink );

			    result = AddOutputNode ( pTopology, pStreamSink, &pOutputNode );
			    CHECK( result );
		    }

		    CHECK( result );
            result = pSourceNode->ConnectOutput ( 0, pOutputNode, 0 );
        }
        
        return cleanup(result);
    }

    HRESULT CreatePlaybackTopology ( IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, HWND hVideoWnd, IMFTopology **ppTopology, IMFVideoPresenter *pVideoPresenter, const WCHAR *audioDeviceId )
    {
        IMFTopology * topology = nullptr;
        DWORD numSourceStreams = 0;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease ( &topology );
            return r;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);

        HRESULT result = MFCreateTopology ( &topology );
        CHECK(result);

        result = pPD->GetStreamDescriptorCount ( &numSourceStreams );
        CHECK(result);

        for ( DWORD i = 0; i < numSourceStreams; i++ )
        {
            result = AddBranchToPartialTopology ( topology, pSource, pPD, i, hVideoWnd, pVideoPresenter, audioDeviceId );
            CHECK ( result );
        }

        *ppTopology = topology;
        (*ppTopology)->AddRef();
        
        #undef CHECK
        return cleanup(result);
    }

    HRESULT AddToPlaybackTopology ( IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, HWND hVideoWnd, IMFTopology *pTopology, IMFVideoPresenter *pVideoPresenter )
    {
        DWORD numSourceStreams = 0;
        
        HRESULT result = pPD->GetStreamDescriptorCount ( &numSourceStreams );
        if ( FAILED ( result ) ) return result;

        for ( DWORD i = 0; i < numSourceStreams; i++ )
        {
            result = AddBranchToPartialTopology ( pTopology, pSource, pPD, i, hVideoWnd, pVideoPresenter );
            if ( FAILED ( result ) ) return result;
        }

        return result;
    }
}