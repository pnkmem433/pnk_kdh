/*
 * AMS.Profile Class Library
 * 
 * Written by Alvaro Mendez
 * Copyright (c) 2004. All Rights Reserved.
 * 
 * The AMS.Profile namespace contains interfaces and classes that 
 * allow reading and writing of user-profile data.
 * This file contains the Xml class.
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
 * Last Updated: Feb. 17, 2005
 */


using System;
using System.IO;
using System.Text;
using System.Xml;
using System.Collections;
using System.Reflection;
using log4net;
using System.Xml.XPath;
using System.Resources;
using System.Globalization;

namespace RFID.Utility.IClass
{

	public class XmlFormat : XmlBased {

		private String			m_rootName = "profile";
		private ResourceManager stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());

		public XmlFormat(String fileName) : base(fileName) { }	
		public XmlFormat(XmlFormat xml) : base(xml) {
			if (xml == null)
				throw new ArgumentNullException(stringManager.GetString("xml argument is null", CultureInfo.CurrentCulture));
			m_rootName = xml.m_rootName;
		}

		public ILog Logger{ get; set; }

		public override String DefaultName {
			get {
				return DefaultNameWithoutExtension() + ".xml";
			}
		}

		public override object Clone() {
			return new XmlFormat(this);
		}

		private String GetSectionsPath(String section) {
			return "section[@name=\"" + section + "\"]";
		}		                              

		private String GetEntryPath(String entry) {
			return "entry[@name=\"" + entry + "\"]";
		}
		
		public String RootName {
			get  { 
				return m_rootName; 
			}
			set  { 
				VerifyNotReadOnly();
				if (value == null)
					throw new ArgumentNullException(stringManager.GetString("value argument is null", CultureInfo.CurrentCulture));
				if (m_rootName == value.Trim())
					return;
					
				if (!OnRaiseChangeEvtCase(true, ProfileChangeType.Other, null, "RootName", value))
					return;

				m_rootName = value.Trim(); 				
				OnRaiseChangeEvtCase(false, ProfileChangeType.Other, null, "RootName", value);				
			}
		}
		
		public override void SetValue(String section, String entry, object value) {
			// If the value is null, remove the entry
			if (value == null) {
				RemoveEntry(section, entry);
				return;
			}
			
			VerifyNotReadOnly();
			VerifyName();
			VerifyAndAdjustSection(ref section);
			VerifyAndAdjustEntry(ref entry);

			if (!OnRaiseChangeEvtCase(true, ProfileChangeType.SetValue, section, entry, value))
				return;

            String valueString = value.ToString();

			// If the file does not exist, use the writer to quickly create it
			if ((m_buffer == null || m_buffer.IsEmpty) && !File.Exists(Name)) {	
				XmlTextWriter writer = null;
				
				// If there's a buffer, write to it without creating the file
				try {
					if (m_buffer == null)
						writer = new XmlTextWriter(Name, Encoding);
					else
						writer = new XmlTextWriter(new MemoryStream(), Encoding);

					writer.Formatting = Formatting.Indented;

					writer.WriteStartDocument();

					writer.WriteStartElement(m_rootName);
					writer.WriteStartElement("section");
					writer.WriteAttributeString("name", null, section);
					writer.WriteStartElement("entry");
					writer.WriteAttributeString("name", null, entry);
					writer.WriteString(valueString);
					writer.WriteEndElement();
					writer.WriteEndElement();
					writer.WriteEndElement();

					if (m_buffer != null)
						m_buffer.Load(writer);
					writer.Close();

					OnRaiseChangeEvtCase(false, ProfileChangeType.SetValue, section, entry, value);
					return;
				}
				catch (ArgumentException ex) {
					Logger.Info(ex.Message);
					return;
				}
			}
			
			// The file exists, edit it
			
			XmlDocument doc = GetXmlDocument();
			if (doc == null) return;
			XmlElement root = doc.DocumentElement;
			
			// Get the section element and add it if it's not there
			XmlNode sectionNode = root.SelectSingleNode(GetSectionsPath(section));
			if (sectionNode == null) {
				XmlElement element = doc.CreateElement("section");
				XmlAttribute attribute = doc.CreateAttribute("name");
				attribute.Value = section;
				element.Attributes.Append(attribute);			
				sectionNode = root.AppendChild(element);			
			}

			// Get the entry element and add it if it's not there
			XmlNode entryNode = sectionNode.SelectSingleNode(GetEntryPath(entry));
			if (entryNode == null) {
				XmlElement element = doc.CreateElement("entry");
				XmlAttribute attribute = doc.CreateAttribute("name");
				attribute.Value = entry;
				element.Attributes.Append(attribute);			
				entryNode = sectionNode.AppendChild(element);			
			}

			// Add the value and save the file
			entryNode.InnerText = valueString;
			Save(doc);		
			OnRaiseChangeEvtCase(false, ProfileChangeType.SetValue, section, entry, value);
		}
		
