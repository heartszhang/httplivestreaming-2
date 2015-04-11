// demo.dll.cpp : Defines the exported functions for the DLL application.
//

#include "pch.h"
#include "demo.h"

#include <wrl.h>
using namespace Microsoft::WRL;

namespace ABI {
  namespace Demo {
    class Dummy : public RuntimeClass < IDummy > {
      InspectableClass( RuntimeClass_Demo_Dummy, BaseTrust )
    public:
      Dummy() {}
      HRESULT __stdcall Hello( _In_ int a, _In_ int b, _Out_ int* value ) {
        OutputDebugStringW( L"what" );
        *value = a + b;
        return S_OK; }
    };
    ActivatableClass( Dummy );
  }
}