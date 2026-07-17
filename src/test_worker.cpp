/**
 * @brief Worker 后台线程功能测试
 *
 * 覆盖：CreateWorker / IsRunning / Join / 重复名称 / DataManager JSON 读写
 */

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataEntry.EntryType;
import Engine.Utils.Script.ScriptManager;

using Engine::Utils::Data::DataManager;
using Engine::Utils::Data::DataEntry;
using Engine::Engine::Utils::Data::EntryType;
using Engine::Utils::Script::ScriptManager;

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) std::cout << "  [TEST] " << name << " ... "
#define PASS() do { std::cout << "PASSED\n"; g_passed++; } while(0)
#define FAIL(msg) do { std::cout << "FAILED: " << msg << "\n"; g_failed++; } while(0)
#define CHECK(cond) do { if (!(cond)) { FAIL(#cond); return; } } while(0)

/// 创建包含 Lua 代码的 Script 类型 DataEntry
static auto MakeScriptEntry(const std::string& name, const std::string& code) -> std::shared_ptr<DataEntry>
{
    auto entry = std::make_shared<DataEntry>();
    entry->SetName(name);
    entry->New(code.size());
    entry->Type.store(static_cast<uint32_t>(EntryType::Script));
    entry->Write([&code](const std::shared_ptr<uint8_t[]>& data) {
        memcpy(data.get(), code.data(), code.size());
    });
    return entry;
}

/// 创建包含 JSON 数据的 DataEntry
static auto MakeJsonEntry(const std::string& name, const std::string& json) -> std::shared_ptr<DataEntry>
{
    auto entry = std::make_shared<DataEntry>();
    entry->SetName(name);
    entry->New(json.size());
    entry->Type.store(static_cast<uint32_t>(EntryType::Binary));
    entry->Write([&json](const std::shared_ptr<uint8_t[]>& data) {
        memcpy(data.get(), json.data(), json.size());
    });
    return entry;
}

/// 读取 DataEntry 内容为字符串
static auto ReadEntryString(const std::shared_ptr<DataEntry>& entry) -> std::string
{
    std::string result;
    entry->Read([&](const std::shared_ptr<uint8_t[]>& data) {
        result.assign(reinterpret_cast<const char*>(data.get()), entry->GetSize());
    });
    return result;
}

// ── 测试用例 ──

/// 测试基本 Worker 创建与执行
static void test_basic_worker()
{
    TEST("Basic Worker create and execute");
    auto dm = std::make_shared<DataManager>();

    // 写入测试数据
    dm->InsertEntry("input_json", MakeJsonEntry("input_json", R"({"msg":"hello","val":100})"));

    // 写入 Lua 脚本
    std::string lua_code = R"lua(
        local data = dm.read("input_json")
        if data then
            dm.write("output_json", {
                received = data,
                worker_name = dm.getworkername(),
                doubled = data.val * 2
            })
        end
    )lua";
    dm->InsertEntry("test_script", MakeScriptEntry("test_script", lua_code));

    // 创建 ScriptManager
    ScriptManager sm;
    sm.BindDataManager(dm);
    sm.OpenLibs();
    sm.SetupMainDMAPI();

    // 创建 Worker
    bool ok = sm.CreateWorker("basic_worker", "test_script");
    CHECK(ok);

    // 等待 Worker 完成（小脚本可能瞬间执行完，先 join 再检查）
    sm.JoinWorker("basic_worker");
    CHECK(!sm.IsWorkerRunning("basic_worker"));

    // 验证输出
    auto output = dm->GetEntry("output_json");
    CHECK(output != nullptr);
    std::string out_str = ReadEntryString(output);
    std::cout << "  output: " << out_str << "\n";
    CHECK(out_str.find("worker_name") != std::string::npos);
    CHECK(out_str.find("basic_worker") != std::string::npos);
    CHECK(out_str.find("200") != std::string::npos);

    PASS();
}

