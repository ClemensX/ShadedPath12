/*
 * Update
 */

class EffectAppData;

class Update
{
public:

	// configure data sets
	// MUST pass pointer to 2-dim array
	void init(EffectAppData *appData);

	// get inactive data set
	// only one thread can work on the inactive data set at one time, next caller will be blocked
	// while caller has this lock he should upload inactive data set to GPU
	EffectAppData* getLockedInactiveDataSet();

	// activate the inactive data set and make it active
	// this will block the caller until active data set is no longer used by anyone
	// also cleanup unused data sets (?)
	// activate()

	// get current active data set and increase use count
	// accessActiveDataSet()

	// release active data set and decrease use count
	// realeaseActiveDataSet()

private:
	//mutex dataSetMutex; // to synchronize updates to effect data (active/inactive)
	EffectAppData* effectAppData = nullptr;
	int currentInactiveAppDataSet = 0;
	int currentActiveAppDataSet = -1;

/*	EffectAppData* getInactiveAppDataSet()
	{
		return &effectAppData[currentInactiveAppDataSet];
	};
	EffectAppData* getActiveAppDataSet()
	{
		if (currentActiveAppDataSet < 0) {
			Error(L"active data set not available in Update. Cannot continue.");
		}
		return &effectAppData[currentActiveAppDataSet];
	}
*/};

