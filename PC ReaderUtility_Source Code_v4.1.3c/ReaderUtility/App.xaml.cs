using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Globalization;
using System.Linq;
using System.Text.RegularExpressions;
using System.Windows;

namespace RFID.Utility
{
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
        protected override void OnStartup(StartupEventArgs e)
        {
            // hook on error before app really starts
            AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);
            base.OnStartup(e);
        }

        void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            String[] strArray = Regex.Split(e.ExceptionObject.ToString(), "--- " , RegexOptions.IgnoreCase);
            // put your tracing or logging code here (I put a message box as an example)
            var result = MessageBox.Show(strArray[0], 
                                        String.Format(CultureInfo.CurrentCulture, "Error"), 
                                        MessageBoxButton.OK);
            if (result == MessageBoxResult.OK) {
                Shutdown();
            }
        }
    }
}
