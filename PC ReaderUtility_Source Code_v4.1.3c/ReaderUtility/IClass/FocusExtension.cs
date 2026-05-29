using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Resources;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace RFID.Utility.IClass
{
    public static class FocusExtension
    {
        private static ResourceManager stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());
        public static bool GetIsFocused(DependencyObject objj)
        {
            if (objj == null)
                throw new ArgumentNullException(stringManager.GetString("DependencyObject argument is null", CultureInfo.CurrentCulture));
            return (bool)objj.GetValue(IsFocusedProperty);
        }

        public static void SetIsFocused(DependencyObject objj, bool value)
        {
            if (objj == null)
                throw new ArgumentNullException(stringManager.GetString("DependencyObject argument is null", CultureInfo.CurrentCulture));

            objj.SetValue(IsFocusedProperty, value);
        }

        public static readonly DependencyProperty IsFocusedProperty =
            DependencyProperty.RegisterAttached(
                "IsFocused", 
                typeof(bool), 
                typeof(FocusExtension),
                new UIPropertyMetadata(false, OnIsFocusedPropertyChanged));

        private static void OnIsFocusedPropertyChanged(
            DependencyObject d,
            DependencyPropertyChangedEventArgs e)
        {
            var uie = (UIElement)d;
            if ((bool)e.NewValue)
            {
                uie.Focus(); // Don't care about false values.
            }
        }
    }
}
