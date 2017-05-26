#pragma once

#include "search/search_quality/sample.hpp"

#include "base/scope_guard.hpp"

#include <cstddef>
#include <functional>
#include <limits>
#include <type_traits>
#include <vector>

class Edits
{
public:
  using Relevance = search::Sample::Result::Relevance;

  struct Entry
  {
    enum class Origin
    {
      Loaded,
      Created
    };

    Entry() = default;
    Entry(Relevance relevance, Origin origin)
      : m_curr(relevance), m_orig(relevance), m_origin(origin)
    {
    }

    Relevance m_curr = Relevance::Irrelevant;
    Relevance m_orig = Relevance::Irrelevant;
    bool m_deleted = false;
    Origin m_origin = Origin::Loaded;
  };

  struct Update
  {
    static auto constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    enum class Type
    {
      Single,
      All,
      Add,
      Delete,
      Resurrect
    };

    Update() = default;
    Update(Type type, size_t index): m_type(type), m_index(index) {}

    static Update MakeAll() { return Update{}; }
    static Update MakeSingle(size_t index) { return Update{Type::Single, index}; }
    static Update MakeAdd(size_t index) { return Update{Type::Add, index}; }
    static Update MakeDelete(size_t index) { return Update{Type::Delete, index}; }
    static Update MakeResurrect(size_t index) { return Update{Type::Resurrect, index}; }

    Type m_type = Type::All;
    size_t m_index = kInvalidIndex;
  };

  using OnUpdate = std::function<void(Update const & update)>;

  class Editor
  {
  public:
    Editor(Edits & parent, size_t index);

    // Sets relevance to |relevance|. Returns true iff |relevance|
    // differs from the original one.
    bool Set(Relevance relevance);
    Relevance Get() const;
    bool HasChanges() const;
    Entry::Origin GetOrigin() const;

  private:
    Edits & m_parent;
    size_t m_index = 0;
  };

  explicit Edits(OnUpdate onUpdate) : m_onUpdate(onUpdate) {}

  void Apply();
  void Reset(std::vector<Relevance> const & relevances);

  // Sets relevance at |index| to |relevance|. Returns true iff
  // |relevance| differs from the original one.
  bool SetRelevance(size_t index, Relevance relevance);

  // Addes new entry.
  void Add(Relevance relevance);

  // Marks entry at |index| as deleted.
  void Delete(size_t index);

  // Resurrects previously deleted entry at |index|.
  void Resurrect(size_t index);

  std::vector<Entry> const & GetEntries() const { return m_entries; }
  Entry & GetEntry(size_t index);
  Entry const & GetEntry(size_t index) const;
  size_t NumEntries() const { return m_entries.size(); }
  std::vector<Relevance> GetRelevances() const;

  Entry const & Get(size_t index) const;


  void Clear();
  bool HasChanges() const;
  bool HasChanges(size_t index) const;

private:
  template <typename Fn>
  typename std::result_of<Fn()>::type WithObserver(Update const & update, Fn && fn)
  {
    MY_SCOPE_GUARD(cleanup, ([this, &update]() {
                     if (m_onUpdate)
                       m_onUpdate(update);
                   }));
    return fn();
  }

  std::vector<Entry> m_entries;

  size_t m_numEdits = 0;

  OnUpdate m_onUpdate;
};
