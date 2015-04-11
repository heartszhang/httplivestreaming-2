// demo.dll.cpp : Defines the exported functions for the DLL application.
//

#include "pch.h"
#include "DemoMix.h"

#include <wrl.h>
using namespace Microsoft::WRL;

namespace ABI {
  namespace DemoMix {
    class Dummy : public RuntimeClass < IDummy > {
      InspectableClass( RuntimeClass_DemoMix_Dummy, BaseTrust )
    public:
      HRESULT __stdcall Hello() { 
        OutputDebugStringW( L"In demomix project;\n" ); return 0; 
      }
    };
    ActivatableClass( Dummy );
  }
}