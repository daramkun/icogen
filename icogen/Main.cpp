#include <Wincodec.h>
#pragma comment ( lib, "WindowsCodecs.lib" )
#include <atlbase.h>
#include <atlconv.h>
#include <cstdio>
#include <vector>
#include <memory>
#include <tuple>

#include "StdoutStream.h"

#pragma pack ( push, 1 )
struct ICOHEADER { WORD reserved; WORD type; WORD count; };
struct ENTRYHEADER
{
	BYTE width;
	BYTE height;
	BYTE colorCount;
	BYTE reserved;
	WORD planes;
	WORD bitCount;
	DWORD sizeInBytes;
	DWORD fileOffset;
};
#pragma pack ( pop )

UINT availableSizes [] = { 256, 192, 128, 96, 64, 48, 32, 16, };
int __getAvailableSizeIndex ( IWICBitmapSource * source )
{
	UINT width, height;
	source->GetSize ( &width, &height );
	for ( int i = 0; i < _countof ( availableSizes ); ++i )
		if ( availableSizes [ i ] == width )
			return i;
	return -1;
}

IWICBitmapSource * __getBitmap ( IWICImagingFactory * factory, char * filename )
{
	USES_CONVERSION;

	CComPtr<IWICBitmapDecoder> decoder;
	if ( FAILED ( factory->CreateDecoderFromFilename ( A2W ( filename ), nullptr, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &decoder ) ) )
		return nullptr;

	CComPtr<IWICBitmapFrameDecode> decodeFrame;
	if ( FAILED ( decoder->GetFrame ( 0, &decodeFrame ) ) )
		return nullptr;

	UINT width, height;
	decodeFrame->GetSize ( &width, &height );

	if ( width != height )
		return nullptr;

	bool sizeIsAvailable = false;
	for ( UINT size : availableSizes )
		if ( width == size )
		{
			sizeIsAvailable = true;
			break;
		}
	if ( !sizeIsAvailable )
		return nullptr;

	CComPtr<IWICFormatConverter> formatConverter;
	if ( FAILED ( factory->CreateFormatConverter ( &formatConverter ) ) )
		return nullptr;
	HRESULT hr;
	if ( FAILED ( hr = formatConverter->Initialize ( decodeFrame, GUID_WICPixelFormat8bppIndexed,
		WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut ) ) )
		return nullptr;

	return formatConverter.Detach ();
}
IWICBitmapSource * __getBitmap ( IWICImagingFactory * factory, IWICBitmapSource * source, int size )
{
	CComPtr<IWICBitmapScaler> bitmapScaler;
	if ( FAILED ( factory->CreateBitmapScaler ( &bitmapScaler ) ) )
		return nullptr;
	if ( FAILED ( bitmapScaler->Initialize ( source, size, size, WICBitmapInterpolationModeHighQualityCubic ) ) )
		return nullptr;
	if ( size < 48 )
		return bitmapScaler.Detach ();

	CComPtr<IWICFormatConverter> formatConverter;
	if ( FAILED ( factory->CreateFormatConverter ( &formatConverter ) ) )
		return nullptr;
	if ( FAILED ( formatConverter->Initialize ( bitmapScaler, GUID_WICPixelFormat8bppIndexed,
		WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut ) ) )
		return nullptr;

	return formatConverter.Detach ();
}
bool __saveToStream ( IWICImagingFactory * factory, IWICBitmapSource * source, IStream * stream )
{
	//CComPtr<IStream> stream = SHCreateMemStream ( nullptr, 0 );
	UINT width, height;
	source->GetSize ( &width, &height );
	WICPixelFormatGUID pixelFormat;
	source->GetPixelFormat ( &pixelFormat );

	GUID containerFormat;
	//if ( width > 32 )
		containerFormat = GUID_ContainerFormatPng;
	//else
	//	containerFormat = GUID_ContainerFormatBmp;

	CComPtr<IWICBitmapEncoder> encoder;
	if ( FAILED ( factory->CreateEncoder ( containerFormat, nullptr, &encoder ) ) )
		return false;
	if ( FAILED ( encoder->Initialize ( stream, WICBitmapEncoderNoCache ) ) )
		return false;

	CComPtr<IWICBitmapFrameEncode> frameEncode;
	CComPtr<IPropertyBag2> encoderOptions;
	if ( FAILED ( encoder->CreateNewFrame ( &frameEncode, &encoderOptions ) ) )
		return false;
	if ( containerFormat == GUID_ContainerFormatPng )
	{
		PROPBAG2 propBag2 = { 0 };
		VARIANT variant;
		
		propBag2.pstrName = ( LPOLESTR ) L"InterlaceOption";
		VariantInit ( &variant );
		variant.vt = VT_BOOL;
		variant.boolVal = VARIANT_FALSE;
		encoderOptions->Write ( 0, &propBag2, &variant );

		propBag2.pstrName = ( LPOLESTR ) L"FilterOption";
		VariantInit ( &variant );
		variant.vt = VT_UI1;
		variant.bVal = WICPngFilterAdaptive;
		encoderOptions->Write ( 1, &propBag2, &variant );
	}
	else if ( containerFormat == GUID_ContainerFormatBmp )
	{
		PROPBAG2 propBag2 = { 0 };
		propBag2.pstrName = ( LPOLESTR ) L"EnableV5Header32bppBGRA";
		VARIANT variant;
		VariantInit ( &variant );
		variant.vt = VT_BOOL;
		variant.boolVal = VARIANT_TRUE;
		encoderOptions->Write ( 0, &propBag2, &variant );
	}

	if ( FAILED ( frameEncode->Initialize ( encoderOptions ) ) )
		return false;

	if ( FAILED ( frameEncode->SetSize ( width, height ) ) )
		return false;
	if ( FAILED ( frameEncode->SetPixelFormat ( &pixelFormat ) ) )
		return false;

	if ( FAILED ( frameEncode->WriteSource ( source, nullptr ) ) )
		return false;

	frameEncode->Commit ();
	encoder->Commit ();

	LARGE_INTEGER zero = { 0, };
	stream->Seek ( zero, STREAM_SEEK_SET, nullptr );

	return true;
}
bool __convertResizedContainsVector ( IWICImagingFactory * factory, const std::vector<CComPtr<IWICBitmapSource>> & resizedSources,
	std::vector<std::tuple<ENTRYHEADER, CComPtr<IStream>>> & tupleVector )
{
	for ( auto i = resizedSources.begin (); i != resizedSources.end (); ++i )
	{
		UINT width, height;
		( *i )->GetSize ( &width, &height );

		CComPtr<IStream> stream = SHCreateMemStream ( nullptr, 0 );
		if ( !__saveToStream ( factory, ( *i ), stream ) )
			continue;

		STATSTG statstg;
		if ( FAILED ( stream->Stat ( &statstg, 0 ) ) )
			continue;

		ENTRYHEADER header = { 0, };
		header.width = ( BYTE ) width;
		header.height = ( BYTE ) height;
		header.planes = 1;
		header.colorCount = 8;
		header.bitCount = 32;
		header.sizeInBytes = ( DWORD ) statstg.cbSize.QuadPart;

		tupleVector.push_back ( std::tuple<ENTRYHEADER, CComPtr<IStream>> ( header, stream ) );
	}

	return true;
}
bool __saveBitmap ( IWICImagingFactory * factory, const std::vector<CComPtr<IWICBitmapSource>> & resizedSources, IStream * outputStream )
{
	ICOHEADER header = { 0, 1, ( WORD ) resizedSources.size () };
	outputStream->Write ( &header, sizeof ( ICOHEADER ), nullptr );

	std::vector<std::tuple<ENTRYHEADER, CComPtr<IStream>>> tupleVector;
	if ( !__convertResizedContainsVector ( factory, resizedSources, tupleVector ) )
		return false;

	size_t totalWritten = tupleVector.size () * sizeof ( ENTRYHEADER ) + sizeof ( ICOHEADER );
	for ( auto i = tupleVector.begin (); i != tupleVector.end (); ++i )
	{
		auto & entry = std::get<0> ( *i );
		entry.fileOffset = ( DWORD ) totalWritten;
		outputStream->Write ( &entry, sizeof ( ENTRYHEADER ), nullptr );
		STATSTG statstg;
		std::get<1> ( *i )->Stat ( &statstg, 0 );
		totalWritten += ( size_t ) statstg.cbSize.QuadPart;
	}

	for ( auto i = tupleVector.begin (); i != tupleVector.end (); ++i )
	{
		STATSTG statstg;
		std::get<1> ( *i )->Stat ( &statstg, 0 );
		int length = ( int ) statstg.cbSize.QuadPart;
		int totalRead = 0;
		BYTE buffer [ 4096 ];
		while ( totalRead < length )
		{
			ULONG read;
			std::get<1> ( *i )->Read ( buffer, 4096, &read );
			outputStream->Write ( buffer, read, nullptr );
			totalRead += read;
		}
	}

	return true;
}

