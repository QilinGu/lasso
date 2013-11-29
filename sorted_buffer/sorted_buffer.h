

//
#ifndef SORTED_BUFFER_SORTED_BUFFER_H_
#define SORTED_BUFFER_SORTED_BUFFER_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include "boost/scoped_ptr.hpp"

#include "base/common.h"
#include "sorted_buffer/memory_piece.h"
#include "sorted_buffer/memory_allocator.h"

namespace sorted_buffer {

class SortedBufferIterator;

// To buffer a massive set of map outputs (key-value pairs) sorted by
// key.  Once the buffer is close to full, the content is output into
// a disk file and the buffer is cleared.  This ensures that key-value
// pairs in each file are sorted.  This gives SortedBufferIterator the
// chance to traverse all files for sorted map outputs.
class SortedBuffer {
 public:
  SortedBuffer(const std::string& disk_file_base,
                    int in_memory_buffer_size);
  ~SortedBuffer();

  void Insert(const std::string& key, const std::string& value);

  void Flush();

  // The caller is responsible to delete the iterator.
  SortedBufferIterator* CreateIterator() const;

  // Remove buffer files generated by Flush().
  void RemoveBufferFiles() const;

  static std::string SortedFilename(const std::string filebase, int index);

  NaiveMemoryAllocator* Allocator() { return allocator_.get(); }
  int NumFiles() { return count_files_; }

 private:
  struct KeyValuePair {
    MemoryPiece key;
    MemoryPiece value;
    KeyValuePair(const MemoryPiece& k,
                 const MemoryPiece& v)
        : key(k), value(v) {}
  };
  typedef std::vector<KeyValuePair> KeyValueList;

  static bool KeyValuePairLessThan(const KeyValuePair& x,
                                   const KeyValuePair& y);
  static bool KeyValuePairEqual(const KeyValuePair& x,
                                const KeyValuePair& y);

  KeyValueList key_value_list_;
  std::string filebase_;
  boost::scoped_ptr<NaiveMemoryAllocator> allocator_;
  int count_files_;

  DISALLOW_COPY_AND_ASSIGN(SortedBuffer);
};

}  // namespace sorted_buffer

#endif  // SORTED_BUFFER_SORTED_BUFFER_H_
