function serialize(o)
    if type(o) == "number" then
        file.write(o)
    elseif type(o) == "string" then
        -- Safer string write to handle escapes of any sort
        file.write(string.format("%q", o))
    elseif type(o) == "table" then
        file.write("{")
        for k,v in pairs(o) do
            file.write("[")
            if type(k) == "number" then
                file.write(string.format("%d", k))
            else
                file.write(string.format("%q", k))
            end
            file.write("]=")
            serialize(v)
            file.write(",")
        end
        file.write("}")
    else
        print("Cannot serialize type " .. type(o))
    end
end

function serialize_file(filename, data)
    file.open(filename, "w")
    file.write("return ")
    serialize(data)
    file.close()

    file.open(filename, "r")
    print(file.read(1024))
    file.close()

    node.compile(filename)
    file.remove(filename)
end
