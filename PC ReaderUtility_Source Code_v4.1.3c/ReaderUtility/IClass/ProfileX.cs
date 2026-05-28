/*
 * AMS.Profile Class Library
 * 
 * Written by Alvaro Mendez
 * Copyright (c) 2005. All Rights Reserved.
 * 
 * The AMS.Profile namespace contains interfaces and classes that 
 * allow reading and writing of user-profile data.
 * This file contains the Profile class.
 * 
 * The code is thoroughly documented, however, if you have any questions, 
 * feel free to email me at alvaromendez@consultant.com.  Also, if you 
 * decide to this in a commercial application I would appreciate an email 
 * message letting me know.
 *
 * This code may be used in compiled form in any way you desire. This
 * file may be redistributed unmodified by any means providing it is 
 * not sold for profit without the authors written consent, and 
 * providing that this notice and the authors name and all copyright 
 * notices remains intact. This file and the accompanying source code 
 * may not be hosted on a website or bulletin board without the author's 
 * written permission.
 * 
 * This file is provided "as is" with no expressed or implied warranty.
 * The author accepts no liability for any damage/loss of business that
 * this product may cause.
 *
 * Last Updated: Feb. 15, 2005
 */


using System;
using System.Data;
using System.Globalization;
using System.Reflection;
using System.Resources;

namespace RFID.Utility.IClass
{		
	/// <summary>
	///   Abstract base class for all Profile classes in this namespace. </summary>
	/// <remarks>
	///   This class contains fields and methods which are common for all the derived Profile classes. 
	///   It fully implements most of the methods and properties of its base interfaces so that 
	///   derived classes don't have to. </remarks>
	public abstract class ProfileX : IProfileX
	{
		// Fields
		private string m_name;
		private bool m_readOnly;	
		public event ProfileChangingEventHandler ChangingEventHandler;
		public event ProfileChangedEventHandler ChangedEventHandler;
		private ResourceManager stringManager;

		/*protected Profile() {			
			m_name = DefaultName;
		}*/
		protected ProfileX(string name) {
			stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());
			m_name = name;
		}	
		protected ProfileX(ProfileX profile) {
			stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());

