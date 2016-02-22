function _display_table(t, spaces)
    if string.len(spaces) > 12 then
        print("<nesting too deep>")
        return
    end
    for k,v in pairs(t) do
        if type(v) == "string" or type(v) == "number" then
            print(string.format("%s%s: %q", spaces, k, v))
        elseif type(v) == "table" then
            if k == "package" or k == "_G" then
                print(string.format("%s%s: <not displayed>", spaces, k))
            else
                print(string.format("%s%s: <table>", spaces, k))
                _display_table(v, spaces.."    ")
                collectgarbage()
            end
        else
            print(string.format("%s%s: <%s>", spaces, k, type(v)))
        end
    end
    k = nil
    v = nil
    collectgarbage()
end

function display_table(t)
    _display_table(t, "")
end

function file_list()
    display_table(file.list())
end
