#include "stdafx.h"
#include "dProfilerTrace.h"
#include "dTimeTrackerViewer.h"


enum dProfilerTrace::dMouseState
{
	m_leftDown,
	m_undefinded,
};

class dProfilerTrace::dThreadTrace : public dArray<dTimeTrackerRecord>
{
	public:
	dThreadTrace()
		:dArray<dTimeTrackerRecord>()
		,m_index(0)
	{
	}

	~dThreadTrace()
	{
	}

	void AddTrace(const void* const inputBuffer, int dataSize)
	{
		dThreadTrace& me = *this;

		me[me.GetSize() + (1 << DG_TIME_TRACKER_ENTRIES_POWER) - 1].m_start = 0;

		dTimeTrackerRecord* const buffer = &me[me.GetSize()];

		//uLongf destLen;
		//int compressError = uncompress((Bytef*)buffer, &destLen, compressedData, compressesDataSize);
		//dAssert(compressError == Z_OK);
		memcpy (buffer, inputBuffer, dataSize);
		m_count += 1 << DG_TIME_TRACKER_ENTRIES_POWER;
	}

	dTrackerSample* GetNextSample();

	int m_index;
};

class dProfilerTrace::dTrackerSample
{
	public:
	dTrackerSample(unsigned name, unsigned start, unsigned duration)
		:m_name(name)
		,m_start(start)
		,m_duration(duration)
		,m_children(dArray<dTrackerSample*>())
	{
	}

	~dTrackerSample()
	{
		for (int i = 0; i < m_children.GetSize(); i++) {
			delete m_children[i];
		}
	}

	const void Render(dTimeTrackerViewer* const viewer, float y0) const
	{
		dProfilerTrace& root = *viewer->GetTrace();

		// calculate view area rectangle
		const dTraceCapture& capture = viewer->GetTrace()->m_rootNode;

		ImVec2 text_size = ImGui::CalcTextSize("");

		float textPadd = 2.0f;
		float textWitdh = text_size.y;

		float grouping = 4.0f;
		float origin = capture.m_origin;
		float p0 = capture.m_minTime;
		float width = capture.m_windowSize;
		float scale = capture.m_windowSize * capture.m_scale / (capture.m_maxTime - capture.m_minTime);

		ImVec2 box0(0.0f, y0);
		ImVec2 box1(0.0f, y0 + textWitdh + textPadd * 2.0f - 1.0f);

		ImU32 rectColor = 0xff00c000;
		ImU32 groupColor = 0xffa00000;
		ImU32 textColor = 0xff000000;
		ImU32 borderColor = 0xffffffff;

		ImDrawList* const draw = ImGui::GetWindowDrawList();

		float x0 = origin + scale * (m_start - p0);
		float x1 = origin + scale * (m_start + m_duration - p0);

		ImU32 color = rectColor;

		box0.x = x0;
		box1.x = x1;
		draw->AddRectFilled(box0, box1, color);
		draw->AddRect(box0, box1, borderColor);
		
		char functionName[256];
		sprintf(functionName, "%s %d", root.m_nameList[m_name].m_string, m_duration);
		text_size = ImGui::CalcTextSize(functionName);
		int t1 = strlen(functionName);
		const char* functionNameEnd = functionName + t1;
		if (text_size.x > (x1 - x0)) {
			int t0 = 0;
			while ((t1 - t0) >= 5) {
				int t = (t0 + t1) / 2;
				text_size = ImGui::CalcTextSize(functionName, functionName + t);
				if (text_size.x > (x1 - x0)) {
					t1 = t;
				} else {
					t0 = t;
				}
			}
			functionNameEnd = functionName + t0;
			text_size = ImGui::CalcTextSize(functionName, functionName + t0);
		}

		if (text_size.x <= (x1 - x0)) {
			ImVec2 textPost(box0);
			textPost.y += textPadd;
			textPost.x += (x1 - x0 - text_size.x) * 0.5f;
			draw->AddText(textPost, textColor, functionName, functionNameEnd);
		}

		for (int i = 0; i < m_children.GetSize(); i++) {
			m_children[i]->Render(viewer, box1.y + 1.0f);
		}
	}

	unsigned m_name;
	unsigned m_start;
	unsigned m_duration;
	dArray<dTrackerSample*> m_children;
};

