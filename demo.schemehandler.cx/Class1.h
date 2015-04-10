#pragma once

namespace demo_schemehandler_cx {
  public ref class Class1 sealed {
  public:
    Class1();
  };
}
namespace ABI {
  namespace demo_schemehandler_cx {
    class Class2 : public RuntimeClass < RuntimeClassFlags<WinRtClassicComMix>, IUnknown > {
      InspectableClass( L"demo_schemehandler_cx.Class2", BaseTrust );
    public:
      Class2() {}
      ~Class2() {}
    };
    ActivatableClass( Class2 )
  }
}
