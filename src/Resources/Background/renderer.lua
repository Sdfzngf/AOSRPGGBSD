local math = require("math")
print("1156137")
bg = os.time()
i=0;
elapsed = 0
b={"手感不好","网卡","断触","卡键","手抖","误触","帧率低","高延迟","瓶颈期","没手感","没心态","没状态","刚睡醒","没支架","打累了","没散热器","卡帧掉帧","手冷手汗","周围太吵","外卖到了","手冻僵了","在上厕所","预判失误","随便玩玩","我在上课","没戴指套","没戴耳机","屏幕太滑了","手机太烫了","腱鞘炎犯了","天气太冷了","对面开挂了","好久没玩了","我妈叫我了","耳机没电了","电量低提示了","灵敏度没改","上线第一把","刚回游不会玩","按键有问题","打电话听不到声音","皮肤手感不行","充电影响发挥","有人打电话给我","有人发消息不小心点到了","新换的键位不熟悉","边吃饭边打的","刚刚在回信息","我这边好吵","坐久了肩酸","手机没平板好用","刚刚黑客入侵了"}
while true do
    local dt = sleep_frame()
    if dt == 0.0 then
        break
    end
    elapsed=elapsed+dt
    gui.rect(0, 0, 640, 480, 255, 0, 0, 255, 10)
    gui.debug_text("wa da xi wa L de su",0,0,255,0,0,255,4,114514)
    gui.debug_text("zhe hang zi shi lua hui zhi chu lai de",0,400,255,0,0,255,3,114514)
    gui.debug_text(tostring(dt) .. "," .. tostring(os.time()),0,100,255,0,0,255,2,114514)
    red = 0.5 + 0.5 * math.sin(elapsed)
    green = 0.5 + 0.5 * math.sin(elapsed + math.pi * 2 / 3)
    blue = 0.5 + 0.5 * math.sin(elapsed + math.pi * 4 / 3)
    gui.debug_text(tostring(os.time()-
    bg),math.sin(elapsed)*200+320,math.cos(elapsed)*200+240,math.floor(red*255),math.floor(green*255),math.floor(blue*255),255,5,114514)
    gui.text("t11est", "__Engine_Font__@SourceHanSans", math.sin(elapsed)*200+320, math.cos(elapsed)*200+240, math.floor(red*255),math.floor(green*255),math.floor(blue*255), 255, math.floor((red+math.pi /2)*255),math.floor((green+math.pi /2)*255),math.floor((blue+math.pi /2)*255), 255, math.abs(math.floor(red*20)+1), 1,0,0,0, 999999)
    gui.text(b[math.random(1, #b)], "__Engine_Font__@SourceHanSans", math.sin(elapsed)*100+320, math.cos(elapsed)*100+240, math.floor(red*255),math.floor(green*255),math.floor(blue*255), 255, math.floor((red+math.pi /2)*255),math.floor((green+math.pi /2)*255),math.floor((blue+math.pi /2)*255), 255, 70, 3,math.abs(math.floor(red*360)+1),math.floor(blue*255),math.floor(green*20), 999999)
    gui.text(tostring(i), "__Engine_Font__@SourceHanSans", 0, 40, 0,0,0, 255, 0,0,0, 255, 20, 0,0,0,0, 999999)
    vec=dm.getlist()
    for i, v in pairs(vec) do
        gui.text(v, "__Engine_Font__@SourceHanSans", 0, (i-1)*20, 0,0,0, 255, 0,0,0, 255, 20, 0,0,0,0, 999999)
    end
    --sleep(1000)
    i=i+1
end
print("myGO")