dProfilerTrace::dTrackerSample* dProfilerTrace::dThreadTrace::GetNextSample()
{
	dTrackerSample* sample = NULL;
	if (m_index < GetSize()) {
		dThreadTrace& me = *this;
		const dTimeTrackerRecord& record = me[m_index];
		sample = new dTrackerSample(record.m_nameHash, record.m_start, record.m_duration);
	}
	m_index++;
	return sample;
}


class dProfilerTrace::dTrackerThread
{
	public:
	dTrackerThread(unsigned threadName, dThreadTrace& track)
		:m_frames()
		,m_name(threadName)
		,m_levels_deep(1)
		,m_isOpen(true)
	{
		int index = 0;

		dTrackerSample* currentSample = track.GetNextSample();
		m_frames.Push(currentSample);
		while (dTrackerSample* nextSample = track.GetNextSample()) {
			unsigned maxX = currentSample->m_start + currentSample->m_duration;
			unsigned minX = nextSample->m_start;
			if (minX >= maxX) {
				m_frames.Push(nextSample);
				currentSample = nextSample;
			} else {
				int stackDepth = 1;
				InsertChild (currentSample, nextSample, stackDepth);
				m_levels_deep = dMax (stackDepth, m_levels_deep);
			} 
		}
	}

	~dTrackerThread()
	{
		for (int i = 0; i < m_frames.GetSize(); i++) {
			delete m_frames[i];
		}
	}

	void InsertChild (dTrackerSample* root, dTrackerSample* const child, int& stackDepth)
	{
		unsigned childX0 = child->m_start;
		unsigned childX1 = child->m_start + child->m_duration;
		dAssert ((childX0 >= root->m_start) && (childX1 <= root->m_start + root->m_duration));

		if (root->m_children.GetSize()) {
			dTrackerSample* const lastSibling = root->m_children[root->m_children.GetSize() - 1];
			unsigned siblingX0 = lastSibling->m_start;
			unsigned siblingX1 = lastSibling->m_start + lastSibling->m_duration;
			if (siblingX1 <= childX0) {
				root->m_children.Push(child);
			} else {
				for (int i = root->m_children.GetSize() - 1; i >= 0 ; i--) {
					dTrackerSample* const sibling = root->m_children[i];
					unsigned siblingX0 = sibling->m_start;
					unsigned siblingX1 = sibling->m_start + sibling->m_duration;
					if ((childX0 >= siblingX0) && (childX1 <= siblingX1)) {
						stackDepth++;
						InsertChild(sibling, child, stackDepth);
						break;
					}
				}
			}
		} else {
			root->m_children.Push(child);
		}
	}

