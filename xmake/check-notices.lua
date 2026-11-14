-- `xmake check-notices`: fails when a binary the release may ship uses a package the third-party notices table
-- (xmake/modules/third_party_notices.lua) does not know. `xmake package-release` stops on the same error, but only at
-- the release; this runs in seconds, without a build, so CI and the `check-libraries` skill catch it in the pull
-- request.
task("check-notices")
    set_menu {
        usage = "xmake check-notices",
        description = "Check that the third-party notices know every package a shipped binary uses",
        options = {},
    }

    on_run(function ()
        import("core.project.config")
        import("core.project.project")
        import("third_party_notices")

        config.load()
        project.load_targets()
        local rootdir = os.projectdir()
        -- The tests and the examples are never in the zip: their packages (gtest, sfml...) need no notice.
        local function shipped(target)
            local kind = target:kind()
            if kind ~= "shared" and kind ~= "binary" then
                return false
            end
            local folder = path.relative(target:scriptdir(), rootdir):gsub("\\", "/")
            return not (folder:startswith("tests") or folder:startswith("examples"))
        end
        local names = {}
        for name, target in pairs(project.targets()) do
            if shipped(target) then
                table.insert(names, name)
            end
        end
        table.sort(names)
        local errors = {}
        for _, name in ipairs(names) do
            -- `try` returns nil on an error, whatever the catch returns: the message is kept by the catch.
            try { function () third_party_notices.rowsOfTarget(name) end,
                  catch { function (e) table.insert(errors, tostring(e)) end } }
        end
        if #errors > 0 then
            raise("%d shipped binaries use packages without a notice:\n%s", #errors, table.concat(errors, "\n"))
        end
        print("third-party notices: the %d shipped binaries are covered", #names)
    end)
task_end()
