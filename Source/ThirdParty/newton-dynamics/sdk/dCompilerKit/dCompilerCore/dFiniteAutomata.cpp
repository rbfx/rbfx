#include "dFiniteAutomata.h"
#include "dAutomataState.h"


dFiniteAutomata::dFiniteAutomata()
{
}

dFiniteAutomata::~dFiniteAutomata()
{
}

dAutomataState* dFiniteAutomata::CreateState (int id) const
{
	return new dAutomataState (id); 
}


int dFiniteAutomata::GetScapeChar (int symbol)
{
	if ((symbol>>8) == '\\') {
		char low = char (symbol);

		static char scapeSquence[] = "nrtvf~!@#$%^&*()-+{}|:\"<>?`_=[]\\;\',./";
		static char ascciSquence[] = "\n\r\t\v\f~!@#$%^&*()-+{}|:\"<>?`_=[]\\;\',./";
		for (int i = 0; scapeSquence[i]; i ++) {
			if (scapeSquence[i] == low) {
				return ascciSquence[i];
			}
		}
		return low;
	}

	return symbol;
}