int process ( char * filename, char * target )
{
	CComPtr<IWICImagingFactory> imagingFactory;
	if ( FAILED ( CoCreateInstance ( CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory, ( LPVOID* ) &imagingFactory ) ) )
		return -3;

	CComPtr<IWICBitmapSource> originalSource = __getBitmap ( imagingFactory, filename );
	if ( originalSource == nullptr )
		return -4;

	std::vector<CComPtr<IWICBitmapSource>> resizedSources;
	resizedSources.push_back ( originalSource );
	for ( int i = __getAvailableSizeIndex ( originalSource ) + 1; i < _countof ( availableSizes ); ++i )
	{
		auto resized = __getBitmap ( imagingFactory, originalSource, availableSizes [ i ] );
		if ( resized == nullptr )
			continue;
		resizedSources.push_back ( resized );
	}

	CComPtr<IStream> outputStream;
	if ( strcmp ( target, "--stream" ) == 0 )
	{
		outputStream = new StdoutStream ();
	}
	else
	{
		USES_CONVERSION;
		if ( FAILED ( SHCreateStreamOnFile ( A2W ( target ), STGM_WRITE | STGM_CREATE, &outputStream ) ) )
			return -5;
	}

	return ( __saveBitmap(imagingFactory, resizedSources, outputStream ) == false ) ? -6 : 0;
}

int main ( int argc, char * argv [] )
{
	if ( argc != 3 )
	{
		printf ( "ERROR: Invalid Command-line arguments.\n" );
		return -1;
	}

	if ( FAILED ( CoInitialize ( nullptr ) ) )
	{
		printf ( "ERROR: Failed COM Initialization.\n" );
		return -2;
	}

	char * imageFilename = argv [ 1 ];
	char * target = argv [ 2 ];

	int ret = process ( imageFilename, target );

	CoUninitialize ();

	return ret;
}