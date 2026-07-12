/**
 * @brief DataManager CoW 快照功能测试
 *
 * 覆盖：CreateSnapshot / Rollback / CoW / 延迟回滚 / SaveSnapshot / LoadSnapshot / DeleteSnapshot / ListSnapshots / CancelRollback
 *
 */

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataEntry.EntryType;

using Engine::Utils::Data::DataManager;
using Engine::Utils::Data::DataEntry;
using Engine::Engine::Utils::Data::EntryType;

/**
 * @brief 辅助：创建含指定字节值的条目
 */
auto MakeEntry(const std::string& name, uint8_t val, uint32_t size = 4) -> std::shared_ptr<DataEntry>
{
    auto entry = std::make_shared<DataEntry>();
    entry->SetName(name);
    entry->New(size);
    entry->Type.store(static_cast<uint32_t>(EntryType::Binary));
    entry->Write([val, size](const std::shared_ptr<uint8_t[]>& data) {
        memset(data.get(), val, size);
    });
    return entry;
}

/**
 * @brief 辅助：读取条目第一个字节
 */
auto ReadFirstByte(const std::shared_ptr<DataEntry>& entry) -> uint8_t
{
    uint8_t result = 0;
    entry->Read([&result](const std::shared_ptr<uint8_t[]>& data) {
        result = data.get()[0];
    });
    return result;
}

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "  [TEST] " << name << " ... "
#define PASS()                                                                               \
    do {                                                                                     \
        std::cout << "PASSED" << std::endl;                                                  \
        g_passed++;                                                                          \
    } while (0)
#define FAIL(msg)                                                                            \
    do {                                                                                     \
        std::cout << "FAILED: " << msg << std::endl;                                         \
        g_failed++;                                                                          \
    } while (0)
#define CHECK(cond)                                                                          \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            FAIL(#cond);                                                                     \
            return;                                                                          \
        }                                                                                    \
    } while (0)

// ── 测试用例 ──

/**
 * @brief 测试基本 InsertEntry / GetEntry
 */
static void test_basic_insert()
{
    TEST("Basic InsertEntry / GetEntry");
    DataManager dm;

    auto entry = MakeEntry("hello", 0x42);
    dm.InsertEntry("hello", entry);

    auto got = dm.GetEntry("hello");
    CHECK(got != nullptr);
    CHECK(ReadFirstByte(got) == 0x42);

    PASS();
}

/**
 * @brief 测试快照创建后，CoW 保证快照数据不变
 */
static void test_cow_isolation()
{
    TEST("CoW: snapshot isolated from later writes");
    DataManager dm;

    dm.InsertEntry("A", MakeEntry("A", 0xAA));
    dm.InsertEntry("B", MakeEntry("B", 0xBB));

    // 创建快照
    bool ok = dm.CreateSnapshot("s1", { "A", "B" }, "测试 CoW");
    CHECK(ok);

    // 写入 A（应触发 CoW 深拷贝）
    dm.Write("A", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x11;
    });

    // 当前 A 应该是 0x11
    auto curA = dm.GetEntry("A");
    CHECK(ReadFirstByte(curA) == 0x11);

    // 回滚到 s1
    ok = dm.Rollback("s1");
    CHECK(ok);

    // 回滚后 GetEntry 应触发替换并返回快照版本
    auto rolledA = dm.GetEntry("A");
    CHECK(ReadFirstByte(rolledA) == 0xAA);

    PASS();
}

/**
 * @brief 测试多次回滚（保留快照）
 */
static void test_multiple_rollback()
{
    TEST("Multiple rollbacks retain snapshot");
    DataManager dm;

    dm.InsertEntry("X", MakeEntry("X", 0x10));
    dm.CreateSnapshot("v1", { "X" });

    dm.Write("X", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x20;
    });
    dm.CreateSnapshot("v2", { "X" });

    dm.Write("X", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x30;
    });

    // 回滚到 v1
    CHECK(dm.Rollback("v1"));
    auto x1 = dm.GetEntry("X");
    CHECK(ReadFirstByte(x1) == 0x10);

    // 取消回滚后，再回滚到 v2
    dm.CancelRollback();
    // 先写回当前值
    dm.Write("X", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x30;
    });
    CHECK(dm.Rollback("v2"));
    auto x2 = dm.GetEntry("X");
    CHECK(ReadFirstByte(x2) == 0x20);

    PASS();
}

/**
 * @brief 测试延迟回滚：Write 触发 pending 替换后再写
 */
