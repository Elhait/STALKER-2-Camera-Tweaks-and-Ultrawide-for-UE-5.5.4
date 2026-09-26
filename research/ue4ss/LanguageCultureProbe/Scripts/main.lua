-- Read-only probe for the game's active Unreal text/culture state.
-- F2: after the initial UI is visible; F3: after applying Interface Language;
-- F4: after restart, once the UI is visible again.

local MODULE = "LANGUAGE_CULTURE_PROBE"
local API_PATH = "/Script/Engine.Default__KismetInternationalizationLibrary"

local function safeText(value)
    if value == nil then return "<nil>" end
    if type(value) == "userdata" then
        local hasConverter, converter = pcall(function() return value.ToString end)
        if hasConverter and converter ~= nil then
            local converted, result = pcall(function() return value:ToString() end)
            if converted and result ~= nil then return result end
        end
    end
    local ok, result = pcall(function() return tostring(value) end)
    if ok then return result end
    return "<string-conversion-failed>"
end

local function capture(stage)
    local ok, api = pcall(function() return StaticFindObject(API_PATH) end)
    if not ok or api == nil then
        print(MODULE .. " stage=" .. stage .. " api=" ..
            (ok and "<not-found>" or safeText(api)) .. "\n")
        return
    end

    local fields = {
        { "GetCurrentLanguage", "language" },
        { "GetCurrentLocale", "locale" },
        { "GetCurrentCulture", "culture" },
    }
    local values = {}
    for _, field in ipairs(fields) do
        local methodName, outputName = field[1], field[2]
        local callOk, value = pcall(function()
            local method = api[methodName]
            if method == nil then error("reflected method missing") end
            return method(api)
        end)
        values[#values + 1] = outputName .. "=" ..
            (callOk and safeText(value) or "<call-failed:" .. safeText(value) .. ">")
    end

    print(MODULE .. " stage=" .. stage .. " " .. table.concat(values, " ") .. "\n")
end

local function bind(key, stage)
    RegisterKeyBind(key, function()
        ExecuteInGameThread(function() capture(stage) end)
    end)
end

bind(0x71, "initial-ui-ready") -- F2
bind(0x72, "after-interface-language-apply") -- F3
bind(0x73, "after-restart-ui-ready") -- F4

print(MODULE .. " loaded; read-only; F2=initial, F3=after-apply, F4=after-restart\n")
