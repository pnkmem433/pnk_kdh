/*
 * AMS.Profile Class Library
 * 
 * Written by Alvaro Mendez
 * Copyright (c) 2005. All Rights Reserved.
 * 
 * The AMS.Profile namespace contains interfaces and classes that 
 * allow reading and writing of user-profile data.
 * This file contains the helper classes for the Xml-based Profile classes.
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
using System.Xml;
using System.Text;
using System.IO;
using System.Security;
using System.Resources;
using System.Reflection;
using System.Globalization;
using System.Net;
using System.Security.Permissions;

namespace RFID.Utility.IClass
{
	/// <summary>
	///   Abstract base class for all XML-based Profile classes. </summary>
	/// <remarks>
	///   This class provides common methods and properties for the XML-based Profile classes 
	///   (<see cref="XmlFormat" />, <see cref="Config" />). </remarks>
	public abstract class XmlBased : ProfileX, IDisposable
    {
		private Encoding		m_encoding = Encoding.UTF8;
		private ResourceManager stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());
		internal XmlBuffer		m_buffer;

		//protected XmlBased() { }
		
		protected XmlBased(String fileName) : base(fileName) { }

		protected XmlBased(XmlBased profile) : base(profile) {
			if (profile == null)
				throw new ArgumentNullException(stringManager.GetString("profile argument is null", CultureInfo.CurrentCulture));
			m_encoding = profile.Encoding;
		}
		
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
		protected XmlDocument GetXmlDocument() {
			if (m_buffer != null)
				return m_buffer.XmlDocument;

			VerifyName();
			if (!File.Exists(Name))
				return null;
            
            #pragma warning disable CA3075 // XML 中不安全的 DTD 處理
            XmlDocument doc = new XmlDocument();
            doc.Load(Name);
            #pragma warning restore CA3075 // XML 中不安全的 DTD 處理
            return doc;

            /*XmlDocument doc = new XmlDocument() { XmlResolver = null };
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                DtdProcessing = DtdProcessing.Parse
            };
            StringReader sreader = new StringReader(Name);
            XmlReader reader = null;
            try
            {
                reader = XmlReader.Create(sreader, settings);
                doc.Load(reader);
                return doc;
            }
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                }
            }*/
            

            /*XmlDocument doc = new XmlDocument() { XmlResolver = null };
			StringReader sreader = new StringReader(Name);
			XmlReader reader = null;
			try
			{
                reader = XmlReader.Create(sreader);
				doc.Load(reader);
				return doc;
			}
			finally
			{
				if (reader != null)
				{
					reader.Close();
				}
			}*/


        }

        protected void Save(XmlDocument doc) {
			if (m_buffer != null)
				m_buffer.m_needsFlushing = true;
			else
			{
				if (doc == null)
					throw new ArgumentNullException(stringManager.GetString("doc argument is null", CultureInfo.CurrentCulture));
				doc.Save(Name);
			}
				

		}

		public XmlBuffer Buffer(bool lockFile) {
			if (m_buffer == null)
				m_buffer = new XmlBuffer(this, lockFile);
			return m_buffer; 
		}

		public XmlBuffer Buffer() {
			return Buffer(true);
		}
		
		public bool Buffering {
			get {
				return m_buffer != null;
			}
		}
		
		public Encoding Encoding {
			get { 
				return m_encoding; 
			}
			set { 
				VerifyNotReadOnly();
				if (m_encoding == value)
					return;
						
				if (!OnRaiseChangeEvtCase(true, ProfileChangeType.Other, null, "Encoding", value))
					return;

				m_encoding = value; 				
				OnRaiseChangeEvtCase(false, ProfileChangeType.Other, null, "Encoding", value);				
			}
		}

        #region IDisposable Support
        private bool disposedValue = false;

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    if (m_buffer != null)
                        m_buffer.Close();
                    m_buffer = null;
                }

                disposedValue = true;
            }
        }


        public void Dispose()
        {

            Dispose(true);
            GC.SuppressFinalize(this);
        }
        #endregion
    }

	
	public class XmlBuffer : IDisposable
    {
		private XmlBased m_profile;
		private XmlDocument m_doc;
		private FileStream m_file;
		internal bool m_needsFlushing;
		private ResourceManager stringManager = new ResourceManager("en-US", Assembly.GetExecutingAssembly());

		internal XmlBuffer(XmlBased profile, bool lockFile) {
			m_profile = profile;

			if (lockFile) {
				m_profile.VerifyName();
				if (File.Exists(m_profile.Name))
					m_file = new FileStream(m_profile.Name, FileMode.Open, m_profile.ReadOnlyValue ? FileAccess.Read : FileAccess.ReadWrite, FileShare.Read);
			}
		}
		
		internal void Load(XmlTextWriter writer) {
			XmlReader reader = null;
			writer.Flush();
			writer.BaseStream.Position = 0;
			try
			{
				reader = XmlReader.Create(writer.BaseStream);
				m_doc.Load(reader);

				m_needsFlushing = true;
			}
			finally
			{
				if (reader != null)
				{
					reader.Close();
				}
			}
			
		}
		
		internal XmlDocument XmlDocument {
			get {
				if (m_doc == null) {
					XmlReader reader = null;
					try
					{
						m_doc = new XmlDocument() { XmlResolver = null };

						if (m_file != null)
						{
							m_file.Position = 0;
							reader = XmlReader.Create(m_file);
							m_doc.Load(reader);//m_file
						}
						else
						{
							m_profile.VerifyName();
							if (File.Exists(m_profile.Name))
							{
								StringReader sReader = new StringReader(m_profile.Name);
								reader = XmlReader.Create(sReader, new XmlReaderSettings() { XmlResolver = null });
								m_doc.Load(reader); //m_profile.Name
							}
								
						}
					}
					catch (ArgumentNullException)
					{
						throw;
					}
					finally
					{
						if (reader != null)
							reader.Close();
					}
					
				}
				return m_doc;
			}
		}

		internal bool IsEmpty {
			get {
				return String.IsNullOrEmpty(XmlDocument.InnerXml);
			}
		}
		
		public bool NeedsFlushing {
			get {
				return m_needsFlushing;
			}
		}
		
		public bool Locked {
			get {
				return m_file != null;
			}
		}
		
		public void Flush() {
			if (m_profile == null)
				throw new InvalidOperationException(stringManager.GetString("Cannot flush an XmlBuffer object that has been closed.", CultureInfo.CurrentCulture));

			if (m_doc == null)
				return;

			if (m_file == null)
				m_doc.Save(m_profile.Name);
			else {
				m_file.SetLength(0);
				m_doc.Save(m_file);
			}

			m_needsFlushing = false;
		}
		
		public void Reset() {
			if (m_profile == null)
				throw new InvalidOperationException(stringManager.GetString("Cannot reset an XmlBuffer object that has been closed.", CultureInfo.CurrentCulture));

			m_doc = null;
			m_needsFlushing = false;
		}
		
		public void Close() {
			if (m_profile == null)
				return;
				
			if (m_needsFlushing)
				Flush();

			m_doc = null;
		
			if (m_file != null)
			{
				m_file.Close();
				m_file = null;
			}

			if (m_profile != null)
				m_profile.m_buffer = null;
			m_profile = null;
		}

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                Close();
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
	}
}