			if (profile == null)
				throw new ArgumentNullException(stringManager.GetString("profile is null.", CultureInfo.CurrentCulture));
			m_name = profile.m_name;
			m_readOnly = profile.m_readOnly;			
			ChangingEventHandler = profile.ChangingEventHandler;
			ChangedEventHandler = profile.ChangedEventHandler;
		}
		
		
		public string Name {
			get { 
				return m_name; 
			}
			set {
				if (value == null)
					throw new ArgumentNullException(stringManager.GetString("Name parameter is null.", CultureInfo.CurrentCulture));
				VerifyNotReadOnly();	
				if (m_name == value.Trim())
					return;
					
				if (!OnRaiseChangeEvtCase(true, ProfileChangeType.Name, null, null, value))
					return;
							
				m_name = value.Trim();
				OnRaiseChangeEvtCase(false, ProfileChangeType.Name, null, null, value);
			}
		}
		
		public bool ReadOnlyValue {
			get { 
				return m_readOnly; 
			}
			
			set { 
				VerifyNotReadOnly();
				if (m_readOnly == value)
					return;
				
				if (!OnRaiseChangeEvtCase(true, ProfileChangeType.ReadOnly, null, null, value))
					return;
							
				m_readOnly = value;
				OnRaiseChangeEvtCase(false, ProfileChangeType.ReadOnly, null, null, value);
			}
		}
		
		public abstract string DefaultName {
			get;
		}
		
		public abstract object Clone();
	
		public abstract void SetValue(string section, string entry, object value);
			
		public abstract object GetValue(string section, string entry);
		
		public virtual string GetValue(string section, string entry, string defaultValue) {
			object value = GetValue(section, entry);
			return (value == null ? defaultValue : value.ToString());
		}
		
		public virtual int GetValue(string section, string entry, int defaultValue) {
			object value = GetValue(section, entry);
			if (value == null)
				return defaultValue;

			try {
				return Convert.ToInt32(value, CultureInfo.CurrentCulture);
			}
			catch (FormatException) {
				return 0;
			}
		}
	
		public virtual double GetValue(string section, string entry, double defaultValue) {
			object value = GetValue(section, entry);
			if (value == null)
				return defaultValue;

			try {
				return Convert.ToDouble(value, CultureInfo.CurrentCulture);
			}
			catch (FormatException)
			{
				return 0;
			}
		}

		public virtual bool GetValue(string section, string entry, bool defaultValue) {
			object value = GetValue(section, entry);
			if (value == null)
				return defaultValue;			

			try {
				return Convert.ToBoolean(value, CultureInfo.CurrentCulture);
			}
			catch(FormatException) {
				return false;
			}
		}
		
		public virtual bool HasEntry(string section, string entry) {
			string[] entries = GetEntryNames(section);
			
			if (entries == null)
				return false;

			VerifyAndAdjustEntry(ref entry);
			return Array.IndexOf(entries, entry) >= 0;
		}

		public virtual bool HasSection(string section) {
			string[] sections = GetSectionNames();

			if (sections == null)
				return false;

			VerifyAndAdjustSection(ref section);
			return Array.IndexOf(sections, section) >= 0;
		}
		
		public abstract void RemoveEntry(string section, string entry);
		
		public abstract void RemoveSection(string section);
				
		public abstract string[] GetEntryNames(string section);
		
		public abstract string[] GetSectionNames();
			
		public virtual IReadOnlyProfile CloneReadOnly() {
			ProfileX profile = (ProfileX)Clone();
			profile.m_readOnly = true;
			
			return profile;
		}
	
		public virtual DataSet GetDataSet() {
			VerifyName();
			
			string[] sections = GetSectionNames();
			if (sections == null)
				return null;
			
			DataSet ds = new DataSet(Name);
			
			// Add a table for each section
			foreach (string section in sections) {
				DataTable table = ds.Tables.Add(section);
				
				// Retrieve the column names and values
				string[] entries = GetEntryNames(section);
				DataColumn[] columns = new DataColumn[entries.Length];
				object[] values = new object[entries.Length];								

				int i = 0;
				foreach (string entry in entries)
				{
					object value = GetValue(section, entry);
				
					columns[i] = new DataColumn(entry, value.GetType());
					values[i++] = value;
				}
												
				// Add the columns and values to the table
				table.Columns.AddRange(columns);
				table.Rows.Add(values);								
			}
			
			return ds;
		}
			
		public virtual void SetDataSet(DataSet ds) {
			if (ds == null)
				throw new ArgumentNullException(stringManager.GetString("DataSet parameter is null.", CultureInfo.CurrentCulture));
			
			// Create a section for each table
			foreach (DataTable table in ds.Tables) {
				string section = table.TableName;
				DataRowCollection rows = table.Rows;				
				if (rows.Count == 0)
					continue;
			
				foreach (DataColumn column in table.Columns) {
					string entry = column.ColumnName;
					object value = rows[0][column];
					
					SetValue(section, entry, value);
				}
			}
		}

		protected static string DefaultNameWithoutExtension() {
			
			try {
				string file = AppDomain.CurrentDomain.SetupInformation.ConfigurationFile;
				return file.Substring(0, file.LastIndexOf('.'));
			}
			catch(ArgumentOutOfRangeException) {
				return "profile";  // if all else fails
			}
			
		}


		protected virtual void VerifyAndAdjustSection(ref string section) {
			if (section == null)
				throw new ArgumentNullException(stringManager.GetString("section", CultureInfo.CurrentCulture));			
			
			section = section.Trim();
		}

		protected virtual void VerifyAndAdjustEntry(ref string entry) {
			if (entry == null)
				throw new ArgumentNullException(stringManager.GetString("entry parameter is null.", CultureInfo.CurrentCulture));			

			entry = entry.Trim();
		}

		protected internal virtual void VerifyName() {
			if (String.IsNullOrEmpty(m_name))
				throw new InvalidOperationException(stringManager.GetString("Operation not allowed because Name property is null or empty.", CultureInfo.CurrentCulture));
		}

		protected internal virtual void VerifyNotReadOnly() {
			if (m_readOnly)
				throw new InvalidOperationException(stringManager.GetString("Operation not allowed because ReadOnly property is true.", CultureInfo.CurrentCulture));			
		}
		
		protected bool OnRaiseChangeEvtCase(bool changing, ProfileChangeType changeType, string section, string entry, object value) {
			if (changing) {
				// Don't even bother if there are no handlers.
				if (ChangingEventHandler == null)
					return true;

				ProfileChangingEventArgs e = new ProfileChangingEventArgs(changeType, section, entry, value);
				OnChanging(e);
				return !e.Cancel;
			}
			
			// Don't even bother if there are no handlers.
			if (ChangedEventHandler != null)
				OnChanged(new ProfileChangedEventArgs(changeType, section, entry, value));
			return true;
		}
		                          
		protected virtual void OnChanging(ProfileChangingEventArgs e) {
			if (ChangingEventHandler == null)
				return;

			foreach (ProfileChangingEventHandler handler in ChangingEventHandler.GetInvocationList()) {
				handler(this, e);
				
				// If a particular handler cancels the event, stop
				if (e.Cancel)
					break;
			}
		}

		protected virtual void OnChanged(ProfileChangedEventArgs e) {
            ChangedEventHandler?.Invoke(this, e);
        }
	
		public virtual void Test(bool cleanup) {
			String task = String.Empty; 
			try {
				string section = "Profile Test";
				
				task = "initializing the profile -- cleaning up the '" + section + "' section";
				
					RemoveSection(section);
				
				task = "getting the sections and their count";
				
					String[] sections = GetSectionNames();
					int sectionCount = (sections == null ? 0 : sections.Length);
					bool haveSections = sectionCount > 1;
				
				task = "adding some valid entries to the '" + section + "' section";
				
					SetValue(section, "Text entry", "123 abc"); 
					SetValue(section, "Blank entry", ""); 
					SetValue(section, "Null entry", null);  // nothing will be added
					SetValue(section, "  Entry with leading and trailing spaces  ", "The spaces should be trimmed from the entry"); 
					SetValue(section, "Integer entry", 2 * 8 + 1); 
					SetValue(section, "Long entry", 1234567890123456789); 
					SetValue(section, "Double entry", 2 * 8 + 1.95); 
					SetValue(section, "DateTime entry", DateTime.Today); 
					SetValue(section, "Boolean entry", haveSections); 
				
				task = "adding a null entry to the '" + section + "' section";

					try
					{
						SetValue(section, null, "123 abc"); 
						throw new Exception(stringManager.GetString("Passing a null entry was allowed for SetValue", CultureInfo.CurrentCulture));
					}
					catch (ArgumentNullException)
					{						
					}
						
				task = "retrieving a null section";

					try
					{
						GetValue(null, "Test"); 
						throw new Exception(stringManager.GetString("Passing a null section was allowed for GetValue", CultureInfo.CurrentCulture));
					}
					catch (ArgumentNullException)
					{						
					}

				task = "getting the number of entries and their count";
				
					Int32 expectedEntries = 8;
					string[] entries = GetEntryNames(section);

				task = "verifying the number of entries is " + expectedEntries;
				
					if (entries.Length != expectedEntries)
						throw new Exception("Incorrect number of entries found: " + entries.Length);

				task = "checking the values for the entries added";
								
					String strValue = GetValue(section, "Text entry", "");
					if (strValue != "123 abc")
						throw new Exception("Incorrect string value found for the Text entry: '" + strValue + "'");
						
					Int32 nValue = GetValue(section, "Text entry", 321);
					if (nValue != 0)
						throw new Exception("Incorrect integer value found for the Text entry: " + nValue);

					strValue = GetValue(section, "Blank entry", "invalid");
					if (!String.IsNullOrEmpty(strValue))
						throw new Exception("Incorrect string value found for the Blank entry: '" + strValue + "'");
				
					object value = GetValue(section, "Blank entry");
					if (value == null)
						throw new Exception(stringManager.GetString("Incorrect null value found for the Blank entry", CultureInfo.CurrentCulture));

					nValue = GetValue(section, "Blank entry", 321);
					if (nValue != 0)
						throw new Exception("Incorrect integer value found for the Blank entry: " + nValue);

					bool bValue = GetValue(section, "Blank entry", true);
					if (bValue != false)
						throw new Exception("Incorrect bool value found for the Blank entry: " + bValue);

					strValue = GetValue(section, "Null entry", "");
					if (!String.IsNullOrEmpty(strValue))
						throw new Exception("Incorrect string value found for the Null entry: '" + strValue + "'");
				
					value = GetValue(section, "Null entry");
					if (value != null)
						throw new Exception("Incorrect object value found for the Blank entry: '" + value + "'");

					strValue = GetValue(section, "  Entry with leading and trailing spaces  ", "");
					if (strValue != "The spaces should be trimmed from the entry")
						throw new Exception("Incorrect string value found for the Entry with leading and trailing spaces: '" + strValue + "'");

					if (!HasEntry(section, "Entry with leading and trailing spaces"))
						throw new Exception(stringManager.GetString("The Entry with leading and trailing spaces (trimmed) was not found", CultureInfo.CurrentCulture));

					nValue = GetValue(section, "Integer entry", 0);
					if (nValue != 17)
						throw new Exception("Incorrect integer value found for the Integer entry: " + nValue);
					
					double dValue = GetValue(section, "Integer entry", 0.0);
					if (dValue != 17)
						throw new Exception("Incorrect double value found for the Integer entry: " + dValue);

					long lValue = Convert.ToInt64(GetValue(section, "Long entry"), CultureInfo.CurrentCulture);
					if (lValue != 1234567890123456789)
						throw new Exception("Incorrect long value found for the Long entry: " + lValue);
					
					strValue = GetValue(section, "Long entry", String.Empty);
					if (strValue != "1234567890123456789")
						throw new Exception("Incorrect string value found for the Long entry: '" + strValue + "'");

					dValue = GetValue(section, "Double entry", 0.0);
					if (dValue != 17.95)
						throw new Exception("Incorrect double value found for the Double entry: " + dValue);

					nValue = GetValue(section, "Double entry", 321);
					if (nValue != 0)
						throw new Exception("Incorrect integer value found for the Double entry: " + nValue);
				
					strValue = GetValue(section, "DateTime entry", String.Empty);
					if (strValue != DateTime.Today.ToString(CultureInfo.CurrentCulture))
						throw new Exception("Incorrect string value found for the DateTime entry: '" + strValue + "'");

					DateTime today = DateTime.Parse(strValue, CultureInfo.CurrentCulture);
					if (today != DateTime.Today)
						throw new Exception("The DateTime value is not today's date: '" + strValue + "'");
				
					bValue = GetValue(section, "Boolean entry", !haveSections);
					if (bValue != haveSections)
						throw new Exception("Incorrect bool value found for the Boolean entry: " + bValue);
					
					strValue = GetValue(section, "Boolean entry", String.Empty);
					if (strValue != haveSections.ToString(CultureInfo.CurrentCulture))
						throw new Exception("Incorrect string value found for the Boolean entry: '" + strValue + "'");

					value = GetValue(section, "Nonexistent entry");
					if (value != null)
						throw new Exception("Incorrect value found for the Nonexistent entry: '" + value + "'");

					strValue = GetValue(section, "Nonexistent entry", "Some Default");
					if (strValue != "Some Default")
						throw new Exception("Incorrect default value found for the Nonexistent entry: '" + strValue + "'");

				task = "creating a ReadOnly clone of the object";
				
					IReadOnlyProfile roProfile = CloneReadOnly();
					
					if (!roProfile.HasSection(section))
						throw new Exception(stringManager.GetString("The section is missing from the cloned read-only profile", CultureInfo.CurrentCulture));

					dValue = roProfile.GetValue(section, "Double entry", 0.0);
					if (dValue != 17.95)
						throw new Exception("Incorrect double value in the cloned object: " + dValue);
				
				task = "checking if ReadOnly clone can be hacked to allow writing";

					try
					{
						((IProfileX)roProfile).ReadOnlyValue = false;
						throw new Exception(stringManager.GetString("Changing of the ReadOnly flag was allowed on the cloned read-only profile", CultureInfo.CurrentCulture));
					}
					catch (InvalidOperationException)
					{						
					}

					try
					{
						// Test if a read-only profile can be hacked by casting
						((IProfileX)roProfile).SetValue(section, "Entry which should not be written", "This should not happen");
						throw new Exception(stringManager.GetString("SetValue did not throw an InvalidOperationException when writing to the cloned read-only profile", CultureInfo.CurrentCulture));
					}
					catch (InvalidOperationException)
					{						
					}
						
			//	task = "checking the DataSet methods";

				//	DataSet ds = GetDataSet();
				//	Profile copy = (Profile)Clone();
				//	copy.Name = Name + "2";
				//	copy.SetDataSet(ds);					
					       
				if (!cleanup)
					return;
					
				task = "deleting the entries just added";

					RemoveEntry(section, "Text entry"); 
					RemoveEntry(section, "Blank entry"); 
					RemoveEntry(section, "  Entry with leading and trailing spaces  "); 
					RemoveEntry(section, "Integer entry"); 
					RemoveEntry(section, "Long entry"); 
					RemoveEntry(section, "Double entry"); 
					RemoveEntry(section, "DateTime entry"); 
					RemoveEntry(section, "Boolean entry"); 													

				task = "deleting a nonexistent entry";

					RemoveEntry(section, "Null entry"); 

				task = "verifying all entries were deleted";

					entries = GetEntryNames(section);
				
					if (entries.Length != 0)
						throw new Exception("Incorrect number of entries still found: " + entries.Length);

				task = "deleting the section";

					RemoveSection(section);

				task = "verifying the section was deleted";

					int sectionCount2 = GetSectionNames().Length;
				
					if (sectionCount != sectionCount2)
						throw new Exception("Incorrect number of sections found after deleting: " + sectionCount2);

					entries = GetEntryNames(section);				
				
					if (entries != null)
						throw new Exception(stringManager.GetString("The section was apparently not deleted since GetEntryNames did not return null", CultureInfo.CurrentCulture));
			}
			catch (Exception ex)
			{
				throw new Exception("Test Failed while " + task, ex);
			}
		}
	}	
}
