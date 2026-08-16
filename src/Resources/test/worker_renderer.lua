print("hellno")
local child_handle = dm.spawn_worker("child_worker_renderer_2", "__Engine_Test_Worker__@worker_renderer_2.lua")
if child_handle then
    print("[worker] Child worker created, name: " .. child_handle.name())
else
    print("[worker] Failed to create child worker")
end
while true do
    local dt = sleep_frame()
    if dt == 0.0 then
        break
    end
    gui.rect(0, 0, 100, 100, 255, 255, 255, 255, 1919811)
end
print("me go")
