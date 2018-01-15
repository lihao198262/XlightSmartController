//  LedLevelBar.h - Ordered List lib with consecutive memory allocation
/// Suitable for small array which requires fast search with infrequent insert or delete operations
/// Can access all data chunk at once

#ifndef OrderedList_h
#define OrderedList_h

template <typename T>
class OrderdList
{
protected:
  bool _desc;
  uint8_t _maxlen;
	uint8_t _count;
  uint8_t _size;

  // Allocate new list
  virtual T *newList(uint8_t nSize);

  // Search list for specific item, return the position
  virtual int search(T*, bool bReplace = false);

public:
  T *_pItems;

	OrderdList(uint8_t maxl = 64, bool desc = false, uint8_t initlen = 8);
	virtual ~OrderdList();

	// Returns current size of OrderdList
	virtual uint8_t size();

  // Returns the number of items in OrderdList
	virtual uint8_t count();

  // Compare two items, return 0 if equal, 1 if the first item is larger, -1 if the second item is larger
  virtual int compare(T _first, T _second) = 0;

  // Retrieve item, return the position and data entry
  virtual int get(T*);

  // Update item and return the position
	virtual int update(T*);

  // Add a new item, whose position is determined by the result of compare()
  /// return the inserted postion if succeeds, otherwise return -1
	virtual int add(T*);

  // Remove item
	virtual bool remove(T*);

  // Remove all items
	virtual void removeAll();
};

// Initialize LinkedList with false values
template<typename T>
OrderdList<T>::OrderdList(uint8_t maxl, bool desc, uint8_t initlen)
 : _maxlen(maxl), _desc(desc)
{
  _count = 0;
  _size = 0;
  _pItems = NULL;

  if( initlen > 0 ) {
    _pItems = newList(initlen);
    if( _pItems ) _size = initlen;
  }
}

// Clear items and free Memory
template<typename T>
OrderdList<T>::~OrderdList()
{
  removeAll();
}

// Remove all items
template<typename T>
void OrderdList<T>::removeAll() {
  if(_pItems) {
    delete []_pItems;
    _pItems = NULL;
  }
  _count = 0;
  _size = 0;
}

// Allocate new list
template<typename T>
T *OrderdList<T>::newList(uint8_t nSize) {
  T *pList = NULL;
  if( nSize <= _maxlen && nSize > 0 ) {
    pList = new T[nSize];
    if( pList ) {
      memset(pList, 0x00, sizeof(T) * nSize );
    }
  }
  return pList;
}

// Returns current size of the list
template<typename T>
uint8_t OrderdList<T>::size() {
	return _size;
}

// Returns the number of items in OrderdList
template<typename T>
uint8_t OrderdList<T>::count() {
	return _count;
}

// Search list for specific item, return the position
template<typename T>
int OrderdList<T>::search(T *_pT, bool bReplace) {
  if( !_pT ) return -1;

  int nStart, nMid, nEnd, nFound;
  nStart= 0;
	nEnd = (int)_count - 1;
  nFound = -1;

	while( nStart <= nEnd ) {
		nMid = (nStart + nEnd) / 2;
    int nResult = compare(*_pT, _pItems[nMid]);
    if( nResult > 0 ) {
      if( _desc ) {
        nEnd = nMid - 1;
      } else {
        nStart = nMid + 1;
      }
		} else if( nResult < 0 ) {
      if( _desc ) {
        nStart = nMid + 1;
      } else {
        nEnd = nMid - 1;
      }
		} else {
			nFound = nMid;
			break;
		}
	}

  if( bReplace && nFound < 0 ) {
    nFound = nStart;
  }
  return nFound;
}

// Retrieve item, return the position and data entry
template<typename T>
int OrderdList<T>::get(T *_pT) {
  int pos = search(_pT);
  if( pos >= 0 ) {
    *_pT = _pItems[pos];
  }
  return pos;
}

// Update item and return the position
template<typename T>
int OrderdList<T>::update(T *_pT) {
  // DO NOT use get() here, cuz we don't want to overwrite the input data
  int pos = search(_pT);
  if( pos >= 0 ) {
    // update data
    _pItems[pos] = *_pT;
  }
	return pos;
}

// Add a new item, update if item exists
template<typename T>
int OrderdList<T>::add(T *_pT) {
  int pos = search(_pT, true);
  if( pos < 0 ) return -1;

  if( pos < _count ) {
    if( compare(*_pT, _pItems[pos]) == 0 ) {
      // Update if item exists
      _pItems[pos] = *_pT;
      return pos;
    }
  }

  // Add new at 'pos'
  uint8_t _newCount = _count + 1;
  if( _newCount > _maxlen ) return -1;
  if( _newCount > _size ) {
    // Resize of array
    T *pList = newList(_newCount);
    if( pList ) {
      // Copy Data
      memcpy(pList, _pItems, sizeof(T) * _count);
      // Delete old list
      removeAll();
      // Set new list
      _pItems = pList;
      _size = _newCount;
    } else {
      return -1;
    }
  }
  // Move data
  for( int i = _newCount - 1; i > pos; i-- ) {
    _pItems[i] = _pItems[i-1];
  }
  _pItems[pos] = *_pT;
  _count = _newCount;

	return pos;
}

// Remove item
template<typename T>
bool OrderdList<T>::remove(T *_pT) {
  if( !_pT ) return false;

  // Search item
  int pos = get(_pT);
  if( pos >= 0 ) {
    // Move data
    for( int i = pos; i < _count - 1; i++ ) {
        _pItems[i] = _pItems[i+1];
    }
    // Erase the last item
    _count--;
    memset(&(_pItems[_count]), 0x00, sizeof(T));
    return true;
  }
	return false;
}

#endif // End of OrderedList_h
