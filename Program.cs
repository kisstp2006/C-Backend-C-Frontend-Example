using System;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("Backend.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int AddNumbers(int a, int b);

    static void Main(string[] args)
    {
        int result = AddNumbers(5, 7);
        Console.WriteLine($"Result from C++: {result}");
    }
}
