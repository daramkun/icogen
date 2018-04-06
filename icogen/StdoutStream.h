#ifndef __StdoutStream_h__
#define __StdoutStream_h__

#include <Windows.h>

class StdoutStream : public IStream
{
private:
	UINT _ref;

public:
	StdoutStream () : _ref ( 1 ) { }

public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface (
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == __uuidof( IStream ) )
		{
			AddRef ();
			*ppvObject = dynamic_cast< IStream* >( this );
			return S_OK;
		}
		return E_FAIL;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef ( void )
	{
		return InterlockedIncrement ( &_ref );
	}

	virtual ULONG STDMETHODCALLTYPE Release ( void )
	{
		UINT ret = InterlockedDecrement ( &_ref );
		if ( ret <= 0 )
			delete this;
		return ret;
	}

public:
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read (
		/* [annotation] */
		_Out_writes_bytes_to_ ( cb, *pcbRead )  void *pv,
		/* [annotation][in] */
		_In_  ULONG cb,
		/* [annotation] */
		_Out_opt_  ULONG *pcbRead )
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write (
		/* [annotation] */
		_In_reads_bytes_ ( cb )  const void *pv,
		/* [annotation][in] */
		_In_  ULONG cb,
		/* [annotation] */
		_Out_opt_  ULONG *pcbWritten )
	{
		size_t written = fwrite ( pv, cb, 1, stdout );
		if ( pcbWritten )
			*pcbWritten = ( ULONG ) written;
		return S_OK;
	}

public:
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek (
		/* [in] */ LARGE_INTEGER dlibMove,
		/* [in] */ DWORD dwOrigin,
		/* [annotation] */
		_Out_opt_  ULARGE_INTEGER *plibNewPosition )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SetSize (
		/* [in] */ ULARGE_INTEGER libNewSize )
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo (
		/* [annotation][unique][in] */
		_In_  IStream *pstm,
		/* [in] */ ULARGE_INTEGER cb,
		/* [annotation] */
		_Out_opt_  ULARGE_INTEGER *pcbRead,
		/* [annotation] */
		_Out_opt_  ULARGE_INTEGER *pcbWritten )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Commit (
		/* [in] */ DWORD grfCommitFlags )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Revert ( void )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE LockRegion (
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion (
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType )
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Stat (
		/* [out] */ __RPC__out STATSTG *pStatstg,
		/* [in] */ DWORD grfStatFlag )
	{
		ZeroMemory ( pStatstg, sizeof ( STATSTG ) );
		pStatstg->type = STGTY_STREAM;
		pStatstg->cbSize.QuadPart = 0;
		pStatstg->grfMode = STGM_WRITE;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Clone (
		/* [out] */ __RPC__deref_out_opt IStream **ppstm )
	{
		return E_NOTIMPL;
	}
};

#endif