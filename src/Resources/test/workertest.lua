-- workertest.lua: 主线程 Lua 脚本，用于测试 Worker 功能
-- 由 StartUp 中的 RunScript 执行
print("[workertest] Starting Worker test...")

-- 先写入测试数据
dm.write("test_input", { msg = "Hello from main thread", count = 42 })

-- 主线程 fire-and-forget 创建 Worker
local ok = dm.create_worker("test_worker", "__Engine_Test_Worker__@worker.lua")
if ok then
    print("[workertest] Worker 'test_worker' created successfully")
else
    print("[workertest] Failed to create worker 'test_worker'")
end

-- 再创建一个 Worker 测试名称冲突
local ok2 = dm.create_worker("test_worker", "__Engine_Test_Worker__@worker.lua")
if not ok2 then
    print("[workertest] Correctly refused duplicate worker name 'test_worker'")
end

print("[workertest] Test script done")