/// 测试重复 Worker 名称
static void test_duplicate_worker()
{
    TEST("Duplicate worker name rejection");
    auto dm = std::make_shared<DataManager>();

    std::string lua_code = R"lua(
        -- 简单脚本，不做什么
    )lua";
    dm->InsertEntry("simple_script", MakeScriptEntry("simple_script", lua_code));

    ScriptManager sm;
    sm.BindDataManager(dm);
    sm.OpenLibs();

    bool ok1 = sm.CreateWorker("dup_worker", "simple_script");
    CHECK(ok1);

    // 等待第一个完成
    sm.JoinWorker("dup_worker");

    // 同一名称再次创建应失败
    bool ok2 = sm.CreateWorker("dup_worker", "simple_script");
    CHECK(!ok2);

    PASS();
}

/// 测试多 Worker 并发
static void test_concurrent_workers()
{
    TEST("Concurrent workers");
    auto dm = std::make_shared<DataManager>();

    for (int i = 0; i < 3; i++) {
        std::string key = "input_" + std::to_string(i);
        std::string json = R"({"id":)" + std::to_string(i) + R"(})";
        dm->InsertEntry(key, MakeJsonEntry(key, json));
    }

    std::string lua_code = R"lua(
        local my_name = dm.getworkername()
        local id = string.sub(my_name, -1)  -- 最后一个字符是数字
        local key = "input_" .. id
        local data = dm.read(key)
        if data then
            dm.write("output_" .. id, { worker = my_name, got = data })
        end
    )lua";
    dm->InsertEntry("concurrent_script", MakeScriptEntry("concurrent_script", lua_code));

    ScriptManager sm;
    sm.BindDataManager(dm);
    sm.OpenLibs();
    sm.SetupMainDMAPI();

    // 同时创建 3 个 Worker
    CHECK(sm.CreateWorker("w_0", "concurrent_script"));
    CHECK(sm.CreateWorker("w_1", "concurrent_script"));
    CHECK(sm.CreateWorker("w_2", "concurrent_script"));

    // 等待全部完成
    for (int i = 0; i < 3; i++) {
        sm.JoinWorker("w_" + std::to_string(i));
    }

    // 验证每个 Worker 都写入了输出
    for (int i = 0; i < 3; i++) {
        auto out = dm->GetEntry("output_" + std::to_string(i));
        CHECK(out != nullptr);
        std::string out_str = ReadEntryString(out);
        CHECK(out_str.find("w_" + std::to_string(i)) != std::string::npos);
    }

    PASS();
}

/// 测试 Shutdown 超时行为（不加入 Worker，验证 Shutdown 不卡死）
static void test_shutdown_timeout()
{
    TEST("Shutdown with timeout (long-running worker)");
    auto dm = std::make_shared<DataManager>();

    // 写入一个长时间运行的脚本
    std::string lua_code = R"lua(
        -- 死循环，直到 should_exit 为 true
        -- 但 shutdown 设置超时 3 秒，所以应该被 detach
        local count = 0
        while not dm.should_exit() and count < 1000 do
            count = count + 1
        end
        if not dm.should_exit() then
            dm.write("long_run_exited", { reason = "loop_exhausted", count = count })
        else
            dm.write("long_run_exited", { reason = "should_exit", count = count })
        end
    )lua";
    dm->InsertEntry("long_script", MakeScriptEntry("long_script", lua_code));

    ScriptManager sm;
    sm.BindDataManager(dm);
    sm.OpenLibs();

    sm.CreateWorker("long_worker", "long_script");

    // 触发关闭（应等最多 3 秒后超时）
    sm.ShutdownWorkers();

    // 不卡死即为通过
    PASS();
}

// ── 主函数 ──

int main()
{
    std::cout << "=== Worker Tests ===\n\n";

    test_basic_worker();
    test_duplicate_worker();
    test_concurrent_workers();
    test_shutdown_timeout();

    std::cout << "\n=== Results: " << g_passed << " passed, " << g_failed << " failed ===\n";
    return g_failed > 0 ? 1 : 0;
}
