#include "testsettings.hpp"
#ifdef TEST_REPLICATION

#include <algorithm>

#include <tightdb.hpp>
#include <tightdb/util/features.h>
#include <tightdb/util/unique_ptr.hpp>
#include <tightdb/util/file.hpp>
#ifdef TIGHTDB_ENABLE_REPLICATION
#  include <tightdb/replication.hpp>
#endif

#include "test.hpp"

#ifdef TIGHTDB_ENABLE_REPLICATION

using namespace std;
using namespace tightdb;
using namespace tightdb::util;


// Test independence and thread-safety
// -----------------------------------
//
// All tests must be thread safe and independent of each other. This
// is required because it allows for both shuffling of the execution
// order and for parallelized testing.
//
// In particular, avoid using std::rand() since it is not guaranteed
// to be thread safe. Instead use the API offered in
// `test/util/random.hpp`.
//
// All files created in tests must use the TEST_PATH macro (or one of
// its friends) to obtain a suitable file system path. See
// `test/util/test_path.hpp`.
//
//
// Debugging and the ONLY() macro
// ------------------------------
//
// A simple way of disabling all tests except one called `Foo`, is to
// replace TEST(Foo) with ONLY(Foo) and then recompile and rerun the
// test suite. Note that you can also use filtering by setting the
// environment varible `UNITTEST_FILTER`. See `README.md` for more on
// this.
//
// Another way to debug a particular test, is to copy that test into
// `experiments/testcase.cpp` and then run `sh build.sh
// check-testcase` (or one of its friends) from the command line.


namespace {

class MyTrivialReplication: public TrivialReplication {
public:
    MyTrivialReplication(string path): TrivialReplication(path) {}

    ~MyTrivialReplication() TIGHTDB_NOEXCEPT
    {
        typedef TransactLogs::const_iterator iter;
        iter end = m_transact_logs.end();
        for (iter i = m_transact_logs.begin(); i != end; ++i)
            delete[] i->data();
    }

    void replay_transacts(SharedGroup& target, ostream* replay_log = 0)
    {
        typedef TransactLogs::const_iterator iter;
        iter end = m_transact_logs.end();
        for (iter i = m_transact_logs.begin(); i != end; ++i)
            apply_transact_log(i->data(), i->size(), target, replay_log);
    }

private:
    void handle_transact_log(const char* data, size_t size, version_type) TIGHTDB_OVERRIDE
    {
        UniquePtr<char[]> log(new char[size]); // Throws
        copy(data, data+size, log.get());
        m_transact_logs.push_back(BinaryData(log.get(), size)); // Throws
        log.release();
    }

    typedef vector<BinaryData> TransactLogs;
    TransactLogs m_transact_logs;
};

TIGHTDB_TABLE_1(MySubsubsubtable,
                i, Int)

TIGHTDB_TABLE_3(MySubsubtable,
                a, Int,
                b, Subtable<MySubsubsubtable>,
                c, Int)

TIGHTDB_TABLE_1(MySubtable,
                t, Subtable<MySubsubtable>)

TIGHTDB_TABLE_9(MyTable,
                my_int,       Int,
                my_bool,      Bool,
                my_float,     Float,
                my_double,    Double,
                my_string,    String,
                my_binary,    Binary,
                my_date_time, DateTime,
                my_subtable,  Subtable<MySubtable>,
                my_mixed,     Mixed)


TEST(Replication_General)
{
    SHARED_GROUP_TEST_PATH(path_1);
    SHARED_GROUP_TEST_PATH(path_2);
    {
        MyTrivialReplication repl(path_1);
        SharedGroup sg_1(repl);
        {
            WriteTransaction wt(sg_1);
            MyTable::Ref table = wt.get_table<MyTable>("my_table");
            table->add();
            wt.commit();
        }
        {
            WriteTransaction wt(sg_1);
            MyTable::Ref table = wt.get_table<MyTable>("my_table");
            char buf[] = { '1' };
            BinaryData bin(buf);
            Mixed mix;
            mix.set_int(1);
            table->set(0, 1, true, 1.0f, 1.0, "x", bin, 727, 0, mix);
            table->add(2, true, 2.0f, 2.0, "xx", bin, 728, 0, mix);
            table->insert(0, 3, true, 3.0f, 3.0, "xxx", bin, 729, 0, mix);
            wt.commit();
        }
        {
            WriteTransaction wt(sg_1);
            MyTable::Ref table = wt.get_table<MyTable>("my_table");
            table[0].my_int = 9;
            wt.commit();
        }
        {
            WriteTransaction wt(sg_1);
            MyTable::Ref table = wt.get_table<MyTable>("my_table");
            table[0].my_int = 10;
            wt.commit();
        }

        ostream* replay_log = 0;
//        replay_log = &cout;
        SharedGroup sg_2(path_2);
        repl.replay_transacts(sg_2, replay_log);

        {
            ReadTransaction rt_1(sg_1);
            ReadTransaction rt_2(sg_2);
            CHECK(rt_1.get_group() == rt_2.get_group());
        }
    }
}

} // anonymous namespace

#endif // TIGHTDB_ENABLE_REPLICATION
#endif // TEST_REPLICATION