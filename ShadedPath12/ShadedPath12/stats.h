// helper class for some statistics like frame timing 
class Stats
{
public:
	void startUpdate(GameTime &gameTime);
	void startDraw(GameTime &gameTime);
	void endUpdate(GameTime &gameTime);
	void endDraw(GameTime &gameTime);

	static const int numFramesGathered = 10 * 3;
	static const int frameNumStartGathering = 10 * 3;
	LONGLONG started[numFramesGathered] = { 0 };
	LONGLONG ended[numFramesGathered] = { 0 };
	Stats();
	virtual ~Stats();

	// measure single methods ans similar
	struct StatTopic {
		string topicName;
		long long cumulated;  // total time spent
		long long called = 0; // number of calls so far
		long long curStart;   // record current invocation time
	};
	unordered_map<string, StatTopic> statTopics;
	void start(string topic);
	void end(string topic);
	StatTopic *get(string name) { return &statTopics[name]; };
	long long getNow();
	wstring getInfo(string name);
	void init(XApp *xapp) { this->xapp = xapp; }
	XApp *xapp = nullptr;
};