	void dProfilerTrace::dTrackerThread::Render(dTimeTrackerViewer* const viewer)
	{
		dProfilerTrace& root = *viewer->GetTrace();
		const char* const threadName = root.m_nameList[m_name].m_string;

		if (ImGui::CollapsingHeader(threadName, &m_isOpen)) {

			// calculate view area rectangle
			ImVec2 cursorPosit0(ImGui::GetCursorScreenPos());
			for (int i = 0; i < m_levels_deep + 2; i++) {
				ImGui::Text("");
			}
			ImVec2 cursorPosit1(ImGui::GetCursorScreenPos());
			cursorPosit1.x += ImGui::GetWindowSize().x;

			if (ImGui::IsMouseHoveringRect(cursorPosit0, cursorPosit1, false)) {
				// if mouse is over this area save the the rectangle
				root.m_rootNode.m_mouseBoxp0 = cursorPosit0;
				root.m_rootNode.m_mouseBoxp1 = cursorPosit1;
			}

			const dTraceCapture& capture = viewer->GetTrace()->m_rootNode; 

			ImVec2 text_size = ImGui::CalcTextSize("");

			float textPadd = 2.0f;
			float textWitdh = text_size.y;

			float grouping = 4.0f;
			float origin = capture.m_origin;
			float p0 = capture.m_minTime;
			float width = capture.m_windowSize;
			float scale = capture.m_windowSize * capture.m_scale / (capture.m_maxTime - capture.m_minTime);

			ImVec2 box0(0.0f, cursorPosit0.y);
			ImVec2 box1(0.0f, cursorPosit0.y + textWitdh + textPadd * 2.0f - 1.0f);
			
			ImU32 rectColor = 0xff00c000;
			ImU32 groupColor = 0xff00c0a0;
			ImU32 textColor = 0xff000000;
			ImU32 borderColor = 0xffffffff;

			ImDrawList* const draw = ImGui::GetWindowDrawList();

			for (int i = 0; i < m_frames.GetSize(); i ++) {
				const dTrackerSample* const sample0 = m_frames[i];

				float x0 = origin + scale * (sample0->m_start - p0);

				if (x0 >= width) {
					break;
				}

				float x1 = origin + scale * (sample0->m_start + sample0->m_duration - p0);

				if (x1 >= 0.0f) {
					ImU32 color = rectColor;
					while (((x1 - x0) < grouping) && (i < m_frames.GetSize() - 1)) {
						const dTrackerSample* const sample1 = m_frames[i + 1];
						float z0 = origin + scale * (sample1->m_start - p0);
						float z1 = origin + scale * (sample1->m_start + sample1->m_duration - p0);
						color = groupColor;
						if ((z1 - z0) >= grouping) {
							break;
						}
						x1 = z1;
						i ++;
					} 

					box0.x = x0;
					box1.x = x1;
					draw->AddRectFilled(box0, box1, color);
					draw->AddRect(box0, box1, borderColor);

					if (sample0 == m_frames[i]) {
						char functionName[256];
						sprintf (functionName, "%s %d", root.m_nameList[sample0->m_name].m_string, sample0->m_duration);
						ImVec2 text_size = ImGui::CalcTextSize(functionName);
						int t1 = strlen (functionName);
						const char* functionNameEnd = functionName + t1;
						if (text_size.x > (x1 - x0)) {
							int t0 = 0;
							while ((t1 - t0) >= 5) {
								int t = (t0 + t1) / 2;
								text_size = ImGui::CalcTextSize(functionName, functionName + t);
								if (text_size.x > (x1 - x0)) {
									t1 = t;
								} else {
									t0 = t;
								}
							}
							functionNameEnd = functionName + t0;
							text_size = ImGui::CalcTextSize(functionName, functionName + t0);
						}

						if (text_size.x <= (x1 - x0)) {
							ImVec2 textPost (box0);
							textPost.y += textPadd;
							textPost.x += (x1 - x0 - text_size.x) * 0.5f;
							draw->AddText(textPost, textColor, functionName, functionNameEnd);
						}

						for (int j = 0; j < sample0->m_children.GetSize(); j ++) {
							sample0->m_children[j]->Render(viewer, box1.y + 1.0f);
						}
					}
				}
			}
		}
	}

	dArray<dTrackerSample*> m_frames;
	int m_name;
	int m_levels_deep;
	bool m_isOpen;
};

class dProfilerTrace::dDataBase
{
	public: 
	dDataBase(FILE* const file)
		:m_file(file)
		,m_trace()
		,m_dictionary()
	{
	}

	public: ~dDataBase()
	{
	}

	int ReadInt()
	{
		int code;
		fread (&code, sizeof (int), 1, m_file);
		return code;
	}

	dTrackerChunkType ReadChunkType()
	{
		return dTrackerChunkType(ReadInt());
	}

	int ReadName(char* const name)
	{
		unsigned size = ReadInt();
		fread(name, size, 1, m_file);
		name[size] = 0;
		return size;
	}

	void ReadCompressedTrack(char* const buffer, int size)
	{
		fread (buffer, size, 1, m_file);
	}

	FILE* m_file;
	dMap<dThreadTrace, unsigned> m_trace;	
	dMap<dTrackerString, unsigned> m_dictionary;
};

