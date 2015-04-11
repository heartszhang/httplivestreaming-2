

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Sat Apr 11 08:41:56 2015
 */
/* Compiler settings for C:\Users\Hearts\AppData\Local\Temp\DemoMix.idl-ba9e0935:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __DemoMix_h_h__
#define __DemoMix_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(__cplusplus)
#if defined(__MIDL_USE_C_ENUM)
#define MIDL_ENUM enum
#else
#define MIDL_ENUM enum class
#endif
#endif


/* Forward Declarations */ 

#ifndef ____x_ABI_CDemoMix_CIDummy_FWD_DEFINED__
#define ____x_ABI_CDemoMix_CIDummy_FWD_DEFINED__
typedef interface __x_ABI_CDemoMix_CIDummy __x_ABI_CDemoMix_CIDummy;

#ifdef __cplusplus
namespace ABI {
    namespace DemoMix {
        interface IDummy;
    } /* end namespace */
} /* end namespace */

#endif /* __cplusplus */

#endif 	/* ____x_ABI_CDemoMix_CIDummy_FWD_DEFINED__ */


/* header files for imported files */
#include "inspectable.h"
#include "Windows.Foundation.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_DemoMix_0000_0000 */
/* [local] */ 


#ifdef __cplusplus
namespace ABI {
namespace DemoMix {
class Dummy;
} /*DemoMix*/
}
#endif
#if !defined(____x_ABI_CDemoMix_CIDummy_INTERFACE_DEFINED__)
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_DemoMix_IDummy[] = L"DemoMix.IDummy";
#endif /* !defined(____x_ABI_CDemoMix_CIDummy_INTERFACE_DEFINED__) */


/* interface __MIDL_itf_DemoMix_0000_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_DemoMix_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_DemoMix_0000_0000_v0_0_s_ifspec;

#ifndef ____x_ABI_CDemoMix_CIDummy_INTERFACE_DEFINED__
#define ____x_ABI_CDemoMix_CIDummy_INTERFACE_DEFINED__

/* interface __x_ABI_CDemoMix_CIDummy */
/* [uuid][object] */ 



/* interface ABI::DemoMix::IDummy */
/* [uuid][object] */ 


EXTERN_C const IID IID___x_ABI_CDemoMix_CIDummy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    } /* end extern "C" */
    namespace ABI {
        namespace DemoMix {
            
            MIDL_INTERFACE("7DCCD41A-94B6-49BE-ACD6-3A306D2C427D")
            IDummy : public IInspectable
            {
            public:
                virtual HRESULT STDMETHODCALLTYPE Hello( void) = 0;
                
            };

            extern const __declspec(selectany) IID & IID_IDummy = __uuidof(IDummy);

            
        }  /* end namespace */
    }  /* end namespace */
    extern "C" { 
    
#else 	/* C style interface */

    typedef struct __x_ABI_CDemoMix_CIDummyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __x_ABI_CDemoMix_CIDummy * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __x_ABI_CDemoMix_CIDummy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __x_ABI_CDemoMix_CIDummy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetIids )( 
            __x_ABI_CDemoMix_CIDummy * This,
            /* [out] */ ULONG *iidCount,
            /* [size_is][size_is][out] */ IID **iids);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuntimeClassName )( 
            __x_ABI_CDemoMix_CIDummy * This,
            /* [out] */ HSTRING *className);
        
        HRESULT ( STDMETHODCALLTYPE *GetTrustLevel )( 
            __x_ABI_CDemoMix_CIDummy * This,
            /* [out] */ TrustLevel *trustLevel);
        
        HRESULT ( STDMETHODCALLTYPE *Hello )( 
            __x_ABI_CDemoMix_CIDummy * This);
        
        END_INTERFACE
    } __x_ABI_CDemoMix_CIDummyVtbl;

    interface __x_ABI_CDemoMix_CIDummy
    {
        CONST_VTBL struct __x_ABI_CDemoMix_CIDummyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define __x_ABI_CDemoMix_CIDummy_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define __x_ABI_CDemoMix_CIDummy_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define __x_ABI_CDemoMix_CIDummy_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define __x_ABI_CDemoMix_CIDummy_GetIids(This,iidCount,iids)	\
    ( (This)->lpVtbl -> GetIids(This,iidCount,iids) ) 

#define __x_ABI_CDemoMix_CIDummy_GetRuntimeClassName(This,className)	\
    ( (This)->lpVtbl -> GetRuntimeClassName(This,className) ) 

#define __x_ABI_CDemoMix_CIDummy_GetTrustLevel(This,trustLevel)	\
    ( (This)->lpVtbl -> GetTrustLevel(This,trustLevel) ) 


#define __x_ABI_CDemoMix_CIDummy_Hello(This)	\
    ( (This)->lpVtbl -> Hello(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* ____x_ABI_CDemoMix_CIDummy_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_DemoMix_0000_0001 */
/* [local] */ 

#ifndef RUNTIMECLASS_DemoMix_Dummy_DEFINED
#define RUNTIMECLASS_DemoMix_Dummy_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_DemoMix_Dummy[] = L"DemoMix.Dummy";
#endif


/* interface __MIDL_itf_DemoMix_0000_0001 */
/* [local] */ 



extern RPC_IF_HANDLE __MIDL_itf_DemoMix_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_DemoMix_0000_0001_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


