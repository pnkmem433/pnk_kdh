using Microsoft.Expression.Interactivity;
using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;

namespace RFID.Utility.IClass
{
    public class ItemsControlDragDropBehavior : Behavior<ItemsControl>
    {
        private bool _isMouseDown;
        private object _data;
        private Point _dragStartPosition;
        private bool _isDragging;
        private bool _isRemove = false;
        private DragAdorner _dragAdorner;
        private InsertAdorner _insertAdorner;
        private int _dragScrollWaitCounter;
        private const int DRAG_WAIT_COUNTER_LIMIT = 10;

        public ItemsControlDragDropBehavior()
        {
            _isMouseDown = false;
            _isDragging = false;
            _dragScrollWaitCounter = DRAG_WAIT_COUNTER_LIMIT;
        }

        protected override void OnAttached()
        {
            this.AssociatedObject.AllowDrop = true;
            this.AssociatedObject.PreviewMouseLeftButtonDown += new MouseButtonEventHandler(ItemsControlPreviewMouseLeftButtonDown);
            this.AssociatedObject.PreviewMouseMove += new MouseEventHandler(ItemsControlPreviewMouseMove);
            this.AssociatedObject.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(ItemsControlPreviewMouseLeftButtonUp);
            //this.AssociatedObject.PreviewMouseRightButtonDown += new MouseButtonEventHandler(ItemsControlPreviewMouseRightButtonDown);
            this.AssociatedObject.PreviewDrop += new DragEventHandler(ItemsControlPreviewDrop);
            this.AssociatedObject.PreviewQueryContinueDrag += new QueryContinueDragEventHandler(ItemsControlPreviewQueryContinueDrag);
            this.AssociatedObject.PreviewDragEnter += new DragEventHandler(ItemsControlPreviewDragEnter);
            this.AssociatedObject.PreviewDragOver += new DragEventHandler(ItemsControlPreviewDragOver);
            this.AssociatedObject.DragLeave += new DragEventHandler(ItemsControlDragLeave);
            //this.AssociatedObject.PreviewDragLeave += new DragEventHandler(ItemsControlPreviewDragLeave);
            this.AssociatedObject.MouseLeave += new MouseEventHandler(ItemsControlMouseLeave);
        }

        

        #region Properties

        public Type ItemType { get; set; }

        public DataTemplate DataTemplate { get; set; }

        #endregion

        #region Button Events

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            //Debug.WriteLine("PreviewMouseLeftButtonDown");
            ItemsControl itemsControl = (ItemsControl)sender;
            Point p = e.GetPosition(itemsControl);
            _data = DragDropHelper.GetDataObjectFromItemsControl(itemsControl, p);
            if (_data != null && ((B02Item02Command)_data).CommandState != MainWindow.CommandStatus.REGULATION)
            {
                //Debug.WriteLine("PreviewMouseLeftButtonDown  _isMouseDown = true");
                _isMouseDown = true;
                _dragStartPosition = p;
            }
        }

        void ItemsControlPreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            /*ItemsControl itemsControl = (ItemsControl)sender;
            Point p = e.GetPosition(itemsControl);
            UIElement draggedItemContainer = DragDropHelper.GetItemContainerFromPoint(itemsControl, p);

            if (draggedItemContainer != null)
            {
                int dragItemIndex = itemsControl.ItemContainerGenerator.IndexFromContainer(draggedItemContainer);

                MessageBoxResult result = MessageBox.Show("Would you like to Delete the item? ", "", MessageBoxButton.OKCancel);
                switch (result)
                {
                    case MessageBoxResult.OK:
                        DragDropHelper.RemoveItem(itemsControl, dragItemIndex);
                        break;
                    case MessageBoxResult.Cancel:

                        break;
                }
            }*/
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewMouseMove(object sender, MouseEventArgs e)
        {
            //Debug.WriteLine("PreviewMouseMove");
            if (_isMouseDown)
            {
                //Debug.WriteLine("PreviewMouseMove _isMouseDown");
                ItemsControl itemsControl = (ItemsControl)sender;
                Point currentPosition = e.GetPosition(itemsControl);
                if ((_isDragging == false) && (Math.Abs(currentPosition.X - _dragStartPosition.X) > SystemParameters.MinimumHorizontalDragDistance) ||
                    (Math.Abs(currentPosition.Y - _dragStartPosition.Y) > SystemParameters.MinimumVerticalDragDistance))
                {
                    //Debug.WriteLine("PreviewMouseMove _isMouseDown DragStarted");
                    DragStarted(itemsControl);
                }
            }
        }

        void ItemsControlMouseLeave(object sender, MouseEventArgs e)
        {
            //Debug.WriteLine("MouseLeave");
            ResetState();
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            //Debug.WriteLine("PreviewMouseLeftButtonUp");
            ResetState();
        }

