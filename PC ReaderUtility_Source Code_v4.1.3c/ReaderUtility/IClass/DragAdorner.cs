using System;
using System.Globalization;
using System.Reflection;
using System.Resources;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace RFID.Utility.IClass
{
    public class DragAdorner : Adorner
    {
        private ContentPresenter _contentPresenter;
        private AdornerLayer _adornerLayer;
        private double _leftOffset;
        private double _topOffset;
        private ResourceManager stringManager;

        public DragAdorner(object data, DataTemplate dataTemplate, UIElement adornedElement, AdornerLayer adornerLayer)
            : base(adornedElement)
        {
            stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());
            _adornerLayer = adornerLayer;
            _contentPresenter = new ContentPresenter() { Content = data, ContentTemplate = dataTemplate, Opacity = 0.75 };

            if (_adornerLayer == null)
                throw new ArgumentNullException(stringManager.GetString("AdornerLayer parameter is null", CultureInfo.CurrentCulture));
            _adornerLayer.Add(this);
        }

        protected override Size MeasureOverride(Size constraint)
        {
            _contentPresenter.Measure(constraint);
            return _contentPresenter.DesiredSize;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            _contentPresenter.Arrange(new Rect(finalSize));
            return finalSize;
        }

        protected override Visual GetVisualChild(int index)
        {
            return _contentPresenter;
        }

        protected override int VisualChildrenCount
        {
            get { return 1; }
        }

        public void UpdatePosition(double left, double top)
        {
            _leftOffset = left;
            _topOffset = top;
            if (_adornerLayer != null)
            {
                _adornerLayer.Update(this.AdornedElement);
            }
        }

        public override GeneralTransform GetDesiredTransform(GeneralTransform transform)
        {
            GeneralTransformGroup result = new GeneralTransformGroup();
            result.Children.Add(base.GetDesiredTransform(transform));
            result.Children.Add(new TranslateTransform(_leftOffset, _topOffset));
            return result;
        }

        public void Destroy()
        {
            _adornerLayer.Remove(this);
        }
    }
}
