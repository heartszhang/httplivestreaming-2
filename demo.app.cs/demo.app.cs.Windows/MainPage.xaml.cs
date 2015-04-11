using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
//using demo_schemehandler_win;
using demo_schemehandler_cx;
using Demo;
//using ABI.demo_schemehandler_cx;
// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace demo.app.cs
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
            var d = new Demo.Dummy();
            var x = d.Hello(1,2);
            var t = new DemoMix.Dummy();
            t.Hello();
        }
    }
}
