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
  ComPtr<IUnknown> un;
  auto hr = Windows::Foundation::ActivateInstance( HStringReference( L"Demo.Dummy" ).Get(), &un );
  return 0;
}
