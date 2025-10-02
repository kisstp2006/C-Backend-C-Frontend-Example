using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Frontend.Bindings
{
    internal class BackendPlusPlus
    {
        [DllImport("Backend.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int AddNumbers(int a, int b);
    }
}
