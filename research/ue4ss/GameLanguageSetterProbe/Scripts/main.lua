-- Read-only characterization of the game's Interface Language setter.
-- F2 records a baseline; changing Interface Language logs setter entry/exit;
-- F3 records the settled UE language after the UI reflects the change.

local MODULE = "GAME_LANGUAGE_SETTER_PROBE"
local SETTER_PATH = "/Script/Stalker2.CppMediator:SetSelectedTextLanguage"
local KISMET_PATH = "/Script/Engine.Default__KismetInternationalizationLibrary"

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
    return ok and result or "<unprintable>"
end

local function currentLanguage()
    local ok, result = pcall(function()
        local library = StaticFindObject(KISMET_PATH)
        if library == nil then error("Kismet internationalization CDO not found") end
        return library:GetCurrentLanguage():ToString()
    end)
    return ok and result or ("<unavailable:" .. safeText(result) .. ">")
end

local function enumValue(parameter)
    if parameter == nil then return "<no-argument>" end
    local ok, result = pcall(function() return parameter:get() end)
    return ok and safeText(result) or ("<unavailable:" .. safeText(result) .. ">")
end

local function emit(stage, parameter)
    print(MODULE .. " stage=" .. stage ..
        " selected_enum=" .. enumValue(parameter) ..
        " ue_language=" .. currentLanguage() .. "\n")
end

local hookOk, preId, postId = pcall(function()
    return RegisterHook(SETTER_PATH,
        function(_, selectedLanguage)
            emit("setter-pre", selectedLanguage)
        end,
        function(_, selectedLanguage)
            emit("setter-post", selectedLanguage)
        end)
end)

if hookOk and preId ~= nil and postId ~= nil then
    print(MODULE .. " setter_hook=registered path=" .. SETTER_PATH .. "\n")
else
    print(MODULE .. " setter_hook=failed error=" .. safeText(preId) .. "\n")
end

RegisterKeyBind(0x71, function()
    ExecuteInGameThread(function() emit("manual-baseline", nil) end)
end) -- F2

RegisterKeyBind(0x72, function()
    ExecuteInGameThread(function() emit("manual-settled-after-change", nil) end)
end) -- F3

print(MODULE .. " loaded; read-only; F2=baseline, change Interface Language, F3=settled\n")
