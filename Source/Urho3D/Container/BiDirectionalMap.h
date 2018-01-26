#pragma once
#include "../Container/Str.h"
#include "../Container/HashMap.h"
//two-way map containing pairs of types.
//
//Diagram:
//(first_0, second_0)
//(first_1, second_1)
//(first_2, second_2)
//(first_3, second_3)
//( ..... , ....... )
//(first_N, second_N)
//
//all "firsts" must be unique
//all "seconds" must be uniuque.
//
namespace Urho3D
{
	template <class T>
	class URHO3D_API BiDirectionalMap {
	public:

		//inserts/update entries into both lookups.
		void UpdatePair(T first, T second) {
			byFirst[first] = second;
			bySecond[second] = first;
		}

		void Clear() {
			byFirst.Clear();
			bySecond.Clear();
		}

		/// returns number of pairs in the map
		unsigned int Size() const {
			return byFirst.Size();
		}

		/// returns reference hash map for lookup of seconds by first.
		const HashMap<T, T>& ByFirst() const { return byFirst; }
		const HashMap<T, T>& BySecond() const { return bySecond; }

		/// returns second given first
		T ByFirst(T first) const {
			if (byFirst.Contains(first))
				return *byFirst[first];
			else
				return T();
		}

		/// returns first given second
		T BySecond(T second) const {
			if (bySecond.Contains(second))
				return *bySecond[second];
			else
				return T();
		}

	protected:
		HashMap<T, T> byFirst;
		HashMap<T, T> bySecond;
	};
}