static void test_lazy_rollback_with_write()
{
    TEST("Lazy rollback: Write triggers pending then writes");
    DataManager dm;

    dm.InsertEntry("K", MakeEntry("K", 0x01));
    dm.CreateSnapshot("snap", { "K" });

    dm.Write("K", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0xFF;
    });

    dm.Rollback("snap");

    // Write 应先触发回滚替换，再 CoW 深拷贝，再写入
    dm.Write("K", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x88;
    });

    auto k = dm.GetEntry("K");
    CHECK(ReadFirstByte(k) == 0x88);

    // 快照应该仍然是 0x01
    dm.Rollback("snap");
    auto k2 = dm.GetEntry("K");
    CHECK(ReadFirstByte(k2) == 0x01);

    PASS();
}

/**
 * @brief 测试活动回滚期间创建新快照
 */
static void test_snapshot_during_active_rollback()
{
    TEST("CreateSnapshot during active rollback uses old data");
    DataManager dm;

    dm.InsertEntry("P", MakeEntry("P", 0x50));
    dm.CreateSnapshot("s1", { "P" });

    dm.Write("P", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x60;
    });

    dm.Rollback("s1"); // P 进入 pending（当前仍为 0x60）

    // 在 pending 状态下创建 s2，应取快照旧数据 0x50
    CHECK(dm.CreateSnapshot("s2", { "P" }));

    // 现在触发回滚，然后验证 s2 保存的是 0x50
    dm.CancelRollback();
    dm.Rollback("s2");
    auto p = dm.GetEntry("P");
    CHECK(ReadFirstByte(p) == 0x50);

    PASS();
}

/**
 * @brief 测试 ListSnapshots
 */
static void test_list_snapshots()
{
    TEST("ListSnapshots");
    DataManager dm;

    dm.InsertEntry("a", MakeEntry("a", 0x01));
    dm.CreateSnapshot("alpha", { "a" }, "first");
    dm.CreateSnapshot("beta", { "a" }, "second");

    auto list = dm.ListSnapshots();
    CHECK(list.size() == 2);

    bool has_alpha = false;
    bool has_beta = false;
    for (const auto& info : list) {
        if (info.name == "alpha" && info.description == "first") {
            has_alpha = true;
        }
        if (info.name == "beta" && info.description == "second") {
            has_beta = true;
        }
    }
    CHECK(has_alpha);
    CHECK(has_beta);

    PASS();
}

/**
 * @brief 测试 SaveSnapshot / LoadSnapshot
 */
static void test_save_load_snapshot()
{
    TEST("SaveSnapshot / LoadSnapshot round-trip");
    DataManager dm;

    dm.InsertEntry("data1", MakeEntry("data1", 0xAB, 10));
    dm.InsertEntry("data2", MakeEntry("data2", 0xCD, 10));

    CHECK(dm.CreateSnapshot("persist", { "data1", "data2" }, "持久化测试"));

    const std::string path = "/tmp/test_snapshot_aosrpg.snap";
    CHECK(dm.SaveSnapshot("persist", path));

    // 新 DataManager 加载
    DataManager dm2;
    CHECK(dm2.LoadSnapshot(path));

    auto list = dm2.ListSnapshots();
    CHECK(list.size() == 1);
    CHECK(list[0].name == "persist");
    CHECK(list[0].entry_count == 2);

    // 验证加载的条目数据
    dm2.InsertEntry("data1", MakeEntry("data1", 0x00)); // dummy
    dm2.InsertEntry("data2", MakeEntry("data2", 0x00)); // dummy
    CHECK(dm2.Rollback("persist"));

    auto e1 = dm2.GetEntry("data1");
    auto e2 = dm2.GetEntry("data2");
    CHECK(e1 != nullptr);
    CHECK(e2 != nullptr);

    // 验证大小
    CHECK(e1->Size.load() == 10);
    CHECK(e2->Size.load() == 10);

    // 清理
    std::remove(path.c_str());

    PASS();
}

/**
 * @brief 测试 DeleteSnapshot
 */
static void test_delete_snapshot()
{
    TEST("DeleteSnapshot");
    DataManager dm;

    dm.InsertEntry("z", MakeEntry("z", 0x99));
    dm.CreateSnapshot("tmp", { "z" });
    CHECK(dm.ListSnapshots().size() == 1);

    CHECK(dm.DeleteSnapshot("tmp"));
    CHECK(dm.ListSnapshots().size() == 0);

    // 删除不存在的快照应返回 false
    CHECK(!dm.DeleteSnapshot("nonexistent"));

    PASS();
}

/**
 * @brief 测试 CancelRollback
 */
static void test_cancel_rollback()
{
    TEST("CancelRollback");
    DataManager dm;

    dm.InsertEntry("q", MakeEntry("q", 0x01));
    dm.CreateSnapshot("s", { "q" });

    dm.Write("q", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0x02;
    });

    dm.Rollback("s");
    dm.CancelRollback();

    // 取消后 GetEntry 不应触发替换
    auto q = dm.GetEntry("q");
    CHECK(ReadFirstByte(q) == 0x02);

    PASS();
}

