/*************************************************************************
 *
 * TIGHTDB CONFIDENTIAL
 * __________________
 *
 *  [2011] - [2012] TightDB Inc
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of TightDB Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to TightDB Incorporated
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from TightDB Incorporated.
 *
 **************************************************************************/
#ifndef TIGHTDB_GROUP_WRITER_HPP
#define TIGHTDB_GROUP_WRITER_HPP

#ifdef _MSC_VER
#include <win32/stdint.h>
#else
#include <stdint.h> // unint8_t etc
#endif
#include <cstdlib> // size_t

namespace tightdb {

// Pre-declarations
class Group;
class SlabAlloc;

class GroupWriter {
public:
    GroupWriter(Group& group, bool doPersist);
    ~GroupWriter();

    bool IsValid() const;
    void SetVersions(size_t current, size_t readlock);

    size_t Commit();

    size_t write(const char* p, size_t n);
    void WriteAt(size_t pos, const char* p, size_t n);

#ifdef TIGHTDB_DEBUG
    void dump();
    void ZeroFreeSpace();
#endif

private:
    void DoCommit(uint64_t topPos);

    size_t get_free_space(size_t len, size_t& filesize);
    size_t reserve_free_space(size_t len, size_t& filesize, size_t start=0);
    void   add_free_space(size_t pos, size_t len, size_t version=0);
    void   merge_free_space();
    size_t extend_free_space(size_t len, size_t& filesize);

    // Member variables
    Group&     m_group;
    SlabAlloc& m_alloc;
    size_t     m_current_version;
    size_t     m_readlock_version;
    size_t     m_len;
    char*      m_data;
    bool       m_doPersist;
};


} // namespace tightdb

#endif // TIGHTDB_GROUP_WRITER_HPP