        #endregion



        #region Drag Events

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewDrop(object sender, DragEventArgs e)
        {
            //Debug.WriteLine("PreviewDrop");
            ItemsControl itemsControl = (ItemsControl)sender;
            DetachAdorners();
            e.Handled = true;

            if (e.Data.GetDataPresent(ItemType))
            {
                object itemToAdd = e.Data.GetData(ItemType);
                if ((e.KeyStates & DragDropKeyStates.ControlKey) != 0 && DragDropHelper.DoesItemExists(itemsControl, itemToAdd))
                {
                    int index = itemsControl.Items.IndexOf(itemToAdd);
                    var _temp = (B02Item02Command)itemToAdd;
                    B02Item02Command _temp2 = new B02Item02Command(
                        _temp.Check,
                        _temp.Type,
                        _temp.Name,
                        _temp.CommandState,
                        _temp.Command,
                        _temp.TabIndex,
                        false);
                    DragDropHelper.RemoveItem(itemsControl, itemToAdd);
                    DragDropHelper.AddItem(itemsControl, _temp2, index);
                    /*if (MessageBox.Show("Item already exists. Do you want to overwrite it?", "Copy File",
                        MessageBoxButton.YesNo) == MessageBoxResult.Yes)
                    {
                        int index = itemsControl.Items.IndexOf(itemToAdd);
                        var _temp = (B02Item02Command)itemToAdd;
                        B02Item02Command _temp2 = new B02Item02Command(
                            _temp.Check,
                            _temp.Type,
                            _temp.Name,
                            _temp.CommandState,
                            _temp.Command,
                            _temp.TabIndex,
                            false);
                        DragDropHelper.RemoveItem(itemsControl, itemToAdd);
                        DragDropHelper.AddItem(itemsControl, _temp2, index);
                    }
                    else
                    {
                        e.Effects = DragDropEffects.None;
                        return;
                    }*/
                }
                e.Effects = ((e.KeyStates & DragDropKeyStates.ControlKey) != 0) ?
                            DragDropEffects.Copy : DragDropEffects.Move;
                if (itemsControl.Items.Count != FindInsertionIndex(itemsControl, e))
                {
                    _isRemove = true;

                    var _temp = (B02Item02Command)itemToAdd;
                    B02Item02Command _temp2 = new B02Item02Command(
                            _temp.Check,
                            _temp.Type,
                            _temp.Name,
                            _temp.CommandState,
                            _temp.Command,
                            _temp.TabIndex,
                            false);
                    DragDropHelper.AddItem(itemsControl, _temp2, FindInsertionIndex(itemsControl, e));
                }
                else
                {
                    _isRemove = false;
                }
                    
            }
            else
            {
                e.Effects = DragDropEffects.None;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewQueryContinueDrag(object sender, QueryContinueDragEventArgs e)
        {
            //Debug.WriteLine("PreviewQueryContinueDrag");
            if (e.EscapePressed)
            {
                e.Action = DragAction.Cancel;
                ResetState();
                DetachAdorners();
                e.Handled = true;
            }
            //e.Handled = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewDragEnter(object sender, DragEventArgs e)
        {
            //Debug.WriteLine("PreviewDragEnter");
            ItemsControl itemsControl = (ItemsControl)sender;
            if (e.Data.GetDataPresent(ItemType))
            {
                object data = e.Data.GetData(ItemType);
                InitializeDragAdorner(itemsControl, data, e.GetPosition(itemsControl));
                InitializeInsertAdorner(itemsControl, e);
            }
            e.Handled = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlPreviewDragOver(object sender, DragEventArgs e)
        {
            //Debug.WriteLine("PreviewDragOver");
            ItemsControl itemsControl = (ItemsControl)sender;
            if (e.Data.GetDataPresent(ItemType))
            {
                UpdateDragAdorner(e.GetPosition(itemsControl));
                UpdateInsertAdorner(itemsControl, e);
                HandleDragScrolling(itemsControl, e);
            }
            e.Handled = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ItemsControlDragLeave(object sender, DragEventArgs e)
        {
            //Debug.WriteLine("DragLeave");
            DetachAdorners();
            e.Handled = true;
        }

        /*void ItemsControlPreviewDragLeave(object sender, DragEventArgs e)
        {
            Debug.WriteLine("PreviewDragLeave");
            DetachAdorners();
            e.Handled = true;
        }*/

        #endregion


        #region Private Methods

        private void DragStarted(ItemsControl itemsControl)
        {
            if (_data.GetType() != ItemType)
            {
                return;
            }

            UIElement draggedItemContainer = DragDropHelper.GetItemContainerFromPoint(itemsControl, _dragStartPosition);
            _isDragging = true;
            DataObject dObject = new DataObject(ItemType, _data);
            DragDropEffects e = DragDrop.DoDragDrop(itemsControl, dObject, DragDropEffects.Copy | DragDropEffects.Move);
            if ((e & DragDropEffects.Move) != 0)
            {
                if (draggedItemContainer != null)
                {
                    int dragItemIndex = itemsControl.ItemContainerGenerator.IndexFromContainer(draggedItemContainer);
                    if (_isRemove)
                    {
                        _isRemove = false;
                        DragDropHelper.RemoveItem(itemsControl, dragItemIndex);
                    }
                    
                }
                else
                {
                    DragDropHelper.RemoveItem(itemsControl, _data);
                }
            }
            ResetState();
        }

        private void HandleDragScrolling(ItemsControl itemsControl, DragEventArgs e)
        {
            bool? isMouseAtTop = DragDropHelper.IsMousePointerAtTop(itemsControl, e.GetPosition(itemsControl));
            if (isMouseAtTop.HasValue)
            {
                if (_dragScrollWaitCounter == DRAG_WAIT_COUNTER_LIMIT)
                {
                    _dragScrollWaitCounter = 0;
                    //Debug.WriteLine(DateTime.Now.Ticks);
                    ScrollViewer scrollViewer = DragDropHelper.FindScrollViewer(itemsControl);
                    if (scrollViewer != null && scrollViewer.CanContentScroll
                        && scrollViewer.ComputedVerticalScrollBarVisibility == Visibility.Visible)
                    {
                        if (isMouseAtTop.Value)
                        {
                            scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - 1.0);

                        }
                        else
                        {
                            scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset + 1.0);
                        }
                        e.Effects = DragDropEffects.Scroll;
                    }
                }
                else
                {
                    _dragScrollWaitCounter++;
                }
            }
            else
            {
                e.Effects = ((e.KeyStates & DragDropKeyStates.ControlKey) != 0) ?
                               DragDropEffects.Copy : DragDropEffects.Move;
            }
        }

        private int FindInsertionIndex(ItemsControl itemsControl, DragEventArgs e)
        {
            UIElement dropTargetContainer = DragDropHelper.GetItemContainerFromPoint(itemsControl, e.GetPosition(itemsControl));
            if (dropTargetContainer != null)
            {
                int index = itemsControl.ItemContainerGenerator.IndexFromContainer(dropTargetContainer);
                if (DragDropHelper.IsPointInTopHalf(itemsControl, e))
                    return index;
                else
                    return index + 1;
            }
            return itemsControl.Items.Count;

        }

        private void ResetState()
        {
            _isMouseDown = false;
            _isDragging = false;
            _data = null;
            _dragScrollWaitCounter = DRAG_WAIT_COUNTER_LIMIT;
        }

        private void InitializeDragAdorner(ItemsControl itemsControl, object dragData, Point startPosition)
        {
            if (this.DataTemplate != null)
            {
                if (_dragAdorner == null)
                {
                    var adornerLayer = AdornerLayer.GetAdornerLayer(itemsControl);
                    _dragAdorner = new DragAdorner(dragData, DataTemplate, itemsControl, adornerLayer);
                    _dragAdorner.UpdatePosition(startPosition.X, startPosition.Y);
                }
            }
        }

        private void UpdateDragAdorner(Point currentPosition)
        {
            if (_dragAdorner != null)
            {
                _dragAdorner.UpdatePosition(currentPosition.X, currentPosition.Y);
            }
        }

        private void InitializeInsertAdorner(ItemsControl itemsControl, DragEventArgs e)
        {
            if (_insertAdorner == null)
            {
                var adornerLayer = AdornerLayer.GetAdornerLayer(itemsControl);
                UIElement itemContainer = DragDropHelper.GetItemContainerFromPoint(itemsControl, e.GetPosition(itemsControl));
                if (itemContainer != null)
                {
                    bool isPointInTopHalf = DragDropHelper.IsPointInTopHalf(itemsControl, e);
                    bool isItemsControlOrientationHorizontal = DragDropHelper.IsItemControlOrientationHorizontal(itemsControl);
                    _insertAdorner = new InsertAdorner(isPointInTopHalf, isItemsControlOrientationHorizontal, itemContainer, adornerLayer);
                }
            }
        }

        private void UpdateInsertAdorner(ItemsControl itemsControl, DragEventArgs e)
        {
            if (_insertAdorner != null)
            {
                _insertAdorner.IsTopHalf(DragDropHelper.IsPointInTopHalf(itemsControl, e));
                _insertAdorner.InvalidateVisual();
            }
        }

        private void DetachAdorners()
        {
            if (_insertAdorner != null)
            {
                _insertAdorner.Destroy();
                _insertAdorner = null;
            }
            if (_dragAdorner != null)
            {
                _dragAdorner.Destroy();
                _dragAdorner = null;
            }
        }

        #endregion
    }
}
