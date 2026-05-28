using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;

namespace RFID.Utility.IClass
{
    public class GridViewColumnResize : GridViewColumn
    {
        protected override void OnPropertyChanged(DependencyPropertyChangedEventArgs e)
        {
            base.OnPropertyChanged(e);
        }

        public static readonly DependencyProperty DefaultWidthProperty = DependencyProperty.Register(
            "DefaultWidth", typeof(double), typeof(GridViewColumnResize), new PropertyMetadata(default(double)));

        public double DefaultWidth
        {
            get { return (double)GetValue(DefaultWidthProperty); }
            set { SetValue(DefaultWidthProperty, value); }
        }

        /// <summary>
        /// Width DependencyProperty
        /// </summary>
        public new static readonly DependencyProperty WidthProperty =
            FrameworkElement.WidthProperty.AddOwner(
                typeof(GridViewColumnResize),
                new PropertyMetadata(
                    Double.NaN /* default value */,
                    new PropertyChangedCallback(OnWidthChanged))
            );


        public static readonly DependencyProperty VisibilityProperty =
            UIElement.VisibilityProperty.AddOwner(typeof(GridViewColumnResize),
                new PropertyMetadata(Visibility.Visible, new PropertyChangedCallback(Target)));

        //private static readonly double _zeroValue = 0d;
        private const double _zeroValue = 0d;

        private static void Target(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs args)
        {
            var column = dependencyObject as GridViewColumnResize;
            var visibility = (Visibility)args.NewValue;
            if (visibility == Visibility.Collapsed)
            {
                column.SetValue(WidthProperty, _zeroValue);
            }
            else if (visibility == Visibility.Visible)
            {
                column.SetValue(WidthProperty, column.DefaultWidth);
            }
        }

        public Visibility Visibility
        {
            get { return (Visibility)GetValue(VisibilityProperty); }
            set { SetValue(VisibilityProperty, value); }
        }



        /// <summary>
        /// width of the column
        /// </summary>
        /// <remarks>
        /// The default value is Double.NaN which means size to max visible item width.
        /// </remarks>
        [TypeConverter(typeof(LengthConverter))]
        public new double Width
        {
            get { return (double)GetValue(WidthProperty); }
            set { SetValue(WidthProperty, value); }
        }

        private static void OnWidthChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            GridViewColumnResize c = (GridViewColumnResize)d;

            var visibility = c.Visibility;
            if (visibility == Visibility.Collapsed)
                c.Width = _zeroValue;

            double newWidth = (double)e.NewValue;
            c.OnPropertyChanged(new PropertyChangedEventArgs(WidthProperty.Name));
        }
    }
}
