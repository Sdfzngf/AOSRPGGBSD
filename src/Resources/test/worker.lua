-- worker.lua: Worker 线程执行的脚本
-- 测试 dm:read, dm:write, getworkername, should_exit, spawn_worker

print("[worker] Worker started")

-- 获取 Worker 名称
local my_name = dm.getworkername()
print("[worker] My name: " .. my_name)

-- 读取主线程写入的数据
local data = dm.read("test_input")
if data then
    print("[worker] Read test_input: msg=" .. data.msg .. ", count=" .. data.count)
else
    print("[worker] test_input not found")
end

-- 写入 Worker 的输出
dm.write("test_output", {
    worker_name = my_name,
    received = data,
    computed = data and data.count * 2 or 0
})

-- 测试 should_exit
if dm.should_exit() then
    print("[worker] should_exit is true (unexpected at this point)")
else
    print("[worker] should_exit is false (expected)")
end

-- 测试 spawn_worker：创建子 Worker
print("[worker] Spawning child worker...")
local child_handle = dm.spawn_worker("child_worker", "__Engine_Test_Worker__@worker.lua")
if child_handle then
    print("[worker] Child worker created, name: " .. child_handle.name())
    print("[worker] Child is_running: " .. tostring(child_handle.is_running()))
    -- 等待子 Worker 完成
    child_handle:join()
    print("[worker] Child worker finished, is_running: " .. tostring(child_handle.is_running()))
else
    print("[worker] Failed to create child worker")
end
print("[worker] Worker done, exiting")
