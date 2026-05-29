using System;
using System.Collections.Specialized;
using System.Globalization;
using System.Reflection;
using System.Resources;
using System.Windows;
using System.Windows.Controls;

namespace RFID.Utility.VM
{
    public class B01ListboxScroll : ListBox
    {
        private static ResourceManager stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());
        public bool AutoScroll
        {
            get { return (bool)GetValue(AutoScrollProperty); }
            set { SetValue(AutoScrollProperty, value); }
        }

        // Using a DependencyProperty as the backing store for AutoScoll.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty AutoScrollProperty =
            DependencyProperty.Register("AutoScroll", 
                                        typeof(bool), 
                                        typeof(B01ListboxScroll), 
                                        new UIPropertyMetadata(default(bool), 
                                        OnAutoScrollChanged));


        public static void OnAutoScrollChanged(DependencyObject s, DependencyPropertyChangedEventArgs e)
        {
            B01ListboxScroll thisLb = (B01ListboxScroll)s;
            if (thisLb == null)
                throw new ArgumentNullException(stringManager.GetString("B01ListboxScroll parameter is null.", CultureInfo.CurrentCulture));
            // Add the event handler in case that the property is set to true
            if ((bool)e.NewValue == true && (bool)e.OldValue == false)
            {
                if (!(thisLb.Items is INotifyCollectionChanged ic))
                {
                    return;
                }
                ic.CollectionChanged += new NotifyCollectionChangedEventHandler(thisLb.ic_CollectionChanged);
            }
            // Remove the event handel in case the property is set to false
            if ((bool)e.NewValue == false && (bool)e.OldValue == true)
            {
                if (!(thisLb.Items is INotifyCollectionChanged ic))
                {
                    return;
                }
                ic.CollectionChanged -= new NotifyCollectionChangedEventHandler(thisLb.ic_CollectionChanged);
            }
        }

        void ic_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (sender is ItemCollection ic)
            {
                //Scroll into the last item
                if (ic.Count > 1)
                {
                    this.ScrollIntoView(ic[ic.Count - 1]);
                }
            }
        }
    }

    /*public class B01ListboxScroll : DependencyObject
    {
        public static readonly DependencyProperty AutoScrollToCurrentItemProperty 
            = DependencyProperty.RegisterAttached("AutoScrollToCurrentItem", 
                                                    typeof(Boolean), 
                                                    typeof(B01ListboxScroll), 
                                                    new UIPropertyMetadata(default(Boolean), 
                                                    OnAutoScrollToCurrentItemChanged));
        public static readonly DependencyProperty AutoScrollToEndProperty 
            = DependencyProperty.RegisterAttached("AutoScrollToEnd", 
                                                    typeof(Boolean), typeof(B01ListboxScroll), 
                                                    new UIPropertyMetadata(default(Boolean), 
                                                    OnAutoScrollToEndChanged));

        /// <summary>
        /// Sets and Returns the value of the AutoScrollToCurrentItemProperty
        /// </summary>
        public Boolean AutoScrollToCurrentItem
        {
            get
            {
                return (bool)GetValue(AutoScrollToCurrentItemProperty);
            }
            set
            {
                SetValue(AutoScrollToCurrentItemProperty, value);
            }
        }

        /// <summary>
        /// Sets and Returns the value of the AutoScrollToEndProperty
        /// </summary>
        public Boolean AutoScrollToEnd
        {
            get
            {
                return (bool)GetValue(AutoScrollToEndProperty);
            }
            set
            {
                SetValue(AutoScrollToEndProperty, value);
            }
        }

        #region Events

        /// <summary>
        /// This method will be called when the AutoScrollToCurrentItem
        /// property was changed
        /// </summary>
        /// <param name="s">The sender (the ListBox)</param>
        /// <param name="e">Some additional information</param>
        public static void OnAutoScrollToCurrentItemChanged(DependencyObject s, DependencyPropertyChangedEventArgs e)
        {
            var listBox = s as ListBox;
            if (listBox != null)
            {
                var listBoxItems = listBox.Items;
                if (listBoxItems != null)
                {
                    var newValue = (bool)e.NewValue;

                    var autoScrollToCurrentItemWorker = new EventHandler((s1, e2) => OnAutoScrollToCurrentItem(listBox, listBox.Items.CurrentPosition));

                    if (newValue)
                        listBoxItems.CurrentChanged += autoScrollToCurrentItemWorker;
                    else
                        listBoxItems.CurrentChanged -= autoScrollToCurrentItemWorker;
                }
            }
        }

        /// <summary>
        /// This method will be called when the ListBox should
        /// be scrolled to the given index
        /// </summary>
        /// <param name="listBox">The ListBox which should be scrolled</param>
        /// <param name="index">The index of the item to which it should be scrolled</param>
        public static void OnAutoScrollToCurrentItem(ListBox listBox, Int32 index)
        {
            if (listBox != null && listBox.Items != null && listBox.Items.Count > index && index >= 0)
                listBox.ScrollIntoView(listBox.Items[index]);
        }

        /// This method will be called when the AutoScrollToEnd
        /// property was changed
        /// </summary>
        /// <param name="s">The sender (the ListBox)</param>
        /// <param name="e">Some additional information</param>
        public static void OnAutoScrollToEndChanged(DependencyObject s, DependencyPropertyChangedEventArgs e)
        {
            var listBox = s as ListBox;
            var listBoxItems = listBox.Items;
            var data = listBoxItems.SourceCollection as INotifyCollectionChanged;

            var scrollToEndHandler = new NotifyCollectionChangedEventHandler(
                (s1, e1) =>
                {
                    if (listBox.Items.Count > 0)
                    {
                        object lastItem = listBox.Items[listBox.Items.Count - 1];
                        listBoxItems.MoveCurrentTo(lastItem);
                        listBox.ScrollIntoView(lastItem);
                    }
                });

            if ((bool)e.NewValue)
                data.CollectionChanged += scrollToEndHandler;
            else
                data.CollectionChanged -= scrollToEndHandler;
        }
        #endregion
    }*/

}