/**
 * @brief 测试重复回滚被拒绝
 */
static void test_double_rollback_rejected()
{
    TEST("Double rollback rejected");
    DataManager dm;

    dm.InsertEntry("r", MakeEntry("r", 0x01));
    dm.CreateSnapshot("s1", { "r" });
    dm.CreateSnapshot("s2", { "r" });

    CHECK(dm.Rollback("s1"));
    CHECK(!dm.Rollback("s2")); // 已有活动回滚，应拒绝

    dm.CancelRollback();
    CHECK(dm.Rollback("s2")); // 取消后可以回滚

    PASS();
}

/**
 * @brief 测试 CreateSnapshotAll
 */
static void test_create_snapshot_all()
{
    TEST("CreateSnapshotAll");
    DataManager dm;

    dm.InsertEntry("a", MakeEntry("a", 0x10));
    dm.InsertEntry("b", MakeEntry("b", 0x20));
    dm.InsertEntry("c", MakeEntry("c", 0x30));

    CHECK(dm.CreateSnapshotAll("full", "全量快照"));

    auto list = dm.ListSnapshots();
    CHECK(list.size() == 1);
    CHECK(list[0].entry_count == 3);

    // 修改所有条目
    dm.Write("a", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0xFF;
    });
    dm.Write("b", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0xFF;
    });
    dm.Write("c", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 0xFF;
    });

    // 回滚验证全量恢复
    dm.Rollback("full");
    CHECK(ReadFirstByte(dm.GetEntry("a")) == 0x10);
    CHECK(ReadFirstByte(dm.GetEntry("b")) == 0x20);
    CHECK(ReadFirstByte(dm.GetEntry("c")) == 0x30);

    PASS();
}

/**
 * @brief 测试 LoadAndRestore
 */
static void test_load_and_restore()
{
    TEST("LoadAndRestore");
    DataManager dm;

    dm.InsertEntry("hp", MakeEntry("hp", 100));
    dm.InsertEntry("mp", MakeEntry("mp", 50));

    CHECK(dm.CreateSnapshotAll("save01", "存档1"));
    const std::string path = "/tmp/test_aosrpg_save.snap";
    CHECK(dm.SaveSnapshot("save01", path));

    // 修改数据
    dm.Write("hp", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 10;
    });
    dm.Write("mp", [](const std::shared_ptr<uint8_t[]>& data) {
        data.get()[0] = 5;
    });

    // 一步加载恢复
    CHECK(dm.LoadAndRestore(path));
    CHECK(ReadFirstByte(dm.GetEntry("hp")) == 100);
    CHECK(ReadFirstByte(dm.GetEntry("mp")) == 50);

    // 快照应已加载到内存
    auto list = dm.ListSnapshots();
    CHECK(list.size() >= 1);

    std::remove(path.c_str());
    PASS();
}

/**
 * @brief 测试 PeekSnapshot
 */
static void test_peek_snapshot()
{
    TEST("PeekSnapshot");
    DataManager dm;

    dm.InsertEntry("x", MakeEntry("x", 0x42));
    dm.CreateSnapshot("peek_me", { "x" }, "预览测试");

    const std::string path = "/tmp/test_aosrpg_peek.snap";
    CHECK(dm.SaveSnapshot("peek_me", path));

    // 纯只读预览
    auto info = DataManager::PeekSnapshot(path);
    CHECK(info.has_value());
    CHECK(info->name == "peek_me");
    CHECK(info->description == "预览测试");
    CHECK(info->entry_count == 1);
    CHECK(info->keys.size() == 1);
    CHECK(info->keys[0] == "x");

    // 确认快照没有被加载到内存
    DataManager dm2;
    CHECK(dm2.ListSnapshots().size() == 0);

    // 格式错误的文件
    auto bad = DataManager::PeekSnapshot("/tmp/nonexistent.snap");
    CHECK(!bad.has_value());

    std::remove(path.c_str());
    PASS();
}

// ── 入口 ──

auto main() -> int
{
    std::cout << "=== DataManager CoW Snapshot Tests ===" << std::endl
              << std::endl;

    test_basic_insert();
    test_cow_isolation();
    test_multiple_rollback();
    test_lazy_rollback_with_write();
    test_snapshot_during_active_rollback();
    test_list_snapshots();
    test_save_load_snapshot();
    test_delete_snapshot();
    test_cancel_rollback();
    test_double_rollback_rejected();
    test_create_snapshot_all();
    test_load_and_restore();
    test_peek_snapshot();

    std::cout << std::endl
              << "=== Results: " << g_passed << " passed, " << g_failed << " failed ===" << std::endl;

    return g_failed > 0 ? 1 : 0;
}
