using RFID.Service;
using RFID.Utility.IClass;
using RFID.Utility.VM;
using System;
using System.Collections;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Reflection;
using System.Resources;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using static RFID.Utility.MainWindow;

namespace RFID.Utility
{
    //partial
    public partial class EditCommandDialog : Window
    {
        enum ValidationStates { DEFAULF, OK, ERROR, WARNING, FOCUS };
        private Hashtable ValidationState = new Hashtable();

        public ObservableCollection<B02Item02Command> B02Item02Commands { get; }
        private Int32 Index = 0;
        private String TabCtrlHeader;
        //private String DefineSequenceStandardTemp, DefineSequenceCustomizeTemp;
        private Boolean IsFocus = false;
        private Int32 Mask_, Action_;
        //private Boolean B02Item02Buttom = false;

        private EditCommandVM VM = new EditCommandVM();

        private ResourceManager stringManager;


        public delegate void PassValuesEventHandler(object sender, PassValuesEventArgs e);

        public event PassValuesEventHandler EditCommandPassValuesEventHandler;

        private void EditCommandPassValuesReceive(Int32 _index, B02Item02Command cmd)
        {
            if (EditCommandPassValuesEventHandler != null)
            {
                PassValuesEventArgs e = new PassValuesEventArgs(_index, cmd);
                EditCommandPassValuesEventHandler(this, e);
            }
        }

        public EditCommandDialog()
        {
            InitializeComponent();
            stringManager =
            new ResourceManager("en-US", Assembly.GetExecutingAssembly());
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="commands"></param>
        /// <param name="idx"></param>
        public EditCommandDialog(ObservableCollection<B02Item02Command> commands, int idx)
        {
            InitializeComponent();
            stringManager =
            new ResourceManager("en-US", Assembly.GetExecutingAssembly());

            this.B02Item02Commands = commands;
            this.VM.EditCommandTabReadMemBank = VM.MemBank[1];
            this.VM.EditCommandTabReadMemBankIndex = 1;

            this.VM.EditCommandTabWriteMemBank = VM.MemBank[1];
            this.VM.EditCommandTabWriteMemBankIndex = 1;

            this.VM.EditCommandTabUQRMemBank = VM.MemBank[1];
            this.VM.EditCommandTabUQRMemBankIndex = 1;
            this.VM.EditCommandTabUQRSlotQComboBox = VM.SlotQ[3];

            this.VM.EditCommandTabSelectMemBank = VM.MemBank[1];
            this.VM.EditCommandTabSelectMemBankIndex = 1;


            this.DataContext = VM;

            this.Index = idx;
            this.IndexForwardNext.Text = idx.ToString("D", CultureInfo.CurrentCulture);


            if (Index == 0) this.IndexForwardNext1.IsEnabled = false;
            if (commands != null)
                if (Index == (commands.Count - 1))
                {
                    this.IndexForwardNext2.IsEnabled = false;
                    DeleteSequenceBtn.IsEnabled = false;
                }
            //OKSequenceBtn.IsEnabled = false;
            ApplySequenceBtn.IsEnabled = false;
            VM.EditCommandFirst = true;
            EditCommandDialogUpdate(Index);
        }

        private void Initialize()
        {
            this.VM.EditCommandTabReadMemBank = VM.MemBank[1];
            this.VM.EditCommandTabReadMemBankIndex = 1;
            this.VM.EditCommandTabReadAddress = String.Empty;
            this.VM.EditCommandTabReadLength = String.Empty;

            this.VM.EditCommandTabWriteMemBank = VM.MemBank[1];
            this.VM.EditCommandTabWriteMemBankIndex = 1;
            this.VM.EditCommandTabWriteAddress = String.Empty;
            this.VM.EditCommandTabWriteLength = String.Empty;
            this.VM.EditCommandTabWriteData = String.Empty;

            this.VM.EditCommandTabUQRMemBank = VM.MemBank[1];
            this.VM.EditCommandTabUQRMemBankIndex = 1;
            this.VM.EditCommandTabUQRAddress = String.Empty;
            this.VM.EditCommandTabUQRLength = String.Empty;
            this.VM.EditCommandTabUQRSlotQComboBox = VM.SlotQ[3];

            this.VM.EditCommandTabSelectMemBank = VM.MemBank[1];
            this.VM.EditCommandTabSelectMemBankIndex = 1;
            this.VM.EditCommandTabSelectAddress = String.Empty;
            this.VM.EditCommandTabSelectLength = String.Empty;
            this.VM.EditCommandTabSelectData = String.Empty;
        }

        private void OnEditCommandDialogCloseClick(object sender, RoutedEventArgs e)
        {
            for (int i = 0; i < B02Item02Commands.Count; i++)
                B02Item02Commands[i].ApplyButton = false;
            this.DialogResult = false;
        }

        private void OnEditCommandBorderMouseLeftDown(object sender, MouseButtonEventArgs e)
        {
            this.DragMove();
        }

        // <
        private void OnIndexForwardNextChecked1(object sender, RoutedEventArgs e)
        {
            this.Index -= 1;
            this.IndexForwardNext.Text = Index.ToString(CultureInfo.CurrentCulture);
            if (Index == 0)
                this.IndexForwardNext1.IsEnabled = false;
            if (Index == 0 && B02Item02Commands.Count > 1)
                this.IndexForwardNext2.IsEnabled = true;
            else if (Index == (B02Item02Commands.Count - 1))
                this.IndexForwardNext2.IsEnabled = false;
            else
            {
                this.IndexForwardNext1.IsEnabled = true;
                this.IndexForwardNext2.IsEnabled = true;
            }

            Initialize();
            VM.EditCommandUpdate = true;
            if (B02Item02Commands[Index].TypeTemp is false)
                VM.EditCommandTypeIsChecked = "Standard";
            else
                VM.EditCommandTypeIsChecked = "Customize";
            EditCommandDialogUpdate(Index);
            
        }

        // >
        private void OnIndexForwardNextChecked2(object sender, RoutedEventArgs e)
        {
            this.Index += 1;
            this.IndexForwardNext.Text = Index.ToString("D", CultureInfo.CurrentCulture);
            if (Index == 0)
                this.IndexForwardNext1.IsEnabled = false;
            else if (Index == (B02Item02Commands.Count - 1))
            {
                this.IndexForwardNext1.IsEnabled = true;
                this.IndexForwardNext2.IsEnabled = false;
            }
            else
            {
                this.IndexForwardNext1.IsEnabled = true;
                this.IndexForwardNext2.IsEnabled = true;
            }

            Initialize();
            VM.EditCommandUpdate = true;
            if (B02Item02Commands[Index].TypeTemp is false)
                VM.EditCommandTypeIsChecked = "Standard";
            else
                VM.EditCommandTypeIsChecked = "Customize";
            EditCommandDialogUpdate(Index);
        }

        private void EditCommandDialogUpdate(Int32 idx)
        {
            Boolean _change = false;

            if (B02Item02Commands[idx].CommandState is CommandStatus.REGULATION)
            {
                VM.EditCommandTabControlItemSelectedIndex = 0;
                VM.DefineName = String.Empty;
                VM.DefineSequence = String.Empty;
                
                DeleteSequenceBtn.IsEnabled = false;
                OKSequenceBtn.IsEnabled = false;
                ApplySequenceBtn.IsEnabled = false;
            }
            else
            {
                OKSequenceBtn.IsEnabled = true;
                DeleteSequenceBtn.IsEnabled = true;
            }

            if (B02Item02Commands[idx].ApplyButton is true)
                ApplySequenceBtn.IsEnabled = true;
            else
                ApplySequenceBtn.IsEnabled = false;

            VM.DefineName = (String.IsNullOrEmpty(B02Item02Commands[idx].Name)) ? String.Empty : B02Item02Commands[idx].Name;
            if (B02Item02Commands[idx].CommandTemp.Equals(B02Item02Commands[idx].Command, StringComparison.CurrentCulture))
            {
                _change = false;
                VM.DefineSequence = B02Item02Commands[idx].Command;
            } 
            else
            {
                _change = true;
                VM.DefineSequence = B02Item02Commands[idx].CommandTemp;
            }
                
            VM.EditCommandUpdate = true;
            if (B02Item02Commands[idx].TypeTemp is false)//B02Item02Commands[idx].Type
            {
                if (VM.DefineSequence != null)
                {
                    var _str = VM.DefineSequence.Split(',');
                    switch (B02Item02Commands[idx].TabIndexTemp)
                    {
                        case 1:
                            VM.EditCommandTabReadMemBank = VM.MemBank[int.Parse(_str[0].Substring(1), CultureInfo.CurrentCulture)];
                            VM.EditCommandTabReadMemBankIndex = int.Parse(_str[0].Substring(1), CultureInfo.CurrentCulture);

                            if (_change is false)
                            {
                                VM.EditCommandTabReadAddress = _str[1];
                                VM.EditCommandTabReadLength = _str[2];
                                ValidationState["EditCommandTabReadAddress"] = ValidationStates.OK;
                                ValidationState["EditCommandTabReadLength"] = ValidationStates.OK;
                            }
                            else
                            {
                                if (String.IsNullOrEmpty(_str[1]))//_str[1].Equals(String.Empty)
                                {
                                    VM.EditCommandTabReadAddress = String.Empty;
                                    ValidationState["EditCommandTabReadAddress"] = ValidationStates.ERROR;
                                }
                                else
                                {
                                    VM.EditCommandTabReadAddress = _str[1];
                                }

                                if (String.IsNullOrEmpty(_str[2])) //_str[2].Equals(String.Empty)
                                {
                                    VM.EditCommandTabReadLength = String.Empty;
                                    ValidationState["EditCommandTabReadLength"] = ValidationStates.ERROR;
                                }
                                else
                                {
                                    VM.EditCommandTabReadLength = _str[2];
                                }
                            }
                            break;
                        //W
                        case 2:
                            VM.EditCommandTabWriteMemBank = VM.MemBank[int.Parse(_str[0].Substring(1), CultureInfo.CurrentCulture)];
                            VM.EditCommandTabWriteMemBankIndex = int.Parse(_str[0].Substring(1), CultureInfo.CurrentCulture);

                            if (_change is false)
                            {
                                VM.EditCommandTabWriteAddress = _str[1];
                                VM.EditCommandTabWriteLength = _str[2];
                                VM.EditCommandTabWriteData = _str[3];
                                var _idx = Convert.ToInt32(VM.EditCommandTabWriteLength, 16) * 4;
                                VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabWriteData.Length, _idx);

                                ValidationState["EditCommandTabWriteAddress"] = ValidationStates.OK;
                                ValidationState["EditCommandTabWriteLength"] = ValidationStates.OK;
                                ValidationState["EditCommandTabWriteData"] = ValidationStates.OK;
                            }
                            else
                            {
                                if (String.IsNullOrEmpty(_str[1])) //_str[1].Equals(String.Empty)
                                {
                                    VM.EditCommandTabWriteAddress = String.Empty;
                                    ValidationState["EditCommandTabWriteAddress"] = ValidationStates.ERROR;
                                }
                                else
                                {
                                    VM.EditCommandTabWriteAddress = _str[1];
                                }

                                if (String.IsNullOrEmpty(_str[2]))  //_str[2].Equals(String.Empty)
                                {
                                    VM.EditCommandTabWriteLength = String.Empty;
                                    ValidationState["EditCommandTabWriteLength"] = ValidationStates.ERROR;
                                }
                                else
                                {
                                    VM.EditCommandTabWriteLength = _str[2];
                                }

                                if (!VM.EditCommandTabWriteData.Equals(VM.EditCommandTabWriteDataTemp, StringComparison.CurrentCulture))
                                {
                                    VM.EditCommandTabWriteData = VM.EditCommandTabWriteDataTemp;
                                }

                                if (!String.IsNullOrEmpty(_str[2])) //_str[2].Equals(String.Empty)
                                {
                                    var _idx = Convert.ToInt32(VM.EditCommandTabWriteLength, 16) * 4;
                                    VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabWriteData.Length, _idx);

                                    if (_idx.Equals(VM.EditCommandTabWriteData.Length))
                                    {
                                        ValidationState["EditCommandTabWriteData"] = ValidationStates.OK;
                                    }
                                    else
                                    {
                                        ValidationState["EditCommandTabWriteData"] = ValidationStates.ERROR;
                                    }
                                }
                            }
                            
                            break;
                        case 3:
                            if (_str[0].Length == 9)
                                VM.EditCommandTabAccessPassword = _str[0].Substring(1, 8);
                            ValidationState["EditCommandTabAccessPassword"] = ValidationStates.OK;
                            break;
                        case 4:
                            VM.EditCommandTabSelectMemBank = VM.MemBank[int.Parse(_str[0].Substring(1), CultureInfo.CurrentCulture)];
                            VM.EditCommandTabSelectMemBankIndex = int.Parse(_str[0].Substring(1), CultureInfo.CurrentCulture);
                            VM.EditCommandTabSelectAddress = _str[1];
                            VM.EditCommandTabSelectLength = _str[2];
                            VM.EditCommandTabSelectData = _str[3];

                            var _idx1 = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) / 4;
                            var _par = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) % 4;
                            if (_par > 0) _idx1++;
                            VM.EditCommandTabSelectDataIdx = String.Format(CultureInfo.CurrentCulture,"({0} / {1})", VM.EditCommandTabSelectData.Length, _idx1);

                            ValidationState["EditCommandTabSelectAddress"] = ValidationStates.OK;
                            ValidationState["EditCommandTabSelectLength"] = ValidationStates.OK;
                            ValidationState["EditCommandTabSelectData"] = ValidationStates.OK;
                            break;
                        case 5:
                            VM.EditCommandTabLockMask = _str[0].Substring(1, 3);
                            VM.EditCommandTabLockAction = _str[1];

                            ValidationState["EditCommandTabLockMask"] = ValidationStates.OK;
                            ValidationState["EditCommandTabLockAction"] = ValidationStates.OK;

                            NoName();
                            break;
                        case 6:
                            if (_str[0].Contains("U"))
                                VM.EditCommandTabUQRIsChecked = "U";
                            else
                                VM.EditCommandTabUQRIsChecked = "Q";
                            if (_str[0].Length > 1)
                                VM.EditCommandTabUQRSlotQIsCheck = true;

                            if (_str.Length > 1)
                            {
                                VM.EditCommandTabUQRMemBank = VM.MemBank[int.Parse(_str[1].Substring(1), CultureInfo.CurrentCulture)];
                                VM.EditCommandTabUQRMemBankIndex = int.Parse(_str[1].Substring(1), CultureInfo.CurrentCulture);
                                VM.EditCommandTabUQRAddress = _str[2];
                                VM.EditCommandTabUQRLength = _str[3];
                                ValidationState["EditCommandTabUQRAddress"] = ValidationStates.OK;
                                ValidationState["EditCommandTabUQRLength"] = ValidationStates.OK;
                                VM.EditCommandTabUQRReadIsCheck = true;
                            }
                            else
                            {
                                VM.EditCommandTabUQRReadIsCheck = false;//2018
                            }
                            break;
                    }
                }

