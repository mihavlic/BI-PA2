#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>

static const unsigned int OFFSET_BASIS = 2166136261u;
static const unsigned int FNV_PRIME = 16777619u;

// fnv hashing implementation from
// https://gist.github.com/hwei/1950649d523afd03285c
class FnvHasher {
  unsigned int state;

public:
  FnvHasher() : state(OFFSET_BASIS) {}
  void hash_str(const char *c) {
    const char *start = c;
    while (*c) {
      hash_char(*c++);
    }
    // disambiguate situations like
    // ""  "a"  ""
    // ""  ""  "a"
    int len = c - start;
    hash_int(len);
  }
  inline void hash_char(char c) {
    state ^= c;
    state *= FNV_PRIME;
  }
  inline void hash_int(int i) {
    // https://stackoverflow.com/a/5585547
    const char *bytes =
        static_cast<const char *>(static_cast<const void *>(&i));
    hash_char(bytes[0]);
    hash_char(bytes[1]);
    hash_char(bytes[2]);
    hash_char(bytes[3]);
  }
  void unsafe_set_state(unsigned int state_) { state = state_; }
  unsigned int finish() { return state; }
};

template <typename K, typename V> class Map {
  class Entry {
    unsigned int m_hash;
    K m_key;
    V m_value;

  public:
    Entry() : m_hash(0) {}
    Entry(unsigned int hash, K key, V value)
        : m_hash(hash), m_key(key), m_value(value) {}
    // hash of 0 means that the entry is unoccupied
    inline bool is_empty() const { return m_hash == 0; }
    inline bool contains_key(const K &key, const unsigned int hash) const {
      return hash == m_hash && key == m_key;
    }
  };

  std::vector<Entry> m_entries;
  int m_occupied;

  void resize(int new_capacity) {
    assert(m_occupied <= new_capacity);

    Map<K, V> bigger(new_capacity);
    for (Entry entry : std::move(m_entries)) {
      if (entry.hash != 0) {
        bigger.insert(entry.key, entry.value, nullptr);
      }
    }
    this = bigger;
  }

  static unsigned int hash_key(const K &key) {
    FnvHasher hasher{};
    key.hash(hasher);
    return hasher.finish();
  }

  Entry &get_entry(size_t entry_index) { return m_entries[entry_index]; }

  Entry *find_entry(const K &key, const unsigned int hash) {
    if (m_entries.empty()) {
      return NULL;
    }

    // we need to compute modulo of the entry count, len is always power of two,
    // so this works
    unsigned int len = size();
    unsigned int mask = len - 1;
    unsigned int bin = hash & mask;

    // FIXME this needs to iterate all entries to fail to find entry
    // round robin hashing sounds good
    for (unsigned int i = 0; i < len; i++, bin++) {
      Entry &entry = get_entry(bin & mask);
      if (entry.is_empty() || entry.contains_key(key, hash)) {
        return entry;
      }
    }

    return NULL;
  }

public:
  Map() : m_entries(std::vector<K, V>()), m_occupied(0) {}
  Map(size_t capacity)
      : m_entries(Entry{}, std::vector<K, V>(capacity)), m_occupied(0) {}

  size_t size() { return m_entries.size(); }

  bool insert(K key, V value, V *prev) {
    int len = size();
    if (len == 0) {
      resize(8);
    } else if (m_occupied > len / 2) {
      resize(len * 2);
    }

    const unsigned int hash = hash_key(key);
    Entry *entry = find_entry(key, value);

    assert(entry);

    bool contained = !entry->is_empty();
    if (prev && contained) {
      *prev = std::move(entry->value);
    }

    m_occupied++;

    *entry = Entry(hash, key, value);
    return contained;
  }
  bool remove(K key, V value, V *out) {
    unsigned int hash = hash_key(key);
    Entry *entry = find_entry(key, hash);

    if (entry) {
      *out = std::move(entry->value);
      return true;
    } else {
      return false;
    }
  }
};