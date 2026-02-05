using System.Diagnostics;
using System.Windows;

namespace WpfApp1
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static object Print(string value)
        {
            Console.WriteLine("print value: " + value);

            //try
            //{
            var asm = System.Reflection.Assembly.LoadFile(value);
            //    var type = asm.GetType("ClassLibrary.MainWindow")!;
            //    var method = type.GetMethod("Print", System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.Static)!;
            //    method.Invoke(null, new object[] { "Hello from WPF!" });
            //}
            //catch (Exception ex)
            //{
            //    MessageBox.Show(ex.ToString());
            //}
            return null;
        }

        public MainWindow()
        {
            var name = typeof(System.Reflection.Assembly).Assembly;
            Console.WriteLine("Assembly Name: " + name);

            Loaded += MainWindow_Loaded;
            InitializeComponent();
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            var id = Process.GetCurrentProcess().Id;
            this.ProcessIdTextBox.Text = id.ToString();

            //var LoadFile = typeof(System.Reflection.Assembly).Assembly.GetType("System.Reflection.Assembly")
            //    .GetMethod("LoadFile");

            //var asm = (Assembly)LoadFile!.Invoke(null,
            //    [@"C:\Users\admin\Desktop\cppsamples\Simple-Manual-Map-Injector\ClassLibrary\bin\Debug\net8.0\ClassLibrary.dll"]);
            ////var asm = Assembly.LoadFile(@"C:\Users\admin\Desktop\cppsamples\Simple-Manual-Map-Injector\ClassLibrary\bin\Debug\net8.0\ClassLibrary.dll");

            //var type = asm.GetType("ClassLibrary.MainWindow")!;
            //var method = type.GetMethod("Print", BindingFlags.Public | BindingFlags.Static)!;
            //method.Invoke(null, new object[] { "Hello from WPF!" });
        }
    }
}