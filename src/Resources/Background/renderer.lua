local math = require("math")
print("1156137")
bg = os.time()
i=0;
elapsed = 0
while true do
    local dt = sleep_frame()
    elapsed=elapsed+dt
    gui.rect(0, 0, 640, 480, 255, 0, 0, 255, 10)
    gui.text("wa da xi wa L de su",0,0,255,0,0,255,4,114514)
    gui.text("zhe hang zi shi lua hui zhi chu lai de",0,400,255,0,0,255,3,114514)
    gui.text(tostring(dt) .. "," .. tostring(os.time()),0,100,255,0,0,255,2,114514)
    gui.text(tostring(os.time()-bg),math.sin(elapsed)*200+320,math.cos(elapsed)*200+240,math.floor(math.cos(elapsed*1)*255/2),math.floor(math.cos(elapsed*2)*255/2),math.floor(math.cos(elapsed*3)*255/2),255,5,114514)
end
