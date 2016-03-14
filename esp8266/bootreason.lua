local reason, extreason, cause, epc1, epc2, epc3, vaddr, depc = node.bootreason()

local reason_str = {
    [1] = 'power on',
    [2] = 'reset',
    [3] = 'hardware reset via reset pin',
    [4] = 'watchdog reset',
}

local extreason_str = {
    [0] = 'power on',
    [1] = 'hardware watchdog reset',
    [2] = 'exception reset',
    [3] = 'software watchdog reset',
    [4] = 'software restart',
    [5] = 'wake from deep sleep',
    [6] = 'external reset',
}

print('Boot Reason:', reason_str[reason])
print('Extended reason:', extreason_str[extreason])

if extreason == 3 then
    print('Cause:', cause)
    print('EPC1:', epc1)
    print('EPC2:', epc2)
    print('EPC3:', epc3)
    print('Virtual Addr:', vaddr)
    print('DEPC:', depc)
    print('')
end

function former_run_failed()
    local _, extreason = node.bootreason()
    if extreason == nil or extreason == 0 or extreason == 5 or extreason == 4 or extreason == 6 then
        return false
    end
    return true
end