        /// <summary>
        /// 
        /// </summary>
        /// <param name="section"></param>
        /// <param name="entry"></param>
        /// <returns></returns>
		public override object GetValue(String section, String entry) {
			VerifyAndAdjustSection(ref section);
			VerifyAndAdjustEntry(ref entry);
			
			try { 	
				XmlDocument doc = GetXmlDocument();
				XmlElement root = doc.DocumentElement;
				
				XmlNode entryNode = root.SelectSingleNode(GetSectionsPath(section) + "/" + GetEntryPath(entry));
                if (entryNode == null)
                    return null;
                else
                    return entryNode.InnerText;
			}
			catch (XPathException) {	
				return null;
			}			
		}
		
        /// <summary>
        /// 
        /// </summary>
        /// <param name="section"></param>
        /// <param name="entry"></param>
		public override void RemoveEntry(String section, String entry) {
			VerifyNotReadOnly();
			VerifyAndAdjustSection(ref section);
			VerifyAndAdjustEntry(ref entry);

			// Verify the document exists
			XmlDocument doc = GetXmlDocument();
			if (doc == null)
				return;

			// Get the entry's node, if it exists
			XmlElement root = doc.DocumentElement;			
			XmlNode entryNode = root.SelectSingleNode(GetSectionsPath(section) + "/" + GetEntryPath(entry));
			if (entryNode == null)
				return;

			if (!OnRaiseChangeEvtCase(true, ProfileChangeType.RemoveEntry, section, entry, null))
				return;
			
			entryNode.ParentNode.RemoveChild(entryNode);			
			Save(doc);
			OnRaiseChangeEvtCase(false, ProfileChangeType.RemoveEntry, section, entry, null);
		}
		
        /// <summary>
        /// 
        /// </summary>
        /// <param name="section"></param>
		public override void RemoveSection(String section) {
			VerifyNotReadOnly();
			VerifyAndAdjustSection(ref section);

			// Verify the document exists
			XmlDocument doc = GetXmlDocument();
			if (doc == null)
				return;
			
			// Get the root node, if it exists
			XmlElement root = doc.DocumentElement;
			if (root == null)
				return;

			// Get the section's node, if it exists
			XmlNode sectionNode = root.SelectSingleNode(GetSectionsPath(section));
			if (sectionNode == null)
				return;
			
			if (!OnRaiseChangeEvtCase(true, ProfileChangeType.RemoveSection, section, null, null))
				return;

			root.RemoveChild(sectionNode);
			Save(doc);
			OnRaiseChangeEvtCase(false, ProfileChangeType.RemoveSection, section, null, null);
		}
		
		public override String[] GetEntryNames(String section) {

			if (!HasSection(section))
				return null;
			    			
			VerifyAndAdjustSection(ref section);
			
			XmlDocument doc = GetXmlDocument();
			XmlElement root = doc.DocumentElement;
			
			// Get the entry nodes
			XmlNodeList entryNodes = root.SelectNodes(GetSectionsPath(section) + "/entry[@name]");
			if (entryNodes == null)
				return null;

            // Add all entry names to the string array			
            String[] entries = new String[entryNodes.Count];
			int i = 0;

			foreach (XmlNode node in entryNodes)
				entries[i++] = node.Attributes["name"].Value;
			
			return entries;
		}
				
		public override String[] GetSectionNames() {
			// Verify the document exists
			XmlDocument doc = GetXmlDocument();
			if (doc == null)
				return null;

			// Get the root node, if it exists
			XmlElement root = doc.DocumentElement;
			if (root == null)
				return null;

			// Get the section nodes
			XmlNodeList sectionNodes = root.SelectNodes("section[@name]");
			if (sectionNodes == null)
				return null;

            // Add all section names to the string array			
            String[] sections = new String[sectionNodes.Count];			
			int i = 0;

			foreach (XmlNode node in sectionNodes)
				sections[i++] = node.Attributes["name"].Value;
			
			return sections;
		}		
	}
}
