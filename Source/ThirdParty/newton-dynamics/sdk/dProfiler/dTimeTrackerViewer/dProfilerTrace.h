#pragma once


#include "dTimeTrackerRecord.h"

class dTimeTrackerViewer;

class dProfilerTrace
{
	enum dMouseState;
	class dDataBase;
	class dThreadTrace;
	class dTrackerSample;
	class dTrackerThread;

	class dTrackerString
	{
		public:
		dTrackerString(){m_string[0] = 0;}
		dTrackerString(const char* const string){ strcpy(m_string, string);	}
		dTrackerString(const dTrackerString& src){strcpy(m_string, src.m_string);}

		char m_string[128];
	};

	class dTraceCapture
	{
		public: 
		dTraceCapture ();
		~dTraceCapture();
		void Render(dTimeTrackerViewer* const viewer);

		void MouseMove();
		void DrawTimeLine();

		dArray<dTrackerThread*> m_treads;
		ImVec2 m_mouseBoxp0;
		ImVec2 m_mouseBoxp1;
		float m_minTime;
		float m_maxTime;
		float m_windowSize;
		float m_scale;
		float m_origin;
		float m_mouseOrigin;
		dMouseState m_mouseState;
	};

	public: 
	dProfilerTrace(FILE* const file);
	~dProfilerTrace();

	void Render (dTimeTrackerViewer* const viewer);

	private: 
	void ReadTrack(dDataBase& database);
	void ReadLabels(dDataBase& database);

	dTraceCapture m_rootNode;
	dArray<dTrackerString> m_nameList;
};

