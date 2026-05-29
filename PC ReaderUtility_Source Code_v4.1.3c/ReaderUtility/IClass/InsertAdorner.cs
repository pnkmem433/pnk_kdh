using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Resources;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace RFID.Utility.IClass
{
    public class InsertAdorner : Adorner
    {
        //public bool IsTopHalf { get; set; }
        private AdornerLayer _adornerLayer;
        private Pen _pen;
        private readonly bool _drawHorizontal;
        private bool _isTopHalf;
        private ResourceManager stringManager;

        public void IsTopHalf(bool b) {
            _isTopHalf = b;
        }

        public bool IsTopHalf() {
            return _isTopHalf;
        }

        public InsertAdorner(bool isTopHalf, bool drawHorizontal, UIElement adornedElement, AdornerLayer adornerLayer)
            : base(adornedElement)
        {
            stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());
            _isTopHalf = isTopHalf;
            _adornerLayer = adornerLayer;
            _drawHorizontal = drawHorizontal;

            if (_adornerLayer == null)
                throw new ArgumentNullException(stringManager.GetString("AdornerLayer parameter is null.", CultureInfo.CurrentCulture));
            _adornerLayer.Add(this);
            _pen = new Pen(new SolidColorBrush(Colors.Red), 3.0);

            DoubleAnimation animation = new DoubleAnimation(0.5, 1, new Duration(TimeSpan.FromSeconds(0.5)))
            {
                AutoReverse = true,
                RepeatBehavior = RepeatBehavior.Forever
            };
            _pen.Brush.BeginAnimation(Brush.OpacityProperty, animation);
        }

        protected override void OnRender(System.Windows.Media.DrawingContext drawingContext)
        {
            Point startPoint;
            Point endPoint;

            if (_drawHorizontal)
                DetermineHorizontalLinePoints(out startPoint, out endPoint);
            else
                DetermineVerticalLinePoints(out startPoint, out endPoint);

            if (drawingContext == null)
                throw new ArgumentNullException(stringManager.GetString("DrawingContext parameter is null.", CultureInfo.CurrentCulture));
            drawingContext.DrawLine(_pen, startPoint, endPoint);
        }

        private void DetermineHorizontalLinePoints(out Point startPoint, out Point endPoint)
        {
            double width = this.AdornedElement.RenderSize.Width;
            double height = this.AdornedElement.RenderSize.Height;

            if (!this._isTopHalf)
            {
                startPoint = new Point(0, height);
                endPoint = new Point(width, height);
            }
            else
            {
                startPoint = new Point(0, 0);
                endPoint = new Point(width, 0);
            }
        }

        private void DetermineVerticalLinePoints(out Point startPoint, out Point endPoint)
        {
            double width = this.AdornedElement.RenderSize.Width;
            double height = this.AdornedElement.RenderSize.Height;

            if (!this._isTopHalf)
            {
                startPoint = new Point(width, 0);
                endPoint = new Point(width, height);
            }
            else
            {
                startPoint = new Point(0, 0);
                endPoint = new Point(0, height);
            }
        }

        public void Destroy()
        {
            _adornerLayer.Remove(this);
        }
    }
}
