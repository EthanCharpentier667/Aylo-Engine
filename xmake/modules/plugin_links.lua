-- The plugin libraries a plugin library links, written next to it as `<library file>.links`: one file name per line,
-- sorted, empty when it links none. The game export reads it: a library it keeps needs the plugin libraries it links in
-- the same folder, even when its plugins do not require theirs (docs/release/export.md, EXP-01).

-- Writes the `.links` file of the plugin library `target` (a target with the rule aylo.plugin_library or
-- aylo.game_plugin).
function write(target)
    local layout = import("layout").names()
    local names = {}
    for _, dep in ipairs(target:orderdeps()) do
        if dep:kind() == "shared" and dep:rule("aylo.plugin_library") then
            table.insert(names, path.filename(dep:targetfile()))
        end
    end
    table.sort(names)
    local text = #names > 0 and (table.concat(names, "\n") .. "\n") or ""
    io.writefile(target:targetfile() .. layout.linksExtension, text)
end
