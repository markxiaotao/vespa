// Copyright 2019 Oath Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/vespalib/objects/objectvisitor.h>
#include <vespa/vespalib/stllike/string.h>
#include <vespa/vespalib/util/memory.h>

namespace search {

/**
 * Basic representation of a query term.
 */
class QueryTermSimple {
public:
    typedef std::unique_ptr<QueryTermSimple> UP;
    typedef vespalib::string string;
    typedef vespalib::stringref stringref;
    enum SearchTerm {
        WORD,
        PREFIXTERM,
        SUBSTRINGTERM,
        EXACTSTRINGTERM,
        SUFFIXTERM,
        REGEXP
    };

    template <typename N>
    struct RangeResult {
        N low;
        N high;
        bool valid; // Whether parsing of the range was successful
        bool adjusted; // Whether the low and high was adjusted according to min and max limits of the given type.
        RangeResult() : low(), high(), valid(true), adjusted(false) {}
        bool isEqual() const { return low == high; }
    };

    QueryTermSimple(const QueryTermSimple &) = default;
    QueryTermSimple & operator = (const QueryTermSimple &) = default;
    QueryTermSimple(QueryTermSimple &&) = default;
    QueryTermSimple & operator = (QueryTermSimple &&) = default;
    QueryTermSimple();
    QueryTermSimple(const string & term_, SearchTerm type);
    virtual ~QueryTermSimple();
    /**
     * Extracts the content of this query term as a range with low and high values.
     */
    template <typename N>
    RangeResult<N> getRange() const;
    int                         getRangeLimit() const { return _rangeLimit; }
    size_t                     getMaxPerGroup() const { return _maxPerGroup; }
    size_t           getDiversityCutoffGroups() const { return _diversityCutoffGroups; }
    bool             getDiversityCutoffStrict() const { return _diversityCutoffStrict; }
    vespalib::stringref getDiversityAttribute() const { return _diversityAttribute; }
    bool getAsIntegerTerm(int64_t & lower, int64_t & upper) const;
    bool getAsDoubleTerm(double & lower, double & upper) const;
    const char * getTerm() const { return _term.c_str(); }
    bool isPrefix()        const { return (_type == PREFIXTERM); }
    bool isSubstring()     const { return (_type == SUBSTRINGTERM); }
    bool isExactstring()   const { return (_type == EXACTSTRINGTERM); }
    bool isSuffix()        const { return (_type == SUFFIXTERM); }
    bool isWord()          const { return (_type == WORD); }
    bool isRegex()         const { return (_type == REGEXP); }
    bool empty()           const { return _term.empty(); }
    virtual void visitMembers(vespalib::ObjectVisitor &visitor) const;
    vespalib::string getClassName() const;
    bool isValid() const { return _valid; }
protected:
    const string & getTermString() const { return _term; }
private:
    bool getRangeInternal(int64_t & low, int64_t & high) const;
    template <typename N>
    RangeResult<N> getIntegerRange() const;
    template <typename N>
    RangeResult<N>    getFloatRange() const;
    SearchTerm  _type;
    int         _rangeLimit;
    uint32_t    _maxPerGroup;
    uint32_t    _diversityCutoffGroups;
    bool        _diversityCutoffStrict;
    bool        _valid;
    string      _term;
    stringref   _diversityAttribute;
    template <typename T, typename D>
    bool    getAsNumericTerm(T & lower, T & upper, D d) const;
};

}

void visit(vespalib::ObjectVisitor &self, const vespalib::string &name,
           const search::QueryTermSimple &obj);
void visit(vespalib::ObjectVisitor &self, const vespalib::string &name,
           const search::QueryTermSimple *obj);
