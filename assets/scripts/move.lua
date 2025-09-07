function onStart(obj)
    print("Start: " .. obj:getName())
end

function onUpdate(obj, dt)
    local t = obj:getTransform()
    local p = t:getPosition()
    p.x = p.x + dt
    t:setPosition(p)
end