dProfilerTrace::dProfilerTrace(FILE* const file)
	:m_rootNode()
	,m_nameList()
{
	dDataBase database(file); 

	for (dTrackerChunkType chunkType = database.ReadChunkType(); chunkType != m_traceEnd; chunkType = database.ReadChunkType()) {

		switch (chunkType) 
		{
			case m_traceSamples:
			{
				ReadTrack(database);
				break;
			}

			case m_traceLabel:
			{
				ReadLabels(database);
				break;
			}

			default:
				dAssert(0);
				break;
		}
	}

	dMap<int, unsigned> nameMap;
	dMap<dTrackerString, unsigned>::Iterator iter (database.m_dictionary);
	for (iter.Begin(); iter; iter ++) {
		dTrackerString name (iter.GetNode()->GetInfo());
		unsigned key = iter.GetKey();
		nameMap.Insert(m_nameList.GetSize(), key);
		_strrev (name.m_string);
		char* const ptr = strstr (name.m_string, "::");
		if (ptr) {
			*ptr = 0;
		}
		_strrev (name.m_string);
		m_nameList.Push (name);
	}

	dMap<dThreadTrace, unsigned>::Iterator traceIter (database.m_trace);
	unsigned maxTime = 0;
	unsigned minTime = -1;
	for (traceIter.Begin(); traceIter; traceIter ++) {
		dThreadTrace& track = traceIter.GetNode()->GetInfo();
		const int threadsCount = track.GetSize();
		for (int i = 0; i < threadsCount; i ++) {
			int remapHashIndex = nameMap.Find(track[i].m_nameHash)->GetInfo();
			track[i].m_nameHash = remapHashIndex;
		}
		int treadHashIndex = nameMap.Find(traceIter.GetKey())->GetInfo();

		dTrackerThread* const thread = new dTrackerThread(treadHashIndex, track);
		m_rootNode.m_treads.Push(thread);

		const dArray<dTrackerSample*>& frames = thread->m_frames;
		minTime = dMin(minTime, frames[0]->m_start);
		maxTime = dMax(maxTime, frames[frames.GetSize() - 1]->m_start);
	}
	m_rootNode.m_minTime = float (minTime);
	m_rootNode.m_maxTime = float (maxTime);

	m_rootNode.m_scale = 300.0f;
	//m_rootNode.m_scale = 1.0f;
	m_rootNode.m_origin = m_rootNode.m_minTime;

	for (int i = 1; i < m_rootNode.m_treads.GetSize(); i ++) {
		dTrackerThread* const tmp = m_rootNode.m_treads[i];
		const dTrackerString& name0 = m_nameList[tmp->m_name];
		int index = i;
		for (; index > 0; index --) {
			dTrackerThread* const tread = m_rootNode.m_treads[index - 1];
			const dTrackerString& name1 = m_nameList[tread->m_name];
			int test = strcmp (name0.m_string, name1.m_string);
			if (test <= 0) {
				break;
			}
			m_rootNode.m_treads[index] = tread;
		}
		m_rootNode.m_treads[index] = tmp;
	}
}

dProfilerTrace::~dProfilerTrace()
{
}

void dProfilerTrace::ReadTrack(dDataBase& database)
{
	unsigned nameCRC = database.ReadInt();
	unsigned compressesDataSize = database.ReadInt();

	char* compressedData = dAlloca (char, compressesDataSize + 1024);
	database.ReadCompressedTrack(compressedData, compressesDataSize);

	dMap<dThreadTrace, unsigned>::dTreeNode* threadNode = database.m_trace.Find(nameCRC);
	if (!threadNode) {
		threadNode = database.m_trace.Insert(nameCRC);
	}
	dThreadTrace& track = threadNode->GetInfo();
	track.AddTrace(compressedData, compressesDataSize);
}

void dProfilerTrace::ReadLabels(dDataBase& database)
{
	for (dTrackerChunkType chunkType = database.ReadChunkType(); chunkType == m_traceLabel; chunkType = database.ReadChunkType()) {
		char name[1024];
		unsigned key = database.ReadInt();
		unsigned size = database.ReadName(name);
		database.m_dictionary.Insert(name, key);
	}
}

