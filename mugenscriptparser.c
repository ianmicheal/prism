#include "tari/mugenscriptparser.h"

#include "tari/memoryhandler.h"
#include "tari/log.h"
#include "tari/system.h"

typedef struct {
	int(*mIsFunc)(MugenDefScriptGroup *);
	void(*mHandleFunc)(MugenDefScriptGroup *);
} ScriptParseElement;

static struct {
	int mIsActive;
	Vector mParseElements;
} gData;

static void clearMugenScriptParser() {
	delete_vector(&gData.mParseElements);
	gData.mIsActive = 0;
}

void resetMugenScriptParser()
{
	if (gData.mIsActive) {
		clearMugenScriptParser();
	}

	gData.mParseElements = new_vector();
	gData.mIsActive = 1;
}

void addMugenScriptParseFunction(int(*tIsFunc)(MugenDefScriptGroup *), void(*tHandleFunc)(MugenDefScriptGroup *))
{
	ScriptParseElement* e = allocMemory(sizeof(ScriptParseElement));
	e->mIsFunc = tIsFunc;
	e->mHandleFunc = tHandleFunc;
	vector_push_back_owned(&gData.mParseElements, e);
}

void parseMugenScript(MugenDefScript * tScript)
{
	MugenDefScriptGroup* current = tScript->mFirstGroup;
	while (current != NULL) {
		int hasFound = 0;
		int i;
		for (i = 0; i < vector_size(&gData.mParseElements); i++) {
			ScriptParseElement* e = vector_get(&gData.mParseElements, i);
			if (e->mIsFunc(current)) {
				e->mHandleFunc(current);
				hasFound = 1;
				break;
			}
		}

		if (!hasFound) {
			logError("Unable to find script group.");
			logErrorString(current->mName);
			abortSystem();
		}

		current = current->mNext;
	}

	clearMugenScriptParser();
}