                if (VM.EditCommandTypeIsChecked is "Standard")
                {
                    VM.EditCommandUpdate = false;
                }
                if (VM.EditCommandTypeIsChecked is null)
                    VM.EditCommandTypeIsChecked = "Standard";
            }
            else//Customize
            {
                if (VM.EditCommandTypeIsChecked is "Customize")
                {
                    VM.EditCommandUpdate = false;
                }
                if (VM.EditCommandTypeIsChecked is null)
                    VM.EditCommandTypeIsChecked = "Customize";
            }

            VM.EditCommandTabControlItemSelectedIndex = B02Item02Commands[idx].TabIndexTemp;

            //DefineSequenceStandardTemp = String.Empty;
            //DefineSequenceCustomizeTemp = String.Empty;

        }

        private void OnEditCommandTypeChecked(object sender, RoutedEventArgs e)
        {

            if (!(sender is RadioButton _radioButton)) return;

            var _str = _radioButton.Tag.ToString();

            if (_str.Equals("Standard", StringComparison.CurrentCulture)) //Standard
            {
                EditCommandTab.Visibility = Visibility.Visible;
                VM.DefineSequenceIsEnabled = false;

                B02Item02Commands[Index].TypeTemp = false;

                if (VM.EditCommandUpdate is true)
                {
                    VM.EditCommandUpdate = false;
                }
                else
                {
                    B02Item02Commands[Index].DefineSequenceCustomizeTemp = VM.DefineSequence;
                    VM.DefineSequence = B02Item02Commands[Index].DefineSequenceStandardTemp;
                    B02Item02Commands[Index].CommandTemp = VM.DefineSequence;

                    var _strArray = VM.DefineSequence.Split(',');
                    switch (B02Item02Commands[Index].TabIndexTemp)
                    {
                        case 1:
                            VM.EditCommandTabReadMemBank = VM.MemBank[int.Parse(_strArray[0].Substring(1), CultureInfo.CurrentCulture)];
                            VM.EditCommandTabReadMemBankIndex = int.Parse(_strArray[0].Substring(1), CultureInfo.CurrentCulture);

                            VM.EditCommandTabReadAddress = _strArray[1];
                            VM.EditCommandTabReadLength = _strArray[2];
                            ValidationState["EditCommandTabReadAddress"] = ValidationStates.OK;
                            ValidationState["EditCommandTabReadLength"] = ValidationStates.OK;

                            break;
                        case 2:

                            VM.EditCommandTabWriteMemBank = VM.MemBank[int.Parse(_strArray[0].Substring(1), CultureInfo.CurrentCulture)];
                            VM.EditCommandTabWriteMemBankIndex = int.Parse(_strArray[0].Substring(1), CultureInfo.CurrentCulture);

                            VM.EditCommandTabWriteAddress = _strArray[1];
                            VM.EditCommandTabWriteLength = _strArray[2];
                            VM.EditCommandTabWriteData = _strArray[3];
                            var _idx = Convert.ToInt32(VM.EditCommandTabWriteLength, 16) * 4;
                            VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabWriteData.Length, _idx);

                            ValidationState["EditCommandTabWriteAddress"] = ValidationStates.OK;
                            ValidationState["EditCommandTabWriteLength"] = ValidationStates.OK;
                            ValidationState["EditCommandTabWriteData"] = ValidationStates.OK;
                            break;
                        case 3:
                            VM.EditCommandTabAccessPassword = _strArray[0].Substring(1, 8);
                            ValidationState["EditCommandTabAccessPassword"] = ValidationStates.OK;
                            break;
                        case 4:
                            VM.EditCommandTabSelectMemBank = VM.MemBank[int.Parse(_strArray[0].Substring(1), CultureInfo.CurrentCulture)];
                            VM.EditCommandTabSelectMemBankIndex = int.Parse(_strArray[0].Substring(1), CultureInfo.CurrentCulture);
                            VM.EditCommandTabSelectAddress = _strArray[1];
                            VM.EditCommandTabSelectLength = _strArray[2];
                            VM.EditCommandTabSelectData = _strArray[3];

                            var _idx1 = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) / 4;
                            var _par = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) % 4;
                            if (_par > 0) _idx1++;
                            VM.EditCommandTabSelectDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabSelectData.Length, _idx1);

                            ValidationState["EditCommandTabSelectAddress"] = ValidationStates.OK;
                            ValidationState["EditCommandTabSelectLength"] = ValidationStates.OK;
                            ValidationState["EditCommandTabSelectData"] = ValidationStates.OK;
                            break;
                        case 5:
                            VM.EditCommandTabLockMask = _strArray[0].Substring(1, 3);
                            VM.EditCommandTabLockAction = _strArray[1];

                            ValidationState["EditCommandTabLockMask"] = ValidationStates.OK;
                            ValidationState["EditCommandTabLockAction"] = ValidationStates.OK;

                            NoName();
                            break;
                        case 6:
                            if (_strArray[0].Contains("U"))
                                VM.EditCommandTabUQRIsChecked = "U";
                            else
                                VM.EditCommandTabUQRIsChecked = "Q";
                            if (_strArray[0].Length > 1)
                                VM.EditCommandTabUQRSlotQIsCheck = true;

                            if (_strArray.Length > 1)
                            {
                                VM.EditCommandTabUQRMemBank = VM.MemBank[int.Parse(_strArray[1].Substring(1), CultureInfo.CurrentCulture)];
                                VM.EditCommandTabUQRMemBankIndex = int.Parse(_strArray[1].Substring(1), CultureInfo.CurrentCulture);
                                VM.EditCommandTabUQRAddress = _strArray[2];
                                VM.EditCommandTabUQRLength = _strArray[3];
                                ValidationState["EditCommandTabUQRAddress"] = ValidationStates.OK;
                                ValidationState["EditCommandTabUQRLength"] = ValidationStates.OK;
                                VM.EditCommandTabUQRReadIsCheck = true;
                            }
                            else
                            {
                                VM.EditCommandTabUQRReadIsCheck = false;//2018
                            }
                            break;
                    }
                }

                //if (VM.EditCommandTypeIsChecked != "Standard")
                 //   VM.EditCommandTypeIsChecked = "Standard";

            }
            else
            {
                EditCommandTab.Visibility = Visibility.Hidden;
                VM.DefineSequenceIsEnabled = true;

                B02Item02Commands[Index].TypeTemp = true;

                if (VM.EditCommandUpdate is true)
                {
                    VM.EditCommandUpdate = false;
                }
                else
                {
                    B02Item02Commands[Index].DefineSequenceStandardTemp = VM.DefineSequence;
                    VM.DefineSequence = B02Item02Commands[Index].DefineSequenceCustomizeTemp;
                    B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                    B02Item02Commands[Index].TabIndexTemp = VM.EditCommandTabControlItemSelectedIndex;
                }

                //if (VM.EditCommandTypeIsChecked != "Customize")
                //    VM.EditCommandTypeIsChecked = "Customize";
                

            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnDeleteSequenceClick(object sender, RoutedEventArgs e)
        {
            /*if (Index == 0 && Index == (B02Item02Commands.Count - 1))
            {
                B02Item02Commands.Remove(B02Item02Commands[Index]);
                EditCommandDialogUpdate(-1);
                return;
            }*/
            if (Index == (B02Item02Commands.Count - 1)) {
                //B02Item02Commands.Remove(B02Item02Commands[Index]);
                //Index--;
                //this.IndexForwardNext.Text = Index.ToString();
            }
            else
                B02Item02Commands.Remove(B02Item02Commands[Index]);


            if (Index == 0)
                this.IndexForwardNext1.IsEnabled = false;
            if (Index == 0 && B02Item02Commands.Count > 1)
                this.IndexForwardNext2.IsEnabled = true;
            else if (Index == (B02Item02Commands.Count - 1))
                this.IndexForwardNext2.IsEnabled = false;
            else
            {
                this.IndexForwardNext1.IsEnabled = true;
                this.IndexForwardNext2.IsEnabled = true;
            }

            EditCommandDialogUpdate(Index);

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnCommandApplyClick(object sender, RoutedEventArgs e)
        {
            MessageBoxResult _result;

            if (EditCommandTab.Visibility is Visibility.Visible)
            {
                switch (VM.EditCommandTabControlItemSelectedIndex)
                {
                    case 0:
                        _result = MessageBox.Show("尚未編輯任何指令。\n\r若要編輯請按[OK]；離開編輯視窗請按[Cancel]",
                            stringManager.GetString("Information", CultureInfo.CurrentCulture),
                            MessageBoxButton.OKCancel, MessageBoxImage.Information);
                        if (_result == MessageBoxResult.OK)
                        {
                            return;
                        }
                        else
                        {
                            this.DialogResult = true;
                            return;
                        }
                    case 1:
                        if ((ValidationStates)ValidationState["EditCommandTabReadAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabReadLength"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02R;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Read(R)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]",
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 2:
                        if ((ValidationStates)ValidationState["EditCommandTabWriteAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabWriteLength"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabWriteData"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02W;
                            this.B02Item02Commands[Index].Command = VM.DefineSequence;
                            VM.EditCommandTabWriteAddress = String.Empty;
                            VM.EditCommandTabWriteLength = String.Empty;
                            VM.EditCommandTabWriteData = String.Empty;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Write(W)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]",
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 3:
                        if ((ValidationStates)ValidationState["EditCommandTabAccessPassword"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02P;
                            this.B02Item02Commands[Index].Command = VM.DefineSequence;
                            VM.EditCommandTabAccessPassword = String.Empty;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Access Password(P)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]",
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 4:
                        if ((ValidationStates)ValidationState["EditCommandTabSelectAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabSelectLength"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabSelectData"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02T;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Select(T)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]",
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    //L
                    case 5:
                        if ((ValidationStates)ValidationState["EditCommandTabLockMask"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabLockAction"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02L;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Lock(L)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]",
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 6:
                        if (VM.EditCommandTabUQRReadIsCheck)
                        {
                            if ((ValidationStates)ValidationState["EditCommandTabUQRAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabUQRLength"] == ValidationStates.OK)
                            {
                                if (VM.EditCommandTabUQRIsChecked is "U")
                                {
                                    if (VM.EditCommandTabUQRReadIsCheck is true)
                                    {
                                        if (VM.EditCommandTabUQRSlotQIsCheck is true)
                                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02URSLOTQ;
                                        else
                                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02UR;
                                    }
                                    else
                                    {
                                        if (VM.EditCommandTabUQRSlotQIsCheck is true)
                                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02USLOTQ;
                                        else
                                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02U;
                                    }
                                }
                                else
                                {
                                    if (VM.EditCommandTabUQRReadIsCheck is true)
                                        this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02QR;
                                    else
                                        this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02Q;
                                }
                            }
                            else
                            {
                                _result = MessageBox.Show("編輯不符合MultiRead(UR),SingleRead(QR)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]",
                                    stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                    MessageBoxButton.OKCancel, MessageBoxImage.Information);
                                if (_result == MessageBoxResult.OK)
                                {
                                    return;
                                }
                                else
                                {
                                    this.DialogResult = true;
                                    return;
                                }
                            }
                        }
                        else
                        {
                            if (VM.EditCommandTabUQRIsChecked is "U")
                            {
                                if (VM.EditCommandTabUQRSlotQIsCheck is true)
                                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02USLOTQ;
                                else
                                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02U;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02Q;
                            }
                        }
                        break;
                }

                this.B02Item02Commands[Index].Type = false;
                this.B02Item02Commands[Index].TypeTemp = false;
                this.B02Item02Commands[Index].TabIndex = VM.EditCommandTabControlItemSelectedIndex;
                this.B02Item02Commands[Index].TabIndexTemp = VM.EditCommandTabControlItemSelectedIndex;
            }
            else
            {
                if (VM.DefineSequence.Length == 0)
                {
                    _result = MessageBox.Show("自訂義指令長度為0。\n\r若要編輯請按[OK]；離開編輯視窗請按[Cancel]",
                        stringManager.GetString("Information", CultureInfo.CurrentCulture),
                        MessageBoxButton.OKCancel, MessageBoxImage.Information);
                    if (_result == MessageBoxResult.OK)
                    {
                        return;
                    }
                    else
                    {
                        this.DialogResult = true;
                        return;
                    }
                }
                else
                {
                    this.B02Item02Commands[Index].Type = true;
                    this.B02Item02Commands[Index].TypeTemp = true;
                    this.B02Item02Commands[Index].TabIndex = 0;
                    this.B02Item02Commands[Index].TabIndexTemp = 0;
                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02CUSTOMIZE;
                }
                
            }
            this.B02Item02Commands[Index].Command = VM.DefineSequence;
            this.B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
            this.B02Item02Commands[Index].Name = VM.DefineName;
            this.B02Item02Commands[Index].ApplyButton = false;

            EditCommandPassValuesReceive(Index, this.B02Item02Commands[Index]);
            ApplySequenceBtn.IsEnabled = false;


            if (Index == B02Item02Commands.Count - 1)
            {
                B02Item02Commands.Add(new B02Item02Command());
                OnIndexForwardNextChecked2(null, null);
            }

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnCommandOkClick(object sender, RoutedEventArgs e)
        {
            MessageBoxResult _result;

            if (EditCommandTab.Visibility is Visibility.Visible)
            {
                switch (VM.EditCommandTabControlItemSelectedIndex)
                {
                    case 0:
                        _result = MessageBox.Show("尚未編輯任何指令。\n\r若要編輯請按[OK]；離開編輯視窗請按[Cancel]", 
                            stringManager.GetString("Information", CultureInfo.CurrentCulture),
                            MessageBoxButton.OKCancel, MessageBoxImage.Information);
                        if (_result == MessageBoxResult.OK)
                        {
                            return;
                        }
                        else
                        {
                            this.DialogResult = true;
                            return;
                        }
                    case 1:
                        if ((ValidationStates)ValidationState["EditCommandTabReadAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabReadLength"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02R;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Read(R)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].TabIndexTemp = this.B02Item02Commands[Index].TabIndex;
                                this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 2:
                        if ((ValidationStates)ValidationState["EditCommandTabWriteAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabWriteLength"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabWriteData"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02W;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Write(W)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].TabIndexTemp = this.B02Item02Commands[Index].TabIndex;
                                this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 3:
                        if ((ValidationStates)ValidationState["EditCommandTabAccessPassword"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02P;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Access Password(P)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].TabIndexTemp = this.B02Item02Commands[Index].TabIndex;
                                this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 4:
                        if ((ValidationStates)ValidationState["EditCommandTabSelectAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabSelectLength"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabSelectData"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02T;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Select(T)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].TabIndexTemp = this.B02Item02Commands[Index].TabIndex;
                                this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    case 5:
                        if ((ValidationStates)ValidationState["EditCommandTabLockMask"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabLockAction"] == ValidationStates.OK)
                        {
                            this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02L;
                        }
                        else
                        {
                            _result = MessageBox.Show("編輯不符合Lock(L)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                MessageBoxButton.OKCancel, MessageBoxImage.Information);
                            if (_result == MessageBoxResult.OK)
                            {
                                return;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].TabIndexTemp = this.B02Item02Commands[Index].TabIndex;
                                this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
                                this.DialogResult = true;
                                return;
                            }
                        }
                        break;
                    //U,UR
                    case 6:
                        if (VM.EditCommandTabUQRReadIsCheck)
                        {
                            if ((ValidationStates)ValidationState["EditCommandTabUQRAddress"] == ValidationStates.OK &&
                            (ValidationStates)ValidationState["EditCommandTabUQRLength"] == ValidationStates.OK)
                            {
                                if (VM.EditCommandTabUQRIsChecked is "U")
                                {
                                    if (VM.EditCommandTabUQRSlotQIsCheck is true)
                                        this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02URSLOTQ;
                                    else
                                        this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02UR;
                                }
                                else
                                {
                                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02QR;
                                } 
                            }
                            else
                            {
                                _result = MessageBox.Show("編輯不符合MultiRead(UR),SingleRead(QR)指令格式。\n\r若要重新編輯請按[OK]；離開編輯請按[Cancel]", 
                                    stringManager.GetString("Information", CultureInfo.CurrentCulture),
                                    MessageBoxButton.OKCancel, MessageBoxImage.Information);
                                if (_result == MessageBoxResult.OK)
                                {
                                    return;
                                }
                                else
                                {
                                    this.B02Item02Commands[Index].TabIndexTemp = this.B02Item02Commands[Index].TabIndex;
                                    this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
                                    this.DialogResult = true;
                                    return;
                                }
                            }
                        }
                        else
                        {
                            if (VM.EditCommandTabUQRIsChecked is "U")
                            {
                                if (VM.EditCommandTabUQRSlotQIsCheck is true)
                                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02USLOTQ;
                                else
                                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02U;
                            }
                            else
                            {
                                this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02Q;
                            }
                        }
                        
                        break;
                }

                this.B02Item02Commands[Index].Type = false;
                this.B02Item02Commands[Index].TypeTemp = false;
                this.B02Item02Commands[Index].TabIndex = VM.EditCommandTabControlItemSelectedIndex;
                this.B02Item02Commands[Index].TabIndexTemp = VM.EditCommandTabControlItemSelectedIndex;
            }
            else
            {
                if (VM.DefineSequence.Length == 0)
                {
                    _result = MessageBox.Show("自訂義指令長度為0。\n\r若要編輯請按[OK]；離開編輯視窗請按[Cancel]", 
                        stringManager.GetString("Information", CultureInfo.CurrentCulture),
                        MessageBoxButton.OKCancel, MessageBoxImage.Information);
                    if (_result == MessageBoxResult.OK)
                    {
                        return;
                    }
                    else
                    {
                        this.DialogResult = true;
                        return;
                    }
                }
                else
                {
                    this.B02Item02Commands[Index].Type = true;
                    this.B02Item02Commands[Index].TypeTemp = true;
                    this.B02Item02Commands[Index].TabIndex = 0;
                    this.B02Item02Commands[Index].TabIndexTemp = 0;
                    this.B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02CUSTOMIZE;
                }
                
            }

            this.B02Item02Commands[Index].Name = VM.DefineName ;
            this.B02Item02Commands[Index].Command = VM.DefineSequence;
            //this.B02Item02Commands[Index].CommandTemp = this.B02Item02Commands[Index].Command;
            //this.B02Item02Commands[Index].OKButton = false;
            for (int i = 0; i < this.B02Item02Commands.Count; i++)
                this.B02Item02Commands[i].CommandTemp = this.B02Item02Commands[i].Command;

            if (Index == (this.B02Item02Commands.Count - 1))
                this.B02Item02Commands.Add(new B02Item02Command());
            this.DialogResult = true;
        }

        private void OnSendSequenceCancelClick(object sender, RoutedEventArgs e)
        {
            this.DialogResult = false;
        }

        public ObservableCollection<B02Item02Command> GetSequences() { return this.B02Item02Commands; }

        private void EditCommandTabControlItemSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (e.Source is TabControl)
            {
                if (sender is TabControl tc)
                {
                    if (((TabItem)tc.SelectedItem) != null)
                        TabCtrlHeader = ((TabItem)tc.SelectedItem).Header.ToString();

                    this.B02Item02Commands[Index].TabIndexTemp = tc.SelectedIndex;
                    switch (tc.SelectedIndex)
                    {
                        case 1:
                            if (String.IsNullOrEmpty(VM.EditCommandTabReadAddress))
                                ValidationState["EditCommandTabReadAddress"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabReadLength))
                                ValidationState["EditCommandTabReadLength"] = ValidationStates.DEFAULF;

                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "R{0},{1},{2}", VM.EditCommandTabReadMemBankIndex, VM.EditCommandTabReadAddress, VM.EditCommandTabReadLength);
                            break;
                        case 2:
                            if (String.IsNullOrEmpty(VM.EditCommandTabWriteAddress))
                                ValidationState["EditCommandTabWriteAddress"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabWriteLength))
                                ValidationState["EditCommandTabWriteLength"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabWriteData))
                                ValidationState["EditCommandTabWriteData"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabWriteLength))
                            {
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "W{0},{1},{2},{3}", VM.EditCommandTabWriteMemBankIndex, VM.EditCommandTabWriteAddress, String.Empty, String.Empty);
                            }
                            else
                            {
                                var _idx = Convert.ToInt32(VM.EditCommandTabWriteLength, 16) * 4;
                                VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabWriteData.Length, _idx);

                                if (_idx.Equals(VM.EditCommandTabWriteData.Length))
                                {
                                    ValidationState["EditCommandTabWriteData"] = ValidationStates.OK;
                                }
                                else
                                {
                                    ValidationState["EditCommandTabWriteData"] = ValidationStates.ERROR;
                                }
                            }
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "W{0},{1},{2},{3}", VM.EditCommandTabWriteMemBankIndex, VM.EditCommandTabWriteAddress, VM.EditCommandTabWriteLength, VM.EditCommandTabWriteData);
                            break;
                        //P
                        case 3:
                            if (String.IsNullOrEmpty(VM.EditCommandTabAccessPassword))
                                ValidationState["EditCommandTabAccessPassword"] = ValidationStates.DEFAULF;

                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "P{0}", VM.EditCommandTabAccessPassword);
                            break;
                        //S
                        case 4:
                            if (String.IsNullOrEmpty(VM.EditCommandTabSelectAddress))
                                ValidationState["EditCommandTabSelectAddress"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabSelectLength))
                                ValidationState["EditCommandTabSelectLength"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabSelectData))
                                ValidationState["EditCommandTabSelectData"] = ValidationStates.DEFAULF;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "T{0},{1},{2},{3}", VM.EditCommandTabSelectMemBankIndex, VM.EditCommandTabSelectAddress, VM.EditCommandTabSelectLength, VM.EditCommandTabSelectData);
                            break;
                        case 5:
                            if (String.IsNullOrEmpty(VM.EditCommandTabLockMask))
                                ValidationState["EditCommandTabLockMask"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabLockAction))
                                ValidationState["EditCommandTabLockAction"] = ValidationStates.DEFAULF;
                            NoName();
                            break;
                        case 6:
                            if (VM.EditCommandTabUQRIsChecked == null)
                                VM.EditCommandTabUQRIsChecked = "U";
                            if (String.IsNullOrEmpty(VM.EditCommandTabUQRAddress))
                                ValidationState["EditCommandTabUQRAddress"] = ValidationStates.DEFAULF;
                            if (String.IsNullOrEmpty(VM.EditCommandTabUQRLength))
                                ValidationState["EditCommandTabUQRLength"] = ValidationStates.DEFAULF;
                            if (VM.EditCommandTabUQRSlotTitlexVisibility == Visibility.Visible)
                            {
                                if (VM.EditCommandTabUQRReadIsCheck is true)
                                {
                                    if (VM.EditCommandTabUQRSlotQIsCheck == true)
                                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0},R{1},{2},{3}", VM.EditCommandTabUQRSlotQComboBox.Tag, VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                                    else
                                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                                }
                                else
                                {
                                    if (VM.EditCommandTabUQRSlotQIsCheck == true)
                                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0}", VM.EditCommandTabUQRSlotQComboBox.Tag);
                                    else
                                        VM.DefineSequence = "U";
                                }
                            }
                            else
                            {
                                if (VM.EditCommandTabUQRReadIsCheck is true)
                                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "Q,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                                else
                                    VM.DefineSequence = "Q";
                            }

                            break;
                        default:
                            break;
                    }
                    if (tc.SelectedIndex != 0)
                        this.B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                }

                //e.Handled = true;
            }
            e.Handled = true;
        }


        #region === Tab Raed ===
        private void OnEditCommandTabReadMemBankDownClosed(object sender, EventArgs e)
        {
            var _idx = (sender as ComboBox).SelectedIndex;
            switch (_idx)
            {
                case 0: VM.EditCommandTabReadAddress = "0"; VM.EditCommandTabReadLength = "4"; break;
                case 1: VM.EditCommandTabReadAddress = "2"; VM.EditCommandTabReadLength = "6"; break;
                case 2: VM.EditCommandTabReadAddress = "0"; VM.EditCommandTabReadLength = "4"; break;
                case 3: VM.EditCommandTabReadAddress = "0"; VM.EditCommandTabReadLength = "1"; break;
                case 4: VM.EditCommandTabReadAddress = ""; VM.EditCommandTabReadLength = ""; break;
                case 5: VM.EditCommandTabReadAddress = "0"; VM.EditCommandTabReadLength = "2"; break;
                case 6: VM.EditCommandTabReadAddress = "2"; VM.EditCommandTabReadLength = "2"; break;
            }
            if (_idx == 4)
            {
                ValidationState["EditCommandTabReadAddress"] = ValidationStates.DEFAULF;
                ValidationState["EditCommandTabReadLength"] = ValidationStates.DEFAULF;
                VM.DefineSequence = String.Empty;
            }
            else
            {
                ValidationState["EditCommandTabReadAddress"] = ValidationStates.OK;
                ValidationState["EditCommandTabReadLength"] = ValidationStates.OK;
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "R{0},{1},{2}", VM.EditCommandTabReadMemBank.Tag, VM.EditCommandTabReadAddress, VM.EditCommandTabReadLength);
            }

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }
            

        }

        private void EditCommandTabReadAddressMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabReadAddressPop.IsOpen = true;
        }

        private void EditCommandTabReadAddressMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabReadAddressPop.IsOpen = false;
        }

        private void EditCommandTabReadLengthMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabReadLengthPop.IsOpen = true;
        }

        private void EditCommandTabReadLengthMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabReadLengthPop.IsOpen = false;
        }
        #endregion

        #region === Tab Write ===
        private void OnEditCommandTabWriteMemBankDownClosed(object sender, EventArgs e)
        {
            var _sel = (sender as ComboBox).SelectedIndex;
            switch (_sel)
            {
                case 0: VM.EditCommandTabWriteAddress = "0"; VM.EditCommandTabWriteLength = "4"; break;
                case 1: VM.EditCommandTabWriteAddress = "2"; VM.EditCommandTabWriteLength = "6"; break;
                case 2: VM.EditCommandTabWriteAddress = "0"; VM.EditCommandTabWriteLength = "4"; break;
                case 3: VM.EditCommandTabWriteAddress = "0"; VM.EditCommandTabWriteLength = "1"; break;
                case 4: VM.EditCommandTabWriteAddress = ""; VM.EditCommandTabWriteLength = ""; break;
                case 5: VM.EditCommandTabWriteAddress = "0"; VM.EditCommandTabWriteLength = "2"; break;
                case 6: VM.EditCommandTabWriteAddress = "2"; VM.EditCommandTabWriteLength = "2"; break;
            }

            if (!_sel.Equals(4))
            {
                EditCommandTabWriteAddress.IsEnabled = true;
                EditCommandTabWriteLength.IsEnabled = true;
                EditCommandTabWriteData.IsEnabled = true;

                var _idx = Convert.ToInt32(VM.EditCommandTabWriteLength, 16) * 4;
                VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabWriteData.Length, _idx);

                if (_idx.Equals(VM.EditCommandTabWriteData.Length))
                {
                    ValidationState["EditCommandTabWriteAddress"] = ValidationStates.OK;
                    ValidationState["EditCommandTabWriteLength"] = ValidationStates.OK;
                    EditCommandTabWriteData.Style = (Style)FindResource("TextBoxNormalStyle");
                    ValidationState["EditCommandTabWriteData"] = ValidationStates.OK;
                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "W{0},{1},{2},{3}",
                        VM.EditCommandTabWriteMemBankIndex, 
                        VM.EditCommandTabWriteAddress, 
                        VM.EditCommandTabWriteLength, 
                        VM.EditCommandTabWriteData);
                    VM.EditCommandTabWriteDataTemp = VM.EditCommandTabWriteData;
                }   
                else
                {
                    ValidationState["EditCommandTabWriteAddress"] = ValidationStates.OK;
                    ValidationState["EditCommandTabWriteLength"] = ValidationStates.OK;

                    EditCommandTabWriteData.Style = (Style)FindResource("TextBoxErrorStyle");
                    ValidationState["EditCommandTabWriteData"] = ValidationStates.ERROR;
                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "W{0},{1},{2},{3}",
                        VM.EditCommandTabWriteMemBankIndex, 
                        VM.EditCommandTabWriteAddress, 
                        VM.EditCommandTabWriteLength, 
                        VM.EditCommandTabWriteDataTemp); //String.Empty
                }    
            }
            else
            {
                VM.EditCommandTabWriteDataIdx = String.Empty;
                VM.DefineSequence = String.Empty;
                EditCommandTabWriteAddress.IsEnabled = false;
                EditCommandTabWriteLength.IsEnabled = false;
                EditCommandTabWriteData.IsEnabled = false;
                ValidationState["EditCommandTabWriteAddress"] = ValidationStates.DEFAULF;
                ValidationState["EditCommandTabWriteLength"] = ValidationStates.DEFAULF;
                ValidationState["EditCommandTabWriteData"] = ValidationStates.DEFAULF;
            }

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            } 
        }

        private void EditCommandTabWriteAddressMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabWriteAddressPop.IsOpen = true;
        }

        private void EditCommandTabWriteAddressMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabWriteAddressPop.IsOpen = false;
        }

        private void EditCommandTabWriteLengthMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabWriteLengthPop.IsOpen = true;
        }

        private void EditCommandTabWriteLengthMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabWriteLengthPop.IsOpen = false;
        }
        #endregion

        #region === Tab Access ===
        private void EditCommandTabAccessPasswordMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabAccessPasswordPop.IsOpen = true;
        }

        private void EditCommandTabAccessPasswordMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabAccessPasswordPop.IsOpen = false;
        }
        #endregion

        #region === Tab UQR ===
        private void OnEditCommandTabUQRMemBankDownClosed(object sender, EventArgs e)
        {
            var _idx = (sender as ComboBox).SelectedIndex;
            switch (_idx)
            {
                case 0: VM.EditCommandTabUQRAddress = "0"; VM.EditCommandTabUQRLength = "4"; break;
                case 1: VM.EditCommandTabUQRAddress = "2"; VM.EditCommandTabUQRLength = "6"; break;
                case 2: VM.EditCommandTabUQRAddress = "0"; VM.EditCommandTabUQRLength = "4"; break;
                case 3: VM.EditCommandTabUQRAddress = "0"; VM.EditCommandTabUQRLength = "1"; break;
                case 4: VM.EditCommandTabUQRAddress = ""; VM.EditCommandTabUQRLength = ""; break;
                case 5: VM.EditCommandTabUQRAddress = "0"; VM.EditCommandTabUQRLength = "2"; break;
                case 6: VM.EditCommandTabUQRAddress = "2"; VM.EditCommandTabUQRLength = "2"; break;
            }

            if (_idx == 4)
            {
                ValidationState["EditCommandTabUQRAddress"] = ValidationStates.DEFAULF;
                ValidationState["EditCommandTabUQRLength"] = ValidationStates.DEFAULF;
                VM.DefineSequence = String.Empty;
            }
            else
            {
                ValidationState["EditCommandTabUQRAddress"] = ValidationStates.OK;
                ValidationState["EditCommandTabUQRLength"] = ValidationStates.OK;

                if (VM.EditCommandTabUQRSlotTitlexVisibility == Visibility.Visible)
                {
                    if (VM.EditCommandTabUQRSlotQIsCheck == true)
                    {
                        if (VM.EditCommandTabUQRReadIsCheck == true)
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0},R{1},{2},{3}", VM.EditCommandTabUQRSlotQComboBox.Tag, VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                        else
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0}", VM.EditCommandTabUQRSlotQComboBox.Tag);
                    }   
                    else
                    {
                        if (VM.EditCommandTabUQRReadIsCheck == true)
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                        else
                            VM.DefineSequence = "U";
                    }      
                }
                else
                {
                    if (VM.EditCommandTabUQRReadIsCheck == true)
                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "Q,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                    else
                        VM.DefineSequence = "Q";
                }  
            }

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }  
        }

        private void OnEditCommandTabUQRChecked(object sender, RoutedEventArgs e)
        {
            if (!(sender is RadioButton _radioButton)) return;

            var _str = _radioButton.Tag.ToString();
            if (_str.Equals("U", StringComparison.CurrentCulture))
            {
                VM.EditCommandTabUQRSlotTitlexVisibility = Visibility.Visible;
                VM.EditCommandTabUQRSlotComboBoxVisibility = Visibility.Visible;

                if (VM.EditCommandTabUQRReadIsCheck is true)
                {
                    if (VM.EditCommandTabUQRSlotQIsCheck == true)
                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0},R{1},{2},{3}", VM.EditCommandTabUQRSlotQComboBox.Tag ?? "1", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                    else
                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                }
                else
                {
                    if (VM.EditCommandTabUQRSlotQIsCheck == true)
                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0}", VM.EditCommandTabUQRSlotQComboBox.Tag ?? "1");
                    else
                        VM.DefineSequence = "U";
                }
                
            }
            else
            {
                VM.EditCommandTabUQRSlotTitlexVisibility = Visibility.Hidden;
                VM.EditCommandTabUQRSlotComboBoxVisibility = Visibility.Hidden;

                if (VM.EditCommandTabUQRReadIsCheck is true)
                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "Q,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                else
                    VM.DefineSequence = "Q";
            }

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }

        }

        private void OnEditCommandTabUQRReadChecked(object sender, RoutedEventArgs e)
        {
            if (VM.EditCommandTabUQRIsChecked is "U")
            {
                if(VM.EditCommandTabUQRSlotQIsCheck)
                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0},R{1},{2},{3}", VM.EditCommandTabUQRSlotQComboBox.Tag ?? "1", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
                else
                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
            }
            else
            {
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "Q,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
            }

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }
        }

        private void OnEditCommandTabUQRReadUnChecked(object sender, RoutedEventArgs e)
        {
            if (VM.EditCommandTabUQRIsChecked is "U")
                if (VM.EditCommandTabUQRSlotQIsCheck)
                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0}", VM.EditCommandTabUQRSlotQComboBox.Tag ?? "1");
                else
                    VM.DefineSequence = "U";
            else
                VM.DefineSequence = "Q";

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }
        }

        private void OnEditCommandTabUQRSlotQChecked(object sender, RoutedEventArgs e)
        {
            if (VM.EditCommandTabUQRReadIsCheck)
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0},R{1},{2},{3}", VM.EditCommandTabUQRSlotQComboBox.Tag ?? "1", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
            else
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U{0}", VM.EditCommandTabUQRSlotQComboBox.Tag ?? "1");

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }
        }

        private void OnEditCommandTabUQRSlotQUnChecked(object sender, RoutedEventArgs e)
        {
            if (VM.EditCommandTabUQRReadIsCheck)
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "U,R{0},{1},{2}", VM.EditCommandTabUQRMemBankIndex, VM.EditCommandTabUQRAddress, VM.EditCommandTabUQRLength);
            else
                VM.DefineSequence = "U";

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }
        }
        #endregion

        #region === Tab Select ===
        private void OnEditCommandTabSelectMemBankDownClosed(object sender, EventArgs e)
        {
            var _idx = (sender as ComboBox).SelectedIndex;

            if (_idx == 4)
            {
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "T{0},{1},{2},{3}", String.Empty, 
                    VM.EditCommandTabSelectAddress, VM.EditCommandTabSelectLength, VM.EditCommandTabSelectData);
            }
            else
            {
                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "T{0},{1},{2},{3}", VM.EditCommandTabSelectMemBank.Tag,
                    VM.EditCommandTabSelectAddress, VM.EditCommandTabSelectLength, VM.EditCommandTabSelectData);
            }

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }

        }

        private void EditCommandTabSelectAddressMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabSelectAddressPop.IsOpen = true;
        }

        private void EditCommandTabSelectAddressMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabSelectAddressPop.IsOpen = false;
        }

        private void EditCommandTabSelectLengthMouseMove(object sender, MouseEventArgs e)
        {
            EditCommandTabSelectLengthPop.IsOpen = true;
        }

        private void EditCommandTabSelectLengthMouseLeave(object sender, MouseEventArgs e)
        {
            EditCommandTabSelectLengthPop.IsOpen = false;
        }
        #endregion

        #region === Tab Lock ===
        private Int32 LockPayloadMask(Int32 mask, Int32 index)
        {
            if (mask == 0) return 0x0;
            else
            {
                this.Action_ |= (mask - 1) << index;
                if (((mask - 1) & 1) == 0) mask = 2;
                else mask = 3;
                return mask << index;
            }
        }

        private void OnEditCommandLockComboBoxDownClosed(object sender, EventArgs e)
        {
            this.Mask_ = 0;
            this.Action_ = 0;

            this.Mask_ |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)EditCommandLockKillPwd.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 8);
            this.Mask_ |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)EditCommandLockAccessPwd.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 6);
            this.Mask_ |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)EditCommandLockEPC.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 4);
            this.Mask_ |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)EditCommandLockTID.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 2);
            this.Mask_ |= LockPayloadMask(Convert.ToInt32(((ComboBoxItem)EditCommandLockUser.SelectedItem).Tag.ToString(), CultureInfo.CurrentCulture), 0);

            VM.EditCommandTabLockMask = this.Mask_.ToString("X3", CultureInfo.CurrentCulture);
            //TextBoxGotFocusValidation(EditCommandTabLockMask, null);
            TextBoxKeyUpValidation(EditCommandTabLockMask, null);
            VM.EditCommandTabLockAction = this.Action_.ToString("X3", CultureInfo.CurrentCulture);
            //TextBoxGotFocusValidation(EditCommandTabLockAction, null);
            TextBoxKeyUpValidation(EditCommandTabLockAction, null);

            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "L{0},{1}", this.Mask_.ToString("X3", CultureInfo.CurrentCulture), this.Action_.ToString("X3", CultureInfo.CurrentCulture));

            if (B02Item02Commands[Index].CommandTemp != VM.DefineSequence)
            {
                B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
                ApplySequenceBtn.IsEnabled = true;
                this.B02Item02Commands[Index].ApplyButton = true;
            }

        }
        #endregion

        #region ===TextBox Validation===
        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxPreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            Regex regex = new Regex(@"^[0-9A-F]+$");
            TextBox tbox = (TextBox)sender;
            String _str = tbox.Text;

            if (!regex.IsMatch(e.Text))
                e.Handled = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxPreviewKeyDownValidation(object sender, KeyEventArgs e)
        {
            TextBox _tbox = (TextBox)sender;

            if ((e.Key >= Key.D0 && e.Key <= Key.D9) || (e.Key >= Key.NumPad0 && e.Key <= Key.NumPad9) 
                || (e.Key >= Key.A && e.Key <= Key.F)
                || e.Key == Key.Back || e.Key == Key.Left || e.Key == Key.Right)
            {
                //e.Handled = false;
                switch (_tbox.Name)
                {
                    case "EditCommandTabWriteData":
                        if (String.IsNullOrEmpty(VM.EditCommandTabWriteLength))
                        {
                            e.Handled = true;
                            EditCommandTabWriteLength.Focus();
                            var _result = MessageBox.Show("請先編輯[Length]。", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture), MessageBoxButton.OK, MessageBoxImage.Information);
                        }
                        break;
                    case "EditCommandTabSelectData":
                        if (String.IsNullOrEmpty(VM.EditCommandTabSelectLength))
                        {
                            e.Handled = true;
                            EditCommandTabSelectLength.Focus();
                            var _result = MessageBox.Show("請先編輯[Length]。", 
                                stringManager.GetString("Information", CultureInfo.CurrentCulture), MessageBoxButton.OK, MessageBoxImage.Information);
                        }
                        break;
                }
            }
            else
            {
                e.Handled = true;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxPreviewKeyDownValidationAll(object sender, KeyEventArgs e)
        {
            TextBox _tbox = (TextBox)sender;

            if ((e.Key >= Key.D0 && e.Key <= Key.D9) || (e.Key >= Key.NumPad0 && e.Key <= Key.NumPad9)
                || (e.Key >= Key.A && e.Key <= Key.Z )
                || e.Key == Key.Back || e.Key == Key.Left || e.Key == Key.Right || e.Key == Key.OemComma)
            {
                //e.Handled = false;
                
            }
            else
            {
                e.Handled = true;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxKeyUpValidation(object sender, KeyEventArgs e)
        {
            Regex regex = new Regex(@"^[0-9A-F]+$");
            TextBox _tbox = (TextBox)sender;
            String _str = _tbox.Text;
            String[] _temp = Array.Empty<string>();
            Int32 _idx, _par;

            if (regex.IsMatch(_str)) //e.Key.ToString()
            {
                if ((ValidationStates)ValidationState[_tbox.Name] != ValidationStates.OK)
                    _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                ValidationState[_tbox.Name] = ValidationStates.OK;

                if (String.IsNullOrEmpty(_str))
                {
                    _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                    ValidationState[_tbox.Name] = ValidationStates.ERROR;
                    goto TextBoxExit;
                }

                switch (_tbox.Name)
                {
                    case "EditCommandTabReadAddress":
                    case "EditCommandTabWriteAddress":
                        if (Convert.ToInt32(_str, 16) > 0x3FFF)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;

                            _temp = VM.DefineSequence.Split(',');
                            _temp[1] = _str;

                            if (_tbox.Name == "EditCommandTabReadAddress")
                            {
                                //VM.EditCommandTabReadAddressTemp = _temp[1];
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2}", _temp[0], _temp[1], _temp[2]);
                            } 
                            else
                            {
                                VM.EditCommandTabWriteAddressTemp = _temp[1];
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2}, {3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                            }
                                
                        }
                        break;
                    case "EditCommandTabReadLength":
                    case "EditCommandTabWriteLength":
                        if (Convert.ToInt32(_str, 16) > 0x20 || Convert.ToInt32(_str, 16) < 1)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;

                            _temp = VM.DefineSequence.Split(',');
                            _temp[2] = _str;

                            if (_tbox.Name == "EditCommandTabReadLength")
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2}", _temp[0], _temp[1], _temp[2]);
                            else
                            {
                                VM.EditCommandTabWriteLengthTemp = _temp[2];
                                if (_temp[2].Length > 0)
                                {
                                    _idx = Convert.ToInt32(_temp[2], 16) * 4;
                                    VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", VM.EditCommandTabWriteData.Length, _idx);

                                    if (_idx.Equals(VM.EditCommandTabWriteData.Length))
                                    {
                                        EditCommandTabWriteData.Style = (Style)FindResource("TextBoxNormalStyle");
                                        ValidationState[EditCommandTabWriteData.Name] = ValidationStates.OK;
                                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], VM.EditCommandTabWriteData);
                                    }  
                                    else
                                    {
                                        EditCommandTabWriteData.Style = (Style)FindResource("TextBoxErrorStyle");
                                        ValidationState[EditCommandTabWriteData.Name] = ValidationStates.ERROR;
                                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                                    }
                                    VM.EditCommandTabWriteDataTemp = VM.EditCommandTabWriteData;
                                }
                                else
                                {
                                    VM.EditCommandTabWriteDataIdx = String.Empty;
                                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                                }
                            }    
                        }
                        break;
                    case "EditCommandTabWriteData":
                        var _nWordsLength = Convert.ToInt32(VM.EditCommandTabWriteLength, 16) * 4;
                        _temp = VM.DefineSequence.Split(',');
                        _temp[3] = _str;

                        if (_nWordsLength!= _tbox.Text.Length)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], String.Empty);
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        }

                        VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", _temp[3].Length, _nWordsLength);
                        break;
                    case "EditCommandTabAccessPassword":
                        if (String.IsNullOrEmpty(_str) || _str.Length < 8)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "P{0}", _str);
                        }
                        break;
                    case "EditCommandTabUQRAddress":
                        if (Convert.ToInt32(_str, 16) > 0x3FFF)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;

                            _temp = VM.DefineSequence.Split(',');
                            if (_temp.Length == 4)
                            {
                                _temp[2] = _str;
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                            }
                        }
                        break;
                    case "EditCommandTabUQRLength":
                        if (Convert.ToInt32(_str, 16) > 0x20 || Convert.ToInt32(_str, 16) < 1)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;

                            _temp = VM.DefineSequence.Split(',');
                            if(_temp.Length == 4)
                            {
                                _temp[3] = _str;
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                            }
                        }
                        break;
                    case "EditCommandTabSelectAddress":
                        if (Convert.ToInt32(_str, 16) > 0x3FFF)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;

                            _temp = VM.DefineSequence.Split(',');
                            _temp[1] = _str;

                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        }
                        break;
                    case "EditCommandTabSelectLength":
                        if (Convert.ToInt32(_str, 16) > 0x60 || Convert.ToInt32(_str, 16) < 1)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;

                            _temp = VM.DefineSequence.Split(',');
                            _temp[2] = _str;
                            _temp[3] = VM.EditCommandTabSelectData;

                            if (_temp[2].Length > 0)
                            {
                                _idx = Convert.ToInt32(VM.EditCommandTabSelectLength, 16)/4;
                                _par = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) % 4;
                                if (_par > 0) _idx++;
                                VM.EditCommandTabSelectDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", _temp[3].Length, _idx);

                                //if (_idx >= VM.EditCommandTabSelectData.Length && ((_idx - 3) <= VM.EditCommandTabSelectData.Length))
                                if (_idx != VM.EditCommandTabSelectData.Length)
                                {
                                    EditCommandTabSelectData.Style = (Style)FindResource("TextBoxErrorStyle");
                                    ValidationState["EditCommandTabSelectData"] = ValidationStates.ERROR;
                                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], String.Empty);
                                } 
                                else
                                {
                                    EditCommandTabSelectData.Style = (Style)FindResource("TextBoxNormalStyle");
                                    ValidationState["EditCommandTabSelectData"] = ValidationStates.OK;
                                    VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                                }  
                            }
                            else
                            {
                                VM.EditCommandTabSelectDataIdx = String.Empty;
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                            }
                        }
                        break;

                    case "EditCommandTabSelectData":
                        var _nBitsLength = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) / 4;
                        _par = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) % 4;
                        if (_par > 0) _nBitsLength++;
                        _temp = VM.DefineSequence.Split(',');
                        _temp[3] = _str;

                        if (_nBitsLength  != _tbox.Text.Length)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], String.Empty);
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        }

                        VM.EditCommandTabSelectDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", _temp[3].Length, _nBitsLength);
                        break;
                    case "EditCommandTabLockMask":
                        if (String.IsNullOrEmpty(_tbox.Text) || _tbox.Text.Length != 3 || Convert.ToInt32(_tbox.Text, 16) > 0x3FF)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "L{0},{1}", String.Empty, String.Empty);
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "L{0},{1}", _tbox.Text, VM.EditCommandTabLockAction);
                        }
                        break;
                    case "EditCommandTabLockAction":
                        if (String.IsNullOrEmpty(_tbox.Text) || _tbox.Text.Length != 3 || Convert.ToInt32(_tbox.Text, 16) > 0x3FF)
                        {
                            _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[_tbox.Name] = ValidationStates.ERROR;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "L{0},{1}", String.Empty, String.Empty);
                        }
                        else
                        {
                            _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[_tbox.Name] = ValidationStates.OK;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "L{0},{1}", VM.EditCommandTabLockMask, _tbox.Text);
                        }
                        break;
                }

            }
            else
            {
                if (!(_tbox.Name is "DefineSequence"))
                {
                    _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                    ValidationState[_tbox.Name] = ValidationStates.ERROR;

                    _temp = VM.DefineSequence.Split(',');
                }

                switch (_tbox.Name)
                {
                    case "DefineSequence":
                        _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                        ValidationState[_tbox.Name] = ValidationStates.OK;
                        break;
                    case "EditCommandTabReadAddress":
                    case "EditCommandTabWriteAddress":
                        _temp[1] = _str;

                        if (_tbox.Name == "EditCommandTabReadAddress")
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2}", _temp[0], _temp[1], _temp[2]);
                        else
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        break;

                    case "EditCommandTabReadLength":
                    case "EditCommandTabWriteLength":
                        _temp[2] = _str;

                        if (_tbox.Name == "EditCommandTabReadLength")
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2}", _temp[0], _temp[1], _temp[2]);
                        else
                        {
                            VM.EditCommandTabWriteLengthTemp = _temp[2];
                            if (_temp[2].Length == 0)
                            {
                                VM.EditCommandTabWriteDataIdx = String.Empty;
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], String.Empty);
                            }
                            else    
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        }
                            
                        
                        break;
                    case "EditCommandTabWriteData":
                        _temp[3] = _str;
                        VM.EditCommandTabWriteDataTemp = _temp[3];
                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);

                        _idx = Convert.ToInt32(_temp[2], 16) * 4;
                        VM.EditCommandTabWriteDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", _temp[3].Length, _idx);
                        break;
                    case "EditCommandTabAccessPassword":
                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "P{0}", _str);
                        break;
                    case "EditCommandTabUQRAddress":
                        if (_temp.Length == 4)
                        {
                            _temp[2] = _str;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        }
                        break;
                    case "EditCommandTabUQRLength":
                        if (_temp.Length == 4)
                        {
                            _temp[3] = _str;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        }
                        break;
                    case "EditCommandTabSelectAddress":
                        _temp[1] = _str;

                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                        break;
                    case "EditCommandTabSelectLength":
                        _temp[2] = _str;
                        if (String.IsNullOrEmpty(_str))
                        {
                            VM.EditCommandTabSelectDataIdx = String.Empty;
                            VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], String.Empty);
                        }
                        else
                        {
                            _idx = Convert.ToInt32(_temp[2], 16) / 4;
                            _par = Convert.ToInt32(_temp[2], 16) % 4;
                            if (_par > 0) _idx++;
                            VM.EditCommandTabSelectDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", _temp[3].Length, _idx);
                            if (_temp[3].Length.Equals(_idx))
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);
                            else
                                VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], String.Empty);
                        }
                        
                        break;
                    case "EditCommandTabSelectData":
                        _temp[3] = _str;

                        VM.DefineSequence = String.Format(CultureInfo.CurrentCulture, "{0},{1},{2},{3}", _temp[0], _temp[1], _temp[2], _temp[3]);

                        _idx = Convert.ToInt32(_temp[2], 16) / 4;
                        _par = Convert.ToInt32(_temp[2], 16) % 4;
                        if (_par > 0) _idx++;
                        VM.EditCommandTabSelectDataIdx = String.Format(CultureInfo.CurrentCulture, "({0} / {1})", _temp[3].Length, _idx);
                        break;
                }

                //e.Handled = true;
            }

        TextBoxExit:;
            if ((ValidationStates)ValidationState[_tbox.Name] == ValidationStates.OK || _tbox.Name is "DefineSequence")
            {
                B02Item02Commands[Index].ApplyButton = true;
                ApplySequenceBtn.IsEnabled = true;
            }
            if ((ValidationStates)ValidationState[_tbox.Name] == ValidationStates.ERROR)
            {
                B02Item02Commands[Index].ApplyButton = false;
                ApplySequenceBtn.IsEnabled = false;
            }
            B02Item02Commands[Index].CommandTemp = VM.DefineSequence;
        }


        private void TextBoxKeyUpValidationAll(object sender, KeyEventArgs e)
        {
            Regex regex = new Regex(@"^[0-9a-zA-Z,]+$");
            TextBox _tbox = (TextBox)sender;
            String _str = _tbox.Text;

            if (regex.IsMatch(_str)) //e.Key.ToString()
            {
                if ((ValidationStates)ValidationState[_tbox.Name] != ValidationStates.OK)
                    _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                ValidationState[_tbox.Name] = ValidationStates.OK;

                if (String.IsNullOrEmpty(_str))
                {
                    _tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                    ValidationState[_tbox.Name] = ValidationStates.ERROR;
                    goto TextBoxExitAll;
                }
                else
                {
                    B02Item02Commands[Index].Type = true;
                    B02Item02Commands[Index].CommandState = CommandStatus.B02ITEM02CUSTOMIZE;
                    B02Item02Commands[Index].CommandTemp = _str;
                }
            }
            else
            {
                if (_str.Length > 0)
                {
                    _tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                    ValidationState[_tbox.Name] = ValidationStates.OK;
                }
                else
                {
                    _tbox.Style = (Style)FindResource("TextBoxDefaultDStyle");
                    ValidationState[_tbox.Name] = ValidationStates.DEFAULF;
                }
            }

        TextBoxExitAll:;
            //if ((ValidationStates)ValidationState[_tbox.Name] == ValidationStates.OK || _tbox.Name is "DefineSequence")
            if ((ValidationStates)ValidationState[_tbox.Name] == ValidationStates.OK)
            {
                ApplySequenceBtn.IsEnabled = true;
                B02Item02Commands[Index].ApplyButton = true;
                
            }
                
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxLostFocusValidation(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            tbox.Style = (Style)FindResource("TextBoxDefaultStyle");
            //ValidationState[tbox.Name] = ValidationStates.DEFAULF;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxLostFocusValidationAll(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            tbox.Style = (Style)FindResource("TextBoxDefaultDStyle");
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxGotFocusValidation(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"\A\b[0-9a-fA-F]+\b\Z");
            if (!regex.IsMatch(tbox.Text))
            {
                tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                ValidationState[tbox.Name] = ValidationStates.ERROR;
                tbox.UpdateLayout();
                Image errImg = (Image)tbox.Template.FindName("ErrorImage", tbox);

                e.Handled = true;
                goto GotFocus;
            }

            if (this.IsFocus)
            {
                this.IsFocus = false;
                tbox.Style = (Style)FindResource("TextBoxFocusStyle");
                ValidationState[tbox.Name] = ValidationStates.FOCUS;
            }
            else
            {
                switch (tbox.Name)
                {
                    case "EditCommandTabReadAddress":
                    case "EditCommandTabWriteAddress":
                    case "EditCommandTabUQRAddress":
                    case "EditCommandTabSelectAddress":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x3FFF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "EditCommandTabReadLength":
                    case "EditCommandTabWriteLength":
                    case "EditCommandTabUQRLength":
                    case "EditCommandTabSelectLength":
                        if (String.IsNullOrEmpty(tbox.Text) || Convert.ToInt32(tbox.Text, 16) > 0x60 || Convert.ToInt32(tbox.Text, 16) < 1)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "EditCommandTabWriteData":
                        if (String.IsNullOrEmpty(VM.EditCommandTabWriteLength)) { EditCommandTabWriteLength.Focus(); goto GotFocus; }

                        Int32 _nWordsLength = Convert.ToInt32(VM.EditCommandTabWriteLength, 16);
                        if (_nWordsLength * 4 != tbox.Text.Length)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }

                        break;
                    case "EditCommandTabAccessPassword":
                        if (String.IsNullOrEmpty(tbox.Text) || tbox.Text.Length < 8)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "EditCommandTabSelectData":
                        if (String.IsNullOrEmpty(VM.EditCommandTabSelectLength)) { EditCommandTabSelectLength.Focus(); goto GotFocus; }

                        Int32 _nBitsLength = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) / 4;
                        var par = Convert.ToInt32(VM.EditCommandTabSelectLength, 16) % 4;
                        if (par > 0) _nBitsLength++;
                        if (_nBitsLength != tbox.Text.Length)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                    case "EditCommandTabLockMask":
                    case "EditCommandTabLockAction":
                        if (String.IsNullOrEmpty(tbox.Text) || tbox.Text.Length != 3 || Convert.ToInt32(tbox.Text, 16) > 0x3FF)
                        {
                            tbox.Style = (Style)FindResource("TextBoxErrorStyle");
                            ValidationState[tbox.Name] = ValidationStates.ERROR;
                        }
                        else
                        {
                            tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                            ValidationState[tbox.Name] = ValidationStates.OK;
                        }
                        break;
                }
            }


        GotFocus:;
        }

        


        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBoxGotFocusValidationAll(object sender, RoutedEventArgs e)
        {
            TextBox tbox = (TextBox)sender;
            Regex regex = new Regex(@"\A\b[0-9a-zA-Z,]+\b\Z");
            if (!regex.IsMatch(tbox.Text))
            {
                tbox.Style = (Style)FindResource("TextBoxDefaultDStyle");
                ValidationState[tbox.Name] = ValidationStates.DEFAULF;
                tbox.UpdateLayout();
                e.Handled = true;
                goto GotFocusAll;
            }

            if (this.IsFocus)
            {
                this.IsFocus = false;
                tbox.Style = (Style)FindResource("TextBoxFocusStyle");
                ValidationState[tbox.Name] = ValidationStates.FOCUS;
            }
            else
            {
                tbox.Style = (Style)FindResource("TextBoxNormalStyle");
                ValidationState[tbox.Name] = ValidationStates.OK;
            }
        GotFocusAll:;
        }


        private void NoName()
        {
            if (String.IsNullOrEmpty(VM.EditCommandTabLockMask) || String.IsNullOrEmpty(VM.EditCommandTabLockAction))
                return;

            var mask = Format.HexStringToInt(VM.EditCommandTabLockMask);
            var action = Format.HexStringToInt(VM.EditCommandTabLockAction);
            for (Int32 i = 0; i < 10; i = i + 2)
            {
                var mask_ = (mask & (3 << i)) >> i;
                var action_ = (action & (3 << i)) >> i;

                switch (i)
                {
                    case 0:
                        EditCommandLockUser.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 2:
                        EditCommandLockTID.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 4:
                        EditCommandLockEPC.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 6:
                        EditCommandLockAccessPwd.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                    case 8:
                        EditCommandLockKillPwd.SelectedIndex = (mask_ > 0) ? (action_ & mask_) + 1 : 0;
                        break;
                }

            }
        }




        #endregion


    }
}
