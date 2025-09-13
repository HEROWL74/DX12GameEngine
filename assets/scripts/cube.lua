function onStart(obj)
    print("Start: " .. obj:getName())
end

function onUpdate(obj, dt)
    local t = obj:getTransform()
    local p = t:getPosition()
    p.z = p.z + dt
    t:setPosition(p)
end