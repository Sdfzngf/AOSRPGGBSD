while true do
    local dt = sleep_frame()
    if dt == 0.0 then
        break
    end
    gui.rect(0, 0, 100, 100, 255, 255, 255, 255, 1919811)
end