void dProfilerTrace::Render (dTimeTrackerViewer* const viewer)
{
	m_rootNode.Render(viewer);
/*
	if (ImGui::CollapsingHeader("Graphs widgets")) {
		static bool animate = true;
		ImGui::Checkbox("Animate", &animate);

		static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
		ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));

		// Create a dummy array of contiguous float values to plot
		// Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float and the sizeof() of your structure in the Stride parameter.
		static float values[90] = { 0 };
		static int values_offset = 0;
		if (animate) {
			static float refresh_time = ImGui::GetTime(); // Create dummy data at fixed 60 hz rate for the demo
			for (; ImGui::GetTime() > refresh_time + 1.0f / 60.0f; refresh_time += 1.0f / 60.0f) {
				static float phase = 0.0f;
				values[values_offset] = cosf(phase);
				values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
				phase += 0.10f*values_offset;
			}
		}
		ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0, 80));
		ImGui::PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80));

		// Use functions to generate output
		// FIXME: This is rather awkward because current plot API only pass in indices. We probably want an API passing floats and user provide sample0 rate/count.
		struct Funcs
		{
			static float Sin(void*, int i) { return sinf(i * 0.1f); }
			static float Saw(void*, int i) { return (i & 1) ? 1.0f : 0.0f; }
		};
		static int func_type = 0, display_count = 70;
		ImGui::Separator();
		ImGui::PushItemWidth(100); ImGui::Combo("func", &func_type, "Sin\0Saw\0"); ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::SliderInt("Sample count", &display_count, 1, 400);
		float(*func)(void*, int) = (func_type == 0) ? Funcs::Sin : Funcs::Saw;
		ImGui::PlotLines("Lines", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
		ImGui::PlotHistogram("Histogram", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
		ImGui::Separator();

		// Animate a simple progress bar
		static float progress = 0.0f, progress_dir = 1.0f;
		if (animate) {
			progress += progress_dir * 0.4f * ImGui::GetIO().DeltaTime;
			if (progress >= +1.1f) { progress = +1.1f; progress_dir *= -1.0f; }
			if (progress <= -0.1f) { progress = -0.1f; progress_dir *= -1.0f; }
		}

		// Typically we would use ImVec2(-1.0f,0.0f) to use all available width, or ImVec2(width,0.0f) for a specified width. ImVec2(0.0f,0.0f) uses ItemWidth.
		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::Text("Progress Bar");

		float progress_saturated = (progress < 0.0f) ? 0.0f : (progress > 1.0f) ? 1.0f : progress;
		char buf[32];
		sprintf(buf, "%d/%d", (int)(progress_saturated * 1753), 1753);
		ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), buf);
	}
*/
}


dProfilerTrace::dTraceCapture::dTraceCapture()
	:m_treads(dArray<dTrackerThread*>())
	,m_minTime(0.0f)
	,m_maxTime(0.0f)
	,m_windowSize(0.0f)
	,m_scale(1.0f)
	,m_origin(0.0f)
	,m_mouseOrigin(0.0f)
	,m_mouseState(m_undefinded)
{
}

dProfilerTrace::dTraceCapture::~dTraceCapture()
{
	for (int i = 0; i < m_treads.GetSize(); i++) {
		delete m_treads[i];
	}
}

void dProfilerTrace::dTraceCapture::DrawTimeLine()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("time line");
/*
	ImDrawList* const draw = ImGui::GetWindowDrawList();
	ImVec2 p0(ImGui::GetCursorScreenPos());
	ImVec2 size(ImGui::GetWindowSize());
	ImVec2 p1(p0);
	p1.x += size.x - 40.0f;
	draw->AddLine(p0, p1, IM_COL32_A_MASK + 0x8040);
	m_timeLineP0 = p0.x;
	m_timeLineP1 = p1.x;

	ImVec2 q0(p0);
	q0.x = m_timeLinePosition - m_timeWidth * 0.5f;
	q0.y -= 5.0f;

	ImVec2 q1(q0);
	q1.x += m_timeWidth;
	q1.y += 10.0f;

	draw->AddRectFilled(q0, q1, IM_COL32_A_MASK + 0x8040);
*/
	ImGui::Text("");
}

void dProfilerTrace::dTraceCapture::MouseMove()
{

	switch (m_mouseState)
	{
		case m_leftDown:
		{
			if (ImGui::IsMouseDown(0)) {
				ImVec2 mousePos(ImGui::GetMousePos());
				m_origin = mousePos.x - m_mouseOrigin;
			} else {
				m_mouseState = m_undefinded;
			}
			break;
		}

		default:
		{
		   if (ImGui::IsMouseDown(0) && ImGui::IsMouseHoveringRect(m_mouseBoxp0, m_mouseBoxp1, false)) {
			   ImVec2 mousePos(ImGui::GetMousePos());
			   //float p0 = m_minTime;
			   //float origin = m_origin;
			   //float width = m_windowSize;
			   //float scale = m_windowSize * m_scale / (m_maxTime - m_minTime);
			   //float x0 = origin + scale * (m_start - p0);
			   //float x1 = origin + scale * (m_start + m_duration - p0);
			   m_mouseOrigin = mousePos.x - m_origin;
		   
			   m_mouseState = m_leftDown;
		   }
		}
	}
}


void dProfilerTrace::dTraceCapture::Render(dTimeTrackerViewer* const viewer)
{
	ImVec2 size(ImGui::GetWindowSize());
	m_windowSize = size.x;

	// display time line
	DrawTimeLine();

	// render all traces
	for (int i = 0; i < m_treads.GetSize(); i++) {
		m_treads[i]->Render(viewer);
	}

	// update mouse motion
	MouseMove();
}
