#include "Reader.h"
#include "RTETools.h"
#include "PresetMan.h"

namespace RTE {

	const std::string Reader::m_ClassName = "Reader";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Reader::Clear() {
		m_pStream = 0;
		m_FilePath.clear();
		m_CurrentLine = 1;
		m_StreamStack.clear();
		m_PreviousIndent = 0;
		m_IndentDifference = 0;
		m_ObjectEndings = 0;
		m_EndOfStreams = false;
		m_fpReportProgress = 0;
		m_ReportTabs = "\t";
		m_FileName.clear();
		m_DataModuleName.clear();
		m_DataModuleID = -1;
		m_OverwriteExisting = false;
		m_SkipIncludes = false;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int Reader::Create(const char *fileName, bool overwrites, ProgressCallback fpProgressCallback, bool failOK) {
		m_FilePath = fileName;

		if (m_FilePath.empty()) {
			return -1;
		}
		// Extract just the filename
		int lastSlashPos = m_FilePath.find_last_of('/');
		if (lastSlashPos == std::string::npos) { lastSlashPos = m_FilePath.find_last_of('\\'); }
		m_FileName = m_FilePath.substr(lastSlashPos + 1);

		// Find the first slash so we can get the module name
		int firstSlashPos = m_FilePath.find_first_of('/');
		if (firstSlashPos == std::string::npos) { firstSlashPos = m_FilePath.find_first_of('\\'); }

		m_DataModuleName = m_FilePath.substr(0, firstSlashPos);
		m_DataModuleID = g_PresetMan.GetModuleID(m_DataModuleName);

		m_pStream = new std::ifstream(fileName);
		if (!failOK) { RTEAssert(m_pStream->good(), "Failed to open data file \'" + std::string(fileName) + "\'!"); }

		m_OverwriteExisting = overwrites;

		// Report that we're starting a new file
		m_fpReportProgress = fpProgressCallback;
		if (m_fpReportProgress && m_pStream->good()) {
			char report[512];
			sprintf_s(report, sizeof(report), "\t%s on line %i", m_FileName.c_str(), m_CurrentLine);
			m_fpReportProgress(std::string(report), true);
		}
		return m_pStream->good() ? 0 : -1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Reader::Destroy(bool notInherited) {
		delete m_pStream;
		// Delete all the streams in the stream stack
		for (plf::list<StreamInfo>::iterator itr = m_StreamStack.begin(); itr != m_StreamStack.end(); ++itr) {
			delete (*itr).m_pStream;
		}
		Clear();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int Reader::GetReadModuleID() const {
		// If we have an invalid ID, try to get a valid one based on the name we do have
		return m_DataModuleID < 0 ? g_PresetMan.GetModuleID(m_DataModuleName) : m_DataModuleID;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Reader::ReadLine(char *locString, int size) {
		DiscardEmptySpace();

		char temp;
		char peek = m_pStream->peek();
		int i = 0;

		for (i = 0; i < size - 1 && peek != '\n' && peek != '\r' && peek != '\t'; ++i) {
			temp = m_pStream->get();
			// Check for line comment "//"
			if (peek == '/' && m_pStream->peek() == '/') {
				m_pStream->putback(temp);
				break;
			}
			if (m_pStream->eof()) {
				EndIncludeFile();
				break;
			}
			if (!m_pStream->good()) { ReportError("Stream failed for some reason"); }

			locString[i] = temp;
			peek = m_pStream->peek();
		}
		locString[i] = '\0';
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string Reader::ReadLine() {
		DiscardEmptySpace();

		std::string retString;
		char temp;
		char peek = m_pStream->peek();

		while (peek != '\n' && peek != '\r' && peek != '\t') {
			temp = m_pStream->get();

			// Check for line comment "//"
			if (peek == '/' && m_pStream->peek() == '/') {
				m_pStream->unget();
				break;
			}

			if (m_pStream->eof()) { break; }
			if (!m_pStream->good()) { ReportError("Stream failed for some reason"); }

			retString.append(1, temp);
			peek = m_pStream->peek();
		}
		return retString;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string Reader::ReadTo(char terminator, bool discardTerminator) {
		std::string retString;
		char temp;
		char peek = m_pStream->peek();

		while (peek != terminator) {
			temp = m_pStream->get();

			if (m_pStream->eof()) { break; }
			if (!m_pStream->good()) { ReportError("Stream failed for some reason"); }

			retString.append(1, temp);
			peek = m_pStream->peek();
		}
		// Discard the terminator if instructed to
		if (discardTerminator && peek == terminator) { m_pStream->get(); }
		return retString;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool Reader::NextProperty() {
		if (!DiscardEmptySpace() || m_EndOfStreams) {
			return false;
		}
		// If there are fewer tabs on the last line eaten this time,
		// that means there are no more properties to read on this object
		if (m_ObjectEndings < -m_IndentDifference) {
			m_ObjectEndings++;
			return false;
		}
		m_ObjectEndings = 0;
		return true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string Reader::ReadPropName() {
		DiscardEmptySpace();

		std::string retString;
		char temp;
		char peek;

		while (true) {
			peek = m_pStream->peek();
			if (peek == '=') {
				m_pStream->ignore(1);
				break;
			}
			if (peek == '\n' || peek == '\r' || peek == '\t') {
				ReportError("Property name wasn't followed by a value");
			}
			temp = m_pStream->get();
			if (m_pStream->eof()) {
				EndIncludeFile();
				break;
			}
			if (!m_pStream->good()) { ReportError("Stream failed for some reason"); }
			retString.append(1, temp);
		}
		// Trim the string of whitespace
		retString = TrimString(retString);
		
		// If the property name turns out to be the special IncludeFile,and we're not skipping include files then open that file and read the first property from it instead.
		if (retString == "IncludeFile") {
			if (m_SkipIncludes) {
				// Discard IncludeFile value
				std::string val = ReadPropValue();
				DiscardEmptySpace();
				retString = ReadPropName();
			} else {
				StartIncludeFile();
				// Return the first property name in the new file, this is to make the file inclusion seamless.
				// Alternatively, if StartIncludeFile failed, this will just grab the next prop name and ignore the failed IncludeFile property.
				retString = ReadPropName();
			}
		}
		return retString;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string Reader::ReadPropValue() {
		std::string fullLine = ReadLine();
		int begin = fullLine.find_first_of('=');
		std::string subStr = (begin == std::string::npos ? fullLine : fullLine.substr(begin + 1));
		return TrimString(subStr);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool Reader::DiscardEmptySpace() {
		char peek;
		int indent = 0;
		bool ateLine = false;
		char report[512];

		while (true) {
			peek = m_pStream->peek();

			// If we have hit the end and don't have any files to resume, then quit and indicate that
			if (m_pStream->eof()) {
				return EndIncludeFile();
			}
			// Not end-of-file but still got junk back... something went to shit
			if (peek == -1) { ReportError("Something went wrong reading the line; make sure it is providing the expected type"); }

			// Discard spaces
			if (peek == ' ') {
				m_pStream->ignore(1);
			// Discard tabs, and count them
			} else if (peek == '\t') {
				indent++;
				m_pStream->ignore(1);
			// Discard newlines and reset the tab count for the new line, also count the lines
			} else if (peek == '\n' || peek == '\r') {
				// So we don't count lines twice when there are both newline and carriage return at the end of lines
				if (peek == '\n') {
					m_CurrentLine++;
					// Only report every few lines
					if (m_fpReportProgress && (m_CurrentLine % 100 == 0)) {
						//char report[512];
						sprintf_s(report, sizeof(report), "%s%s reading line %i", m_ReportTabs.c_str(), m_FileName.c_str(), m_CurrentLine);
						m_fpReportProgress(std::string(report), false);
					}
				}
				indent = 0;
				ateLine = true;
				m_pStream->ignore(1);

			// Comment line?
			} else if (m_pStream->peek() == '/') {
				char temp = m_pStream->get();
				char temp2;

				// Confirm that it's a comment line, if so discard it and continue
				if (m_pStream->peek() == '/') {
					while (m_pStream->peek() != '\n' && m_pStream->peek() != '\r' && !m_pStream->eof()) { m_pStream->ignore(1); }
				// Block comment
				} else if (m_pStream->peek() == '*') {
					// Find the matching "*/"
					while (!((temp2 = m_pStream->get()) == '*' && m_pStream->peek() == '/') && !m_pStream->eof()) {
						// Count the lines within the comment though
						if (temp2 == '\n') { ++m_CurrentLine; }
					}
					// Discard that final '/'
					if (!m_pStream->eof()) { m_pStream->ignore(1); }

				// Not a comment, so it's data, so quit.
				} else {
					m_pStream->putback(temp);
					break;
				}
			} else { 
				break;
			}
		}

		// This precaution enables us to use DiscardEmptySpace repeatedly without messing up the indentation tracking logic
		if (ateLine) {
			// Get indentation difference from the last line of the last call to DiscardEmptySpace(), and the last line of this call to DiscardEmptySpace().
			m_IndentDifference = indent - m_PreviousIndent;
			// Save the last tab count
			m_PreviousIndent = indent;
		}
		return true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::string Reader::TrimString(std::string &stringToTrim) {
		if (stringToTrim.empty()) {
			return "";
		}
		int start = stringToTrim.find_first_not_of(' ');
		int end = stringToTrim.find_last_not_of(' ');

		if (start > end) {
			return "";
		} else if (start == 0 && end == stringToTrim.size() - 1) {
			return stringToTrim;
		}
		return stringToTrim.substr(start, (end - start) + 1);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Reader::ReportError(std::string errorDesc) {
		char error[1024];
		sprintf_s(error, sizeof(error), "%s Error happened in %s at line %i!", errorDesc.c_str(), m_FilePath.c_str(), m_CurrentLine);
		RTEAbort(error);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool Reader::StartIncludeFile() {
		// Report that we're including a file
		if (m_fpReportProgress) {
			char report[512];
			sprintf_s(report, sizeof(report), "%s%s on line %i includes:", m_ReportTabs.c_str(), m_FileName.c_str(), m_CurrentLine);
			m_fpReportProgress(std::string(report), false);
		}
		// Push the current stream onto the StreamStack for future retrieval when the new include file has run out of data.
		m_StreamStack.push_back(StreamInfo(m_pStream, m_FilePath, m_CurrentLine, m_PreviousIndent));

		// Get the file path from the stream
		m_FilePath = ReadPropValue();
		m_pStream = new std::ifstream(m_FilePath.c_str());
		if (m_pStream->fail()) {
			// Backpedal and set up to read the next property in the old stream
			delete m_pStream;
			m_pStream = m_StreamStack.back().m_pStream;
			m_FilePath = m_StreamStack.back().m_FilePath;
			m_CurrentLine = m_StreamStack.back().m_CurrentLine;
			m_PreviousIndent = m_StreamStack.back().m_PreviousIndent;
			m_StreamStack.pop_back();

			ReportError("Failed to open included data file");

			DiscardEmptySpace();
			return false;
		}

		// Line counting starts with 1, not 0
		m_CurrentLine = 1;
		// This is set to 0, because locally in the included file, all properties start at that count
		m_PreviousIndent = 0;

		// Extract just the filename
		int firstSlashPos = m_FilePath.find_first_of('/');
		if (firstSlashPos == std::string::npos) { firstSlashPos = m_FilePath.find_first_of('\\'); }
		m_FileName = m_FilePath.substr(firstSlashPos + 1);

		// Report that we're starting a new file
		if (m_fpReportProgress) {
			m_ReportTabs = "\t";
			for (int i = 0; i < m_StreamStack.size(); ++i) {
				m_ReportTabs.append("\t");
			}
			char report[512];
			sprintf_s(report, sizeof(report), "%s%s on line %i", m_ReportTabs.c_str(), m_FileName.c_str(), m_CurrentLine);
			m_fpReportProgress(std::string(report), true);
		}
		// Discard any fluff in the beginning of the new file
		DiscardEmptySpace();
		// indicate success
		return true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool Reader::EndIncludeFile() {
		// Do final report on the file we're closing
		if (m_fpReportProgress) {
			char report[512];
			sprintf_s(report, sizeof(report), "%s%s - done! %c", m_ReportTabs.c_str(), m_FileName.c_str(), -42);
			m_fpReportProgress(std::string(report), false);
		}
		if (m_StreamStack.empty()) {
			m_EndOfStreams = true;
			return false;
		}
		// Replace the current included stream with the parent one
		delete m_pStream;
		m_pStream = m_StreamStack.back().m_pStream;
		m_FilePath = m_StreamStack.back().m_FilePath;
		m_CurrentLine = m_StreamStack.back().m_CurrentLine;
		// Observe it's being added, not just replaced. This is to keep proper track when exiting out of a file
		m_PreviousIndent += m_StreamStack.back().m_PreviousIndent;
		m_StreamStack.pop_back();

		// Extract just the filename
		int firstSlashPos = m_FilePath.find_first_of('/');
		if (firstSlashPos == std::string::npos) { firstSlashPos = m_FilePath.find_first_of('\\'); }

		m_FileName = m_FilePath.substr(firstSlashPos + 1);

		// Report that we're going back a file
		if (m_fpReportProgress) {
			m_ReportTabs = "\t";
			for (int i = 0; i < m_StreamStack.size(); ++i) {
				m_ReportTabs.append("\t");
			}
			char report[512];
			sprintf_s(report, sizeof(report), "%s%s on line %i", m_ReportTabs.c_str(), m_FileName.c_str(), m_CurrentLine);
			m_fpReportProgress(std::string(report), true);
		}
		// Set up the resumed file for reading again
		DiscardEmptySpace();
		return true;
	}
}