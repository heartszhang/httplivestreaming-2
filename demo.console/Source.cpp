// Source.cpp : Defines the entry point for the console application.
//
#include <wrl.h>

#include <iostream>
using namespace std;

using namespace Platform;
//using namespace Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

int main( Platform::Array<Platform::String^>^ args ) {
  Platform::Details::Console::WriteLine( "Hello World" );
  auto c = ref new demo_schemehandler_win::Class1();
  c->Hello();
  c = nullptr;
  //  auto x = ref new demo_schemehandler_cx::Class2();
  ComPtr<IUnknown> un;
  auto hr = Windows::Foundation::ActivateInstance( HStringReference( L"demo_schemehandler_win.Class1" ).Get(), &un );
  return 0;
}